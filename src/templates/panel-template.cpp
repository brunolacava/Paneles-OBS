#include "templates/panel-template.hpp"

#include <algorithm>
#include <cmath>

namespace sp {

PanelLayout PanelTemplate::layout(const PanelContext &ctx) const
{
	const PanelModel &m = *ctx.model;

	qreal width = 0.0;
	if (m.widthMode == WidthMode::Custom) {
		width = m.customWidth;
	} else {
		const qreal lo = m.minWidth;
		const qreal hi = std::max<qreal>(lo, m.maxWidth);
		width = std::clamp(std::ceil(naturalWidth(ctx)), lo, hi);
	}
	width = std::clamp<qreal>(width, limits::kMinDimension, limits::kMaxDimension);

	std::optional<qreal> fixedHeight;
	if (m.heightMode == HeightMode::Custom)
		fixedHeight = std::clamp<qreal>(m.customHeight, limits::kMinDimension, limits::kMaxDimension);

	PanelLayout l = layoutAtWidth(ctx, width, fixedHeight);

	// Whole pixels everywhere: crisp edges, and the texture size matches the source size.
	l.size = QSizeF(std::ceil(std::max<qreal>(1.0, l.size.width())), std::ceil(std::max<qreal>(1.0, l.size.height())));
	l.size.setWidth(std::min<qreal>(l.size.width(), limits::kMaxDimension));
	l.size.setHeight(std::min<qreal>(l.size.height(), limits::kMaxDimension));
	return l;
}

qreal PanelTemplate::snap(qreal v)
{
	return std::round(v);
}

qreal PanelTemplate::atLeast(qreal v, qreal floor)
{
	return std::max(v, floor);
}

QSizeF PanelTemplate::block(const PanelContext &ctx, const QFont &font, const QString &text, qreal width)
{
	return ctx.measurer->wrapped(font, text, std::max<qreal>(1.0, width));
}

qreal PanelTemplate::naturalTextWidth(const PanelContext &ctx, const QFont &font, const QString &text)
{
	if (text.isEmpty())
		return 0.0;
	return std::ceil(ctx.measurer->naturalWidth(font, text)) + kSlack;
}

void PanelTemplate::drawText(QPainter &painter, const QRectF &rect, const QFont &font, const QColor &color,
			     const QString &text, Qt::Alignment vAlign)
{
	if (text.isEmpty() || rect.width() <= 0 || rect.height() <= 0)
		return;
	painter.save();
	painter.setClipRect(rect, Qt::IntersectClip);
	painter.setFont(font);
	painter.setPen(color);
	painter.drawText(rect, int(TextMeasurer::kWrapFlags & ~Qt::AlignVertical_Mask) | int(vAlign), text);
	painter.restore();
}

void PanelTemplate::drawLogo(QPainter &painter, const PanelContext &ctx, const QRectF &box, qreal paddingScale)
{
	if (!ctx.hasLogo)
		return;
	const PanelModel &m = *ctx.model;
	const qreal pad = m.logoPadding * paddingScale;
	const QRectF inner = box.adjusted(pad, pad, -pad, -pad);
	if (inner.width() <= 0 || inner.height() <= 0)
		return;

	Qt::Alignment align = Qt::AlignHCenter;
	if (m.logoAlign == LogoAlign::Left)
		align = Qt::AlignLeft;
	else if (m.logoAlign == LogoAlign::Right)
		align = Qt::AlignRight;

	const QRectF target = ctx.logo.fitRect(inner, m.logoScalePercent / 100.0, align);
	ctx.logo.paint(painter, target);
}

} // namespace sp
