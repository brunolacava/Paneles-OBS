#include "templates/template-modern.hpp"

#include <algorithm>
#include <cmath>

namespace sp {

// polys[0] = body, polys[1] = logo block, polys[2] = title tab, polys[3] = chip, polys[4] = bottom stripe

TemplateDefaults ModernTemplate::defaults() const
{
	TemplateDefaults d;
	d.palette.primary = QColor(QStringLiteral("#0284C7"));    // title tab
	d.palette.secondary = QColor(QStringLiteral("#FACC15"));  // chip + bottom stripe
	d.palette.title = QColor(QStringLiteral("#FFFFFF"));
	d.palette.text = QColor(QStringLiteral("#FFFFFF"));
	d.palette.background = QColor(QStringLiteral("#F00B1220"));
	d.palette.logoBlock = QColor(QStringLiteral("#1E293B"));
	d.titleFont = FontSpec{QString(), 24, true, false};
	d.textFont = FontSpec{QString(), 36, false, false};
	d.padding = 26;
	d.titleUppercase = true;
	d.minWidth = 340;
	d.maxWidth = 1600;
	return d;
}

qreal ModernTemplate::logoSide(const PanelContext &ctx)
{
	if (!ctx.hasLogo)
		return 0.0;
	const qreal vB = std::round(ctx.pad * 0.7);
	return std::ceil((ctx.textLine + 2.0 * vB) * 1.1);
}

qreal ModernTemplate::naturalWidth(const PanelContext &ctx) const
{
	const qreal p = ctx.pad;
	const qreal vT = std::round(p * 0.35);
	const qreal vB = std::round(p * 0.7);

	// body
	const qreal bodyMin = std::ceil(ctx.textLine + 2.0 * vB);
	const qreal bodyH = ctx.hasLogo ? std::max(bodyMin, logoSide(ctx)) : bodyMin;
	const qreal shift = kSlope * bodyH;
	const qreal textW = ctx.hasText ? naturalTextWidth(ctx, ctx.textFont, ctx.text) : 0.0;
	const qreal logoTop = ctx.hasLogo ? logoSide(ctx) + shift : 0.0;
	const qreal bodyNeed = logoTop + p + textW + p + shift;

	// title tab + chip
	qreal titleNeed = 0.0;
	if (ctx.hasTitle) {
		const qreal titleH = ctx.titleLine + 2.0 * vT;
		const qreal tab = naturalTextWidth(ctx, ctx.titleFont, ctx.title) + 2.0 * p + kSlope * titleH;
		titleNeed = tab + std::round(p * 0.3) + std::round(p * 0.9);
	}
	return std::max(bodyNeed, titleNeed);
}

PanelLayout ModernTemplate::layoutAtWidth(const PanelContext &ctx, qreal W, std::optional<qreal> fixedHeight) const
{
	const qreal p = ctx.pad;
	const qreal k = kSlope;
	const qreal vT = std::round(p * 0.35);
	const qreal vB = std::round(p * 0.7);
	const qreal side = logoSide(ctx);
	const qreal chipGap = std::round(p * 0.3);
	const qreal chipW = std::round(p * 0.9);

	// ---- title tab
	qreal titleH = 0.0, tabW = 0.0;
	QRectF titleRect;
	if (ctx.hasTitle) {
		const qreal lineH = ctx.titleLine + 2.0 * vT;
		const qreal natural = naturalTextWidth(ctx, ctx.titleFont, ctx.title);
		const qreal maxTab = std::max<qreal>(1.0, W - chipGap - chipW);
		tabW = std::min(natural + 2.0 * p + k * lineH, maxTab);
		const qreal avail = std::max<qreal>(1.0, tabW - k * lineH - 2.0 * p);
		const QSizeF t = block(ctx, ctx.titleFont, ctx.title, avail);
		titleH = std::ceil(std::max(ctx.titleLine, t.height())) + 2.0 * vT;
		titleRect = QRectF(p, vT, avail, titleH - 2.0 * vT);
	}

	// ---- body (iterate: the diagonal cut eats into the text width, which can change the height)
	const qreal bodyMin = ctx.hasLogo ? std::max(std::ceil(ctx.textLine + 2.0 * vB), side)
					  : std::ceil(ctx.textLine + 2.0 * vB);
	qreal bodyH = bodyMin;
	QSizeF t(0, 0);
	for (int i = 0; i < 4; ++i) {
		const qreal shift = k * bodyH;
		const qreal left = (ctx.hasLogo ? side + shift : 0.0) + p;
		const qreal avail = std::max<qreal>(1.0, W - left - p - shift);
		t = ctx.hasText ? block(ctx, ctx.textFont, ctx.text, avail) : QSizeF(0, 0);
		const qreal next = std::max(bodyMin, std::ceil(t.height()) + 2.0 * vB);
		if (std::abs(next - bodyH) < 0.5) {
			bodyH = next;
			break;
		}
		bodyH = next;
	}

	qreal H = titleH + bodyH;
	if (fixedHeight) {
		H = *fixedHeight;
		bodyH = std::max<qreal>(1.0, H - titleH);
	}

	const qreal shift = k * bodyH;
	const qreal yT = titleH;       // body top
	const qreal yB = titleH + bodyH; // body bottom
	auto rightEdge = [&](qreal y) { return W - k * (y - yT); };

	const qreal left = (ctx.hasLogo ? side + shift : 0.0) + p;
	const qreal availText = std::max<qreal>(1.0, W - left - p - shift);

	PanelLayout l;
	l.size = QSizeF(W, H);
	l.polys.resize(5);
	l.polys[0] = QPolygonF({QPointF(0, yT), QPointF(W, yT), QPointF(W - shift, yB), QPointF(0, yB)});
	l.polys[1] = ctx.hasLogo ? QPolygonF({QPointF(0, yT), QPointF(side + shift, yT), QPointF(side, yB), QPointF(0, yB)})
				 : QPolygonF();
	if (ctx.hasTitle) {
		const qreal s = k * titleH;
		l.polys[2] = QPolygonF({QPointF(0, 0), QPointF(tabW, 0), QPointF(tabW - s, titleH), QPointF(0, titleH)});
		const qreal cx = tabW + chipGap;
		l.polys[3] = QPolygonF({QPointF(cx, 0), QPointF(cx + chipW, 0), QPointF(cx + chipW - s, titleH),
					QPointF(cx - s, titleH)});
	}
	const qreal stripe = std::max<qreal>(3.0, std::round(p * 0.16));
	l.polys[4] = QPolygonF({QPointF(0, yB - stripe), QPointF(rightEdge(yB - stripe), yB - stripe),
				QPointF(rightEdge(yB), yB), QPointF(0, yB)});

	l.titleRect = titleRect;
	l.textRect = QRectF(left, yT + vB, availText, std::max<qreal>(0.0, bodyH - 2.0 * vB - stripe * 0.5));
	l.logoBox = QRectF(0, yT, side, bodyH - stripe);
	return l;
}

void ModernTemplate::paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &l) const
{
	const PanelPalette &c = ctx.palette;

	painter.save();
	painter.setPen(Qt::NoPen);

	painter.setBrush(c.background);
	painter.drawPolygon(l.polys[0]);

	if (ctx.hasLogo) {
		painter.setBrush(c.logoBlock);
		painter.drawPolygon(l.polys[1]);
	}
	painter.setBrush(c.secondary);
	painter.drawPolygon(l.polys[4]);

	if (ctx.hasTitle) {
		painter.setBrush(c.primary);
		painter.drawPolygon(l.polys[2]);
		painter.setBrush(c.secondary);
		painter.drawPolygon(l.polys[3]);
	}
	painter.restore();

	if (ctx.hasLogo)
		drawLogo(painter, ctx, l.logoBox);

	drawText(painter, l.titleRect, ctx.titleFont, c.title, ctx.title);
	drawText(painter, l.textRect, ctx.textFont, c.text, ctx.text);
}

} // namespace sp
