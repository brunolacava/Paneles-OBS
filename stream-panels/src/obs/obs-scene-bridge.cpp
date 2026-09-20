#include "obs/obs-scene-bridge.hpp"

#include "common/platform.hpp"
#include "model/panel-manager.hpp"
#include "obs/panel-source.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QJsonDocument>

namespace sp::obsapi {

namespace {

// "Name", "Name 2", "Name 3"... so the new source never collides with an existing one.
QByteArray uniqueSourceName(const QString &wanted)
{
	QString candidate = wanted;
	for (int i = 2; i < 10000; ++i) {
		obs_source_t *existing = obs_get_source_by_name(candidate.toUtf8().constData());
		if (!existing)
			break;
		obs_source_release(existing);
		candidate = QStringLiteral("%1 %2").arg(wanted).arg(i);
	}
	return candidate.toUtf8();
}

} // namespace

bool ObsSceneBridge::addToCurrentScene(const QString &panelId, const QString &sourceName, QString *error)
{
	auto fail = [&](const QString &message) {
		if (error)
			*error = message;
		return false;
	};

	const auto model = PanelManager::instance().panel(panelId);
	if (!model)
		return fail(T("Scene.PanelMissing"));

	// In Studio Mode new sources go to the preview scene, so nothing reaches
	// the program output before the operator transitions.
	obs_source_t *sceneSource = obs_frontend_preview_program_mode_active() ? obs_frontend_get_current_preview_scene()
									      : obs_frontend_get_current_scene();
	if (!sceneSource)
		return fail(T("Scene.NoScene"));

	obs_scene_t *scene = obs_scene_from_source(sceneSource);
	if (!scene) {
		obs_source_release(sceneSource);
		return fail(T("Scene.NoScene"));
	}

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "panel_id", panelId.toUtf8().constData());
	obs_data_set_string(settings, "snapshot",
			    QJsonDocument(model->toJson()).toJson(QJsonDocument::Compact).constData());

	const QByteArray name = uniqueSourceName(sourceName);
	obs_source_t *source = obs_source_create(kSourceId, name.constData(), settings, nullptr);
	obs_data_release(settings);

	bool ok = false;
	if (source) {
		obs_sceneitem_t *item = obs_scene_add(scene, source);
		if (item) {
			// Initial placement only (bottom-left, like a lower third). From here on the
			// operator moves / scales it with OBS' normal transform tools.
			struct obs_video_info ovi;
			if (obs_get_video_info(&ovi)) {
				obs_sceneitem_set_alignment(item, OBS_ALIGN_LEFT | OBS_ALIGN_BOTTOM);
				struct vec2 pos;
				vec2_set(&pos, float(ovi.base_width) * 0.04f, float(ovi.base_height) * 0.94f);
				obs_sceneitem_set_pos(item, &pos);
			}
			obs_sceneitem_select(item, true);
			ok = true;
		}
		obs_source_release(source);
	}
	obs_source_release(sceneSource);

	if (!ok)
		return fail(T("Status.AddFailed"));
	return true;
}

int ObsSceneBridge::usageCount(const QString &panelId)
{
	return countLinkedSources(panelId);
}

void ObsSceneBridge::unlinkSources(const QString &panelId)
{
	obsapi::unlinkSources(panelId);
}

} // namespace sp::obsapi
