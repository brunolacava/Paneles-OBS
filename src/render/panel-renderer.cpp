#include "render/panel-renderer.hpp"

#include "common/platform.hpp"
#include "model/logo-store.hpp"
#include "render/logo-source.hpp"
#include "render/text-measurer.hpp"
#include "templates/panel-template.hpp"
#include "templates/template-registry.hpp"

#include <QPainter>

#include <algorithm>
#include <cmath>

namespace sp {

namespace {

constexpr int kMaxImageSide = 8192; // hard cap on the produced texture

// Resolves the model + template defaults into the flat context templates work with.
PanelContext buildContext(const PanelModel &m, const PanelTemplate &tpl, const TextMeasurer &measurer)
{
	const TemplateDefaults d = tpl.defaults();

	PanelContext ctx;
	ctx.model = &m;
	ctx.measurer = &measurer;

	// ---- colors: user colors fall back to the template's per color if invalid
	if (m.useTemplateColors) {
		ctx.palette = d.palette;
	} else {
		ctx.palette = m.colors;
		if (!ctx.palette.primary.isValid())
			ctx.palette.primary = d.palette.primary;
		if (!ctx.palette.secondary.isValid())
			ctx.palette.secondary = d.palette.secondary;
		if (!ctx.palette.title.isValid())
			ctx.palette.title = d.palette.title;
		if (!ctx.palette.text.isValid())
			ctx.palette.text = d.palette.text;
		if (!ctx.palette.background.isValid())
			ctx.palette.background = d.palette.background;
		if (!ctx.palette.logoBlock.isValid())
			ctx.palette.logoBlock = d.palette.logoBlock;
	}

	// ---- fonts
	ctx.titleFont = TextMeasurer::makeFont(m.useTemplateFonts ? d.titleFont : m.titleFont);
	ctx.textFont = TextMeasurer::makeFont(m.useTemplateFonts ? d.textFont : m.textFont);
	ctx.titleLine = measurer.lineHeight(ctx.titleFont);
	ctx.textLine = measurer.lineHeight(ctx.textFont);

	ctx.pad = m.padding >= 0 ? m.padding : d.padding;

	// ---- content
	QString title = m.title.trimmed();
	if (m.titleUppercase)
		title = title.toUpper();
	const QString text = m.text.trimmed();
	ctx.hasTitle = m.showTitle && !title.isEmpty();
	ctx.hasText = m.showText && !text.isEmpty();
	// Hidden elements are removed from the context entirely so no template can draw them by mistake.
	ctx.title = ctx.hasTitle ? title : QString();
	ctx.text = ctx.hasText ? text : QString();

	// ---- logo (a missing / broken file simply means "no logo")
	if (m.showLogo && !m.logoPath.isEmpty()) {
		const QString abs = LogoStore::resolve(m.logoPath);
		if (!abs.isEmpty()) {
			ctx.logo = LogoSource::load(abs);
			ctx.hasLogo = ctx.logo.isValid();
		}
	}
	return ctx;
}

} // namespace

QSize PanelRenderer::measure(const PanelModel &model)
{
	const PanelTemplate *tpl = TemplateRegistry::instance().find(model.templateId);
	const TextMeasurer measurer;
	const PanelContext ctx = buildContext(model, *tpl, measurer);
	const PanelLayout layout = tpl->layout(ctx);
	return layout.size.toSize();
}

RenderResult PanelRenderer::render(const PanelModel &model, qreal scale)
{
	RenderResult result;
	try {
		const PanelTemplate *tpl = TemplateRegistry::instance().find(model.templateId);
		const TextMeasurer measurer;
		const PanelContext ctx = buildContext(model, *tpl, measurer);
		const PanelLayout layout = tpl->layout(ctx);

		const QSize logical = layout.size.toSize();
		if (logical.width() < 1 || logical.height() < 1)
			return result;

		// Keep the texture within sane bounds whatever the requested scale is.
		qreal s = std::clamp<qreal>(scale, 0.1, 8.0);
		const int longest = std::max(logical.width(), logical.height());
		if (longest * s > kMaxImageSide)
			s = static_cast<qreal>(kMaxImageSide) / longest;

		const QSize px(std::max(1, int(std::lround(logical.width() * s))),
			       std::max(1, int(std::lround(logical.height() * s))));

		QImage image(px, QImage::Format_RGBA8888_Premultiplied);
		if (image.isNull()) {
			log(LogLevel::Error, QStringLiteral("Cannot allocate a %1x%2 image").arg(px.width()).arg(px.height()));
			return result;
		}
		image.fill(Qt::transparent);

		{
			QPainter painter(&image);
			painter.setRenderHint(QPainter::Antialiasing, true);
			painter.setRenderHint(QPainter::TextAntialiasing, true);
			painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
			// Map logical panel coordinates onto the (possibly supersampled) image exactly.
			painter.scale(qreal(px.width()) / logical.width(), qreal(px.height()) / logical.height());
			tpl->paint(painter, ctx, layout);
			painter.end();
		}

		result.image = std::move(image);
		result.logicalSize = logical;
		result.scale = s;
	} catch (const std::exception &e) {
		log(LogLevel::Error, QStringLiteral("Render failed: %1").arg(QString::fromUtf8(e.what())));
		result = RenderResult();
	} catch (...) {
		log(LogLevel::Error, QStringLiteral("Render failed with an unknown error"));
		result = RenderResult();
	}
	return result;
}

} // namespace sp
