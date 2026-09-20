#include "obs/panel-source.hpp"

#include "common/platform.hpp"
#include "model/panel-manager.hpp"
#include "render/panel-renderer.hpp"
#include "templates/template-registry.hpp"

#include <obs-module.h>
#include <graphics/graphics.h>

#include <QCoreApplication>
#include <QImage>
#include <QJsonDocument>
#include <QMetaObject>
#include <QThread>

#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>

namespace sp::obsapi {

const char *const kSourceId = "com.brunolacava.streampanels.source";

namespace {

// Setting keys stored in the scene collection.
constexpr const char *kKeyPanelId = "panel_id";     // link to the library ("" = unlinked copy)
constexpr const char *kKeySnapshot = "snapshot";    // JSON copy of the panel (keeps the source self-contained)
constexpr const char *kKeyRenderScale = "render_scale";

std::function<void(const QString &)> g_openEditor;

// -------------------------------------------------------------- state

// State shared between the OBS callbacks (graphics thread), the render job that
// runs on the Qt UI thread, and the source itself. Held by shared_ptr so a
// queued job can never outlive the memory it touches.
struct SharedState {
	std::mutex mutex;
	PanelModel model;
	int renderScale = 2;
	uint64_t requested = 0; // incremented for every render request
	uint64_t applied = 0;   // generation of the image in `pending`/last uploaded
	QImage pending;
	bool hasPending = false;
	bool closed = false;
	std::atomic<uint32_t> width{0};  // logical size = size reported to OBS
	std::atomic<uint32_t> height{0};
};

struct PanelSource {
	obs_source_t *source = nullptr;
	std::shared_ptr<SharedState> state = std::make_shared<SharedState>();
	gs_texture_t *texture = nullptr; // graphics thread only
};

// Runs on the Qt UI thread: text rendering must not happen on OBS' graphics thread.
void renderJob(const std::shared_ptr<SharedState> &state, uint64_t generation)
{
	PanelModel model;
	int scale = 2;
	{
		std::lock_guard<std::mutex> lock(state->mutex);
		if (state->closed || generation < state->requested)
			return; // source gone, or a newer request supersedes this one
		model = state->model;
		scale = state->renderScale;
	}

	const RenderResult result = PanelRenderer::render(model, scale);

	std::lock_guard<std::mutex> lock(state->mutex);
	if (state->closed || generation < state->applied)
		return;
	state->applied = generation;
	if (!result.ok()) {
		return; // keep showing the previous image rather than flashing empty
	}
	state->pending = result.image;
	state->hasPending = true;
	state->width.store(uint32_t(result.logicalSize.width()));
	state->height.store(uint32_t(result.logicalSize.height()));
}

void requestRender(const std::shared_ptr<SharedState> &state)
{
	uint64_t generation = 0;
	{
		std::lock_guard<std::mutex> lock(state->mutex);
		generation = ++state->requested;
	}
	QCoreApplication *app = QCoreApplication::instance();
	if (!app)
		return;
	if (QThread::currentThread() == app->thread()) {
		renderJob(state, generation);
	} else {
		QMetaObject::invokeMethod(
			app, [state, generation]() { renderJob(state, generation); }, Qt::QueuedConnection);
	}
}

QByteArray toJsonBytes(const PanelModel &m)
{
	return QJsonDocument(m.toJson()).toJson(QJsonDocument::Compact);
}

std::optional<PanelModel> parseSnapshot(const QString &json)
{
	if (json.isEmpty())
		return std::nullopt;
	QJsonParseError err;
	const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return std::nullopt;
	return PanelModel::fromJson(doc.object());
}

// ---------------------------------------------------------- OBS callbacks

const char *ps_get_name(void *)
{
	return obs_module_text("Source.Name");
}

void ps_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, kKeyPanelId, "");
	obs_data_set_default_string(settings, kKeySnapshot, "");
	obs_data_set_default_int(settings, kKeyRenderScale, 2);
}

