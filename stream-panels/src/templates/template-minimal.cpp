#include "templates/template-minimal.hpp"

#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace sp {

// rects[0] = background, rects[1] = accent bar, rects[2] = logo tile

TemplateDefaults MinimalTemplate::defaults() const
{
	TemplateDefaults d;
	d.palette.primary = QColor(QStringLiteral("#22D3EE"));    // accent bar
	d.palette.secondary = QColor(QStringLiteral("#FFFFFF"));  // (unused by this design)
	d.palette.title = QColor(QStringLiteral("#9AA4B5"));
	d.palette.text = QColor(QStringLiteral("#FFFFFF"));
	d.palette.background = QColor(QStringLiteral("#EB12161D")); // ~92% opaque
	d.palette.logoBlock = QColor(QStringLiteral("#2A3140"));
	d.titleFont = FontSpec{QString(), 24, false, false};
	d.textFont = FontSpec{QString(), 40, true, false};
	d.padding = 22;
	d.titleUppercase = false;
	d.minWidth = 300;
	d.maxWidth = 1500;
	return d;
}

qreal MinimalTemplate::accentWidth(const PanelContext &ctx)
{
	return std::max<qreal>(4.0, std::round(ctx.pad * 0.28));
}

qreal MinimalTemplate::logoSide(const PanelContext &ctx)
{
	if (!ctx.hasLogo)
		return 0.0;
	const qreal gap = std::round(ctx.pad * 0.2);
	qreal h = 0.0;
	if (ctx.hasText)
		h += ctx.textLine;
	if (ctx.hasTitle)
		h += ctx.titleLine;
	if (ctx.hasText && ctx.hasTitle)
		h += gap;
	if (h <= 0.0)
		h = ctx.textLine;
	return std::ceil(std::max(h, 56.0));
}

qreal MinimalTemplate::textX(const PanelContext &ctx)
{
	qreal x = accentWidth(ctx) + ctx.pad;
	if (ctx.hasLogo)
		x += logoSide(ctx) + std::round(ctx.pad * 0.8);
	return x;
}

qreal MinimalTemplate::naturalWidth(const PanelContext &ctx) const
{
	const qreal tw = ctx.hasText ? naturalTextWidth(ctx, ctx.textFont, ctx.text) : 0.0;
	const qreal sw = ctx.hasTitle ? naturalTextWidth(ctx, ctx.titleFont, ctx.title) : 0.0;
	return textX(ctx) + std::max(tw, sw) + ctx.pad * 1.2;
}

PanelLayout MinimalTemplate::layoutAtWidth(const PanelContext &ctx, qreal W, std::optional<qreal> fixedHeight) const
{
	const qreal p = ctx.pad;
	const qreal gap = std::round(p * 0.2);
	const qreal vpad = std::round(p * 0.8);
	const qreal side = logoSide(ctx);
	const qreal x0 = textX(ctx);
	const qreal availW = std::max<qreal>(1.0, W - x0 - p * 1.2);

	QSizeF t(0, 0), s(0, 0);
	if (ctx.hasText)
		t = block(ctx, ctx.textFont, ctx.text, availW);
	if (ctx.hasTitle)
		s = block(ctx, ctx.titleFont, ctx.title, availW);
	qreal colH = t.height() + s.height() + ((ctx.hasText && ctx.hasTitle) ? gap : 0.0);
	if (colH <= 0.0)
		colH = ctx.textLine;

	const qreal contentH = std::max(colH, side);
	qreal H = std::ceil(contentH + 2.0 * vpad);
	if (fixedHeight)
		H = *fixedHeight;

	const qreal cy = (H - colH) / 2.0;

	PanelLayout l;
	l.size = QSizeF(W, H);
	l.rects = {QRectF(0, 0, W, H), QRectF(0, 0, accentWidth(ctx), H),
		   QRectF(accentWidth(ctx) + p, (H - side) / 2.0, side, side)};
	l.logoBox = l.rects[2];
	l.textRect = QRectF(x0, cy, availW, t.height());
	l.titleRect = QRectF(x0, cy + t.height() + (ctx.hasText ? gap : 0.0), availW, s.height());
	return l;
}

void MinimalTemplate::paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &l) const
{
	const PanelPalette &c = ctx.palette;
	const QRectF bg = l.rects[0];
	const qreal radius = std::min<qreal>(std::round(ctx.textLine * 0.35), bg.height() / 2.0);

	painter.save();
	QPainterPath clip;
	clip.addRoundedRect(bg, radius, radius);
	painter.setClipPath(clip, Qt::IntersectClip);
	painter.fillRect(bg, c.background);
	painter.fillRect(l.rects[1], c.primary);
	painter.restore();

	if (ctx.hasLogo) {
		painter.save();
		painter.setPen(Qt::NoPen);
		painter.setBrush(c.logoBlock);
		painter.drawRoundedRect(l.rects[2], radius * 0.7, radius * 0.7);
		painter.restore();
		drawLogo(painter, ctx, l.logoBox);
	}

	drawText(painter, l.textRect, ctx.textFont, c.text, ctx.text, Qt::AlignVCenter);
	drawText(painter, l.titleRect, ctx.titleFont, c.title, ctx.title, Qt::AlignVCenter);
}

} // namespace sp
