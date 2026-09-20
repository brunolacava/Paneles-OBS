#include "templates/template-classic.hpp"

#include <algorithm>
#include <cmath>

namespace sp {

// rects[0] = title strip, rects[1] = body row, rects[2] = logo block, rects[3] = divider

TemplateDefaults ClassicTemplate::defaults() const
{
	TemplateDefaults d;
	d.palette.primary = QColor(QStringLiteral("#C8102E"));    // title strip
	d.palette.secondary = QColor(QStringLiteral("#FFC72C"));  // divider line
	d.palette.title = QColor(QStringLiteral("#FFFFFF"));
	d.palette.text = QColor(QStringLiteral("#14171F"));
	d.palette.background = QColor(QStringLiteral("#F2F3F5"));
	d.palette.logoBlock = QColor(QStringLiteral("#FFFFFF"));
	d.titleFont = FontSpec{QString(), 26, true, false};
	d.textFont = FontSpec{QString(), 36, false, false};
	d.padding = 24;
	d.titleUppercase = true;
	d.minWidth = 320;
	d.maxWidth = 1600;
	return d;
}

qreal ClassicTemplate::logoSide(const PanelContext &ctx)
{
	if (!ctx.hasLogo)
		return 0.0;
	const qreal vB = std::round(ctx.pad * 0.6);
	// A little more presence than a plain text row, derived from line height
	// (not from wrapped height) so the logo never depends on the text width.
	return std::ceil((ctx.textLine + 2.0 * vB) * 1.25);
}

qreal ClassicTemplate::naturalWidth(const PanelContext &ctx) const
{
	const qreal p = ctx.pad;
	const qreal titleNeed = ctx.hasTitle ? naturalTextWidth(ctx, ctx.titleFont, ctx.title) + 2.0 * p : 0.0;
	const qreal textNeed = logoSide(ctx) + p + naturalTextWidth(ctx, ctx.textFont, ctx.hasText ? ctx.text : QString()) + p;
	return std::max(titleNeed, textNeed);
}

PanelLayout ClassicTemplate::layoutAtWidth(const PanelContext &ctx, qreal W, std::optional<qreal> fixedHeight) const
{
	const qreal p = ctx.pad;
	const qreal vT = std::round(p * 0.45);
	const qreal vB = std::round(p * 0.6);
	const qreal side = logoSide(ctx);

	// Title strip
	qreal titleH = 0.0;
	if (ctx.hasTitle) {
		const QSizeF t = block(ctx, ctx.titleFont, ctx.title, W - 2.0 * p);
		titleH = std::ceil(t.height()) + 2.0 * vT;
	}

	// Body row
	QSizeF x(0, 0);
	if (ctx.hasText)
		x = block(ctx, ctx.textFont, ctx.text, W - side - 2.0 * p);
	qreal bodyMin = std::ceil(ctx.textLine + 2.0 * vB);
	if (ctx.hasLogo)
		bodyMin = std::max(bodyMin, side);
	qreal bodyH = std::max(bodyMin, std::ceil(x.height()) + 2.0 * vB);

	qreal H = titleH + bodyH;
	if (fixedHeight) {
		H = *fixedHeight;
		bodyH = std::max<qreal>(1.0, H - titleH);
	}

	PanelLayout l;
	l.size = QSizeF(W, H);
	const QRectF titleStrip(0, 0, W, titleH);
	const QRectF body(0, titleH, W, bodyH);
	const QRectF logoBlock(0, titleH, side, bodyH);
	const qreal divH = std::max<qreal>(2.0, std::round(p * 0.14));
	const QRectF divider(0, titleH, W, ctx.hasTitle ? divH : 0.0);

	l.rects = {titleStrip, body, logoBlock, divider};
	l.titleRect = QRectF(p, vT, std::max<qreal>(1.0, W - 2.0 * p), std::max<qreal>(0.0, titleH - 2.0 * vT));
	l.textRect = QRectF(side + p, titleH + vB, std::max<qreal>(1.0, W - side - 2.0 * p),
			    std::max<qreal>(0.0, bodyH - 2.0 * vB));
	l.logoBox = logoBlock;
	return l;
}

void ClassicTemplate::paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &l) const
{
	const PanelPalette &c = ctx.palette;

	// body first, then the title strip on top
	painter.fillRect(l.rects[1], c.background);
	if (ctx.hasLogo)
		painter.fillRect(l.rects[2], c.logoBlock);
	if (ctx.hasTitle) {
		painter.fillRect(l.rects[0], c.primary);
		painter.fillRect(l.rects[3], c.secondary);
	}

	if (ctx.hasLogo)
		drawLogo(painter, ctx, l.logoBox);

	drawText(painter, l.titleRect, ctx.titleFont, c.title, ctx.title);
	drawText(painter, l.textRect, ctx.textFont, c.text, ctx.text);
}

} // namespace sp