// NOTE: for video sources libobs defers update() to the graphics thread, so this
// function must not touch Qt widgets or fonts directly. Rendering is handed to
// the UI thread by requestRender().
void ps_update(void *data, obs_data_t *settings)
{
	auto *s = static_cast<PanelSource *>(data);
	try {
		const QString panelId = QString::fromUtf8(obs_data_get_string(settings, kKeyPanelId));
		const QString snapshot = QString::fromUtf8(obs_data_get_string(settings, kKeySnapshot));
		int scale = int(obs_data_get_int(settings, kKeyRenderScale));
		scale = scale < 1 ? 1 : (scale > 4 ? 4 : scale);

		PanelModel model;
		bool have = false;
		auto &mgr = PanelManager::instance();

		if (!panelId.isEmpty()) {
			if (const auto fromLibrary = mgr.panel(panelId)) {
				model = *fromLibrary;
				have = true;
			}
		}
		if (!have) {
			if (auto fromSnapshot = parseSnapshot(snapshot)) {
				model = *fromSnapshot;
				have = true;
				// The scene collection references a panel this library does not know (moved to
				// another PC, library reset...): adopt the embedded copy so it can be edited.
				if (!panelId.isEmpty()) {
					model.id = panelId;
					mgr.importIfMissing(model);
				}
			}
		}
		if (!have) {
			model = TemplateRegistry::instance().makeDefaultModel(QStringLiteral("classic"));
			model.title = QStringLiteral("?");
			model.text = QString();
			blog(LOG_WARNING, "[stream-panels] source has no valid panel data; showing a placeholder");
		}

		// Keep the embedded snapshot current so the source survives on its own.
		const QByteArray json = toJsonBytes(model);
		if (json != snapshot.toUtf8())
			obs_data_set_string(settings, kKeySnapshot, json.constData());

		{
			std::lock_guard<std::mutex> lock(s->state->mutex);
			s->state->model = model;
			s->state->renderScale = scale;
		}
		requestRender(s->state);
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[stream-panels] update failed: %s", e.what());
	} catch (...) {
		blog(LOG_ERROR, "[stream-panels] update failed with an unknown error");
	}
}

void *ps_create(obs_data_t *settings, obs_source_t *source)
{
	auto *s = new PanelSource();
	s->source = source;
	ps_update(s, settings);
	return s;
}

void ps_destroy(void *data)
{
	auto *s = static_cast<PanelSource *>(data);
	if (!s)
		return;
	{
		std::lock_guard<std::mutex> lock(s->state->mutex);
		s->state->closed = true;
		s->state->pending = QImage();
		s->state->hasPending = false;
	}
	if (s->texture) {
		obs_enter_graphics();
		gs_texture_destroy(s->texture);
		obs_leave_graphics();
		s->texture = nullptr;
	}
	delete s;
}

uint32_t ps_get_width(void *data)
{
	return static_cast<PanelSource *>(data)->state->width.load();
}

uint32_t ps_get_height(void *data)
{
	return static_cast<PanelSource *>(data)->state->height.load();
}

void uploadPending(PanelSource *s)
{
	QImage image;
	{
		std::lock_guard<std::mutex> lock(s->state->mutex);
		if (!s->state->hasPending)
			return;
		image = std::move(s->state->pending);
		s->state->pending = QImage();
		s->state->hasPending = false;
	}
	if (image.isNull())
		return;

	// gs_texture_create() wants tightly packed rows. QImage rows of 32-bit
	// pixels are always contiguous, but be defensive.
	if (image.format() != QImage::Format_RGBA8888_Premultiplied)
		image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
	if (image.bytesPerLine() != image.width() * 4)
		image = image.copy(); // repack

	const uint8_t *pixels = image.constBits();
	gs_texture_t *tex = gs_texture_create(uint32_t(image.width()), uint32_t(image.height()), GS_RGBA, 1, &pixels, 0);
	if (!tex) {
		blog(LOG_WARNING, "[stream-panels] gs_texture_create failed (%dx%d)", image.width(), image.height());
		return;
	}
	if (s->texture)
		gs_texture_destroy(s->texture);
	s->texture = tex;
}

void ps_video_render(void *data, gs_effect_t *effect)
{
	auto *s = static_cast<PanelSource *>(data);
	uploadPending(s);

	gs_texture_t *tex = s->texture;
	if (!tex)
		return;
	const uint32_t w = s->state->width.load();
	const uint32_t h = s->state->height.load();
	if (w == 0 || h == 0)
		return;

	// Same drawing recipe as OBS' own image source: premultiplied alpha,
	// sRGB-correct sampling and blending.
	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

	gs_eparam_t *param = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture_srgb(param, tex);
	// The texture may be supersampled: the sprite is always drawn at the logical size.
	gs_draw_sprite(tex, 0, w, h);

	gs_blend_state_pop();
	gs_enable_framebuffer_srgb(previous);
}

// -------------------------------------------------------------- properties

bool onOpenEditor(obs_properties_t *, obs_property_t *, void *data)
{
	auto *s = static_cast<PanelSource *>(data);
	if (!s || !g_openEditor)
		return false;
	obs_data_t *settings = obs_source_get_settings(s->source);
	const QString id = QString::fromUtf8(obs_data_get_string(settings, kKeyPanelId));
	obs_data_release(settings);
	g_openEditor(id);
	return false;
}

obs_properties_t *ps_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	QString currentId;
	QString currentName;
	if (data) {
		auto *s = static_cast<PanelSource *>(data);
		obs_data_t *settings = obs_source_get_settings(s->source);
		currentId = QString::fromUtf8(obs_data_get_string(settings, kKeyPanelId));
		if (auto snap = parseSnapshot(QString::fromUtf8(obs_data_get_string(settings, kKeySnapshot))))
			currentName = snap->displayName();
		obs_data_release(settings);
	}

	obs_property_t *list = obs_properties_add_list(props, kKeyPanelId, obs_module_text("Source.Panel"),
						       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	bool foundCurrent = currentId.isEmpty();
	for (const PanelModel &m : PanelManager::instance().panels()) {
		const QString label = m.displayName().isEmpty() ? T("Panels.Untitled") : m.displayName();
		obs_property_list_add_string(list, label.toUtf8().constData(), m.id.toUtf8().constData());
		if (m.id == currentId)
			foundCurrent = true;
	}
	if (currentId.isEmpty()) {
		// Unlinked copy: still shows what it is.
		const QString label = (currentName.isEmpty() ? T("Panels.Untitled") : currentName) + QStringLiteral(" ") +
				      T("Source.Unlinked");
		obs_property_list_insert_string(list, 0, label.toUtf8().constData(), "");
	} else if (!foundCurrent) {
		const QString label = (currentName.isEmpty() ? currentId : currentName) + QStringLiteral(" ") +
				      T("Source.Missing");
		obs_property_list_insert_string(list, 0, label.toUtf8().constData(), currentId.toUtf8().constData());
	}

	obs_properties_add_button2(props, "edit_panel", obs_module_text("Source.OpenEditor"), onOpenEditor, data);

	obs_property_t *quality = obs_properties_add_list(props, kKeyRenderScale, obs_module_text("Source.Quality"),
							  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(quality, obs_module_text("Source.Quality.1x"), 1);
	obs_property_list_add_int(quality, obs_module_text("Source.Quality.2x"), 2);
	obs_property_list_add_int(quality, obs_module_text("Source.Quality.3x"), 3);
	obs_property_list_add_int(quality, obs_module_text("Source.Quality.4x"), 4);
	return props;
}

// --------------------------------------------------------------- helpers

struct EnumContext {
	std::function<void(obs_source_t *)> fn;
};

bool enumProc(void *param, obs_source_t *source)
{
	const char *id = obs_source_get_id(source);
	if (id && std::strcmp(id, kSourceId) == 0)
		static_cast<EnumContext *>(param)->fn(source);
	return true;
}

void forEachPanelSource(const std::function<void(obs_source_t *)> &fn)
{
	EnumContext ctx{fn};
	obs_enum_sources(enumProc, &ctx);
}

QString linkedPanelId(obs_source_t *source)
{
	obs_data_t *settings = obs_source_get_settings(source);
	const QString id = QString::fromUtf8(obs_data_get_string(settings, kKeyPanelId));
	obs_data_release(settings);
	return id;
}

} // namespace

// ================================================================= public

void registerPanelSource()
{
	static obs_source_info info = {};
	info.id = kSourceId;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB;
	info.get_name = ps_get_name;
	info.create = ps_create;
	info.destroy = ps_destroy;
	info.update = ps_update;
	info.get_defaults = ps_get_defaults;
	info.get_properties = ps_get_properties;
	info.get_width = ps_get_width;
	info.get_height = ps_get_height;
	info.video_render = ps_video_render;
	info.icon_type = OBS_ICON_TYPE_TEXT;
	obs_register_source(&info);
}

void setOpenEditorCallback(std::function<void(const QString &)> callback)
{
	g_openEditor = std::move(callback);
}

void refreshLinkedSources(const QString &panelId)
{
	forEachPanelSource([&](obs_source_t *source) {
		if (linkedPanelId(source) != panelId)
			return;
		// update() re-reads the library and refreshes the embedded snapshot.
		obs_data_t *touch = obs_data_create();
		obs_data_set_string(touch, kKeyPanelId, panelId.toUtf8().constData());
		obs_source_update(source, touch);
		obs_data_release(touch);
	});
}

int countLinkedSources(const QString &panelId)
{
	int n = 0;
	forEachPanelSource([&](obs_source_t *source) {
		if (linkedPanelId(source) == panelId)
			++n;
	});
	return n;
}

void unlinkSources(const QString &panelId)
{
	forEachPanelSource([&](obs_source_t *source) {
		if (linkedPanelId(source) != panelId)
			return;
		obs_data_t *touch = obs_data_create();
		obs_data_set_string(touch, kKeyPanelId, "");
		obs_source_update(source, touch);
		obs_data_release(touch);
	});
}

} // namespace sp::obsapi
