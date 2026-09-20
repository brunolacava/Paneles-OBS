#include "templates/template-tag.hpp"

#include <QPainterPath>

#include <algorithm>
#include <cmath>

namespace sp {

// rects[0] = tag, rects[1] = text strip, rects[2] = logo tile (inside the tag)

struct TagTemplate::Metrics {
	qreal p = 0;       // horizontal padding
	qreal vT = 0;      // vertical padding of the tag
	qreal vS = 0;      // vertical padding of the text strip
	qreal inset = 0;   // x offset of the strip under the tag
	qreal gapV = 0;    // vertical gap between tag and strip
	qreal logoSide = 0;
	qreal logoGap = 0;
	qreal logoInset = 0;
};

TemplateDefaults TagTemplate::defaults() const
{
	TemplateDefaults d;
	d.palette.primary = QColor(QStringLiteral("#7C3AED"));    // tag
	d.palette.secondary = QColor(QStringLiteral("#F472B6"));  // strip accent
	d.palette.title = QColor(QStringLiteral("#FFFFFF"));
	d.palette.text = QColor(QStringLiteral("#FFFFFF"));
	d.palette.background = QColor(QStringLiteral("#EB0F172A"));
	d.palette.logoBlock = QColor(QStringLiteral("#FFFFFF"));
	d.titleFont = FontSpec{QString(), 24, true, false};
	d.textFont = FontSpec{QString(), 34, false, false};
	d.padding = 20;
	d.titleUppercase = true;
	d.minWidth = 200;
	d.maxWidth = 1400;
	return d;
}

TagTemplate::Metrics TagTemplate::metrics(const PanelContext &ctx)
{
	Metrics m;
	m.p = ctx.pad;
	m.vT = std::round(ctx.pad * 0.4);
	m.vS = std::round(ctx.pad * 0.5);
	m.inset = ctx.hasTitle ? std::round(ctx.pad * 1.0) : 0.0;
	m.gapV = ctx.hasTitle && ctx.hasText ? std::round(ctx.pad * 0.25) : 0.0;
	m.logoInset = std::round(m.vT * 0.5);
	if (ctx.hasLogo) {
		m.logoSide = std::ceil(ctx.titleLine + 2.0 * m.vT - 2.0 * m.logoInset);
		m.logoGap = std::round(ctx.pad * 0.5);
	}
	return m;
}

qreal TagTemplate::tagWidthNeeded(const PanelContext &ctx, const Metrics &m)
{
	if (!ctx.hasTitle && !ctx.hasLogo)
		return 0.0;
	qreal w = 0.0;
	if (ctx.hasLogo)
		w += m.logoInset + m.logoSide;
	if (ctx.hasTitle) {
		w += ctx.hasLogo ? m.logoGap : m.p;
		w += naturalTextWidth(ctx, ctx.titleFont, ctx.title) + m.p;
	} else {
		w += m.logoInset; // logo-only tag
	}
	return w;
}

qreal TagTemplate::naturalWidth(const PanelContext &ctx) const
{
	const Metrics m = metrics(ctx);
	const qreal stripNeed =
		ctx.hasText ? m.inset + m.p + 3.0 + naturalTextWidth(ctx, ctx.textFont, ctx.text) + m.p : 0.0;
	return std::max(tagWidthNeeded(ctx, m), stripNeed);
}

PanelLayout TagTemplate::layoutAtWidth(const PanelContext &ctx, qreal W, std::optional<qreal> fixedHeight) const
{
	const Metrics m = metrics(ctx);
	const bool hasTag = ctx.hasTitle || ctx.hasLogo;

	// ---- tag
	qreal tagW = 0.0, tagH = 0.0;
	QRectF titleRect, logoTile;
	if (hasTag) {
		tagW = std::min(W, std::ceil(tagWidthNeeded(ctx, m)));

		const qreal titleLeft = ctx.hasLogo ? m.logoInset + m.logoSide + m.logoGap : m.p;
		const qreal titleAvail = std::max<qreal>(1.0, tagW - titleLeft - m.p);
		QSizeF t(0, 0);
		if (ctx.hasTitle)
			t = block(ctx, ctx.titleFont, ctx.title, titleAvail);
		tagH = std::ceil(std::max(ctx.titleLine, t.height())) + 2.0 * m.vT;
		titleRect = QRectF(titleLeft, m.vT, titleAvail, tagH - 2.0 * m.vT);
		if (ctx.hasLogo)
			logoTile = QRectF(m.logoInset, m.logoInset, m.logoSide,
					  std::max<qreal>(m.logoSide, tagH - 2.0 * m.logoInset));
	}

	// ---- strip
	qreal stripX = m.inset, stripY = hasTag ? tagH + m.gapV : 0.0, stripW = 0.0, stripH = 0.0;
	QRectF textRect;
	if (ctx.hasText) {
		stripW = std::max<qreal>(1.0, W - stripX);
		const qreal availText = std::max<qreal>(1.0, stripW - 2.0 * m.p);
		const QSizeF t = block(ctx, ctx.textFont, ctx.text, availText);
		stripH = std::ceil(std::max(ctx.textLine, t.height())) + 2.0 * m.vS;
	}

	qreal H = std::max(tagH, 0.0);
	if (ctx.hasText)
		H = stripY + stripH;
	if (H <= 0.0)
		H = std::ceil(ctx.titleLine + 2.0 * m.vT);
	if (fixedHeight) {
		H = *fixedHeight;
		if (ctx.hasText)
			stripH = std::max<qreal>(1.0, H - stripY);
	}
	if (ctx.hasText)
		textRect = QRectF(stripX + m.p + 3.0, stripY + m.vS, std::max<qreal>(1.0, stripW - 2.0 * m.p - 3.0),
				  std::max<qreal>(0.0, stripH - 2.0 * m.vS));

	PanelLayout l;
	l.size = QSizeF(W, H);
	l.rects = {QRectF(0, 0, tagW, tagH), QRectF(stripX, stripY, stripW, stripH), logoTile};
	l.titleRect = titleRect;
	l.textRect = textRect;
	l.logoBox = logoTile;
	return l;
}

void TagTemplate::paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &l) const
{
	const PanelPalette &c = ctx.palette;
	const QRectF tag = l.rects[0];
	const QRectF strip = l.rects[1];
	const bool hasTag = ctx.hasTitle || ctx.hasLogo;

	painter.save();
	painter.setPen(Qt::NoPen);

	if (ctx.hasText && strip.width() > 0 && strip.height() > 0) {
		const qreal r = std::min<qreal>(std::round(strip.height() * 0.22), strip.height() / 2.0);
		QPainterPath clip;
		clip.addRoundedRect(strip, r, r);
		painter.setClipPath(clip);
		painter.fillRect(strip, c.background);
		painter.fillRect(QRectF(strip.left(), strip.top(), 5.0, strip.height()), c.secondary);
		painter.setClipping(false);
	}

	if (hasTag && tag.width() > 0) {
		const qreal r = std::min<qreal>(std::round(tag.height() * 0.28), tag.height() / 2.0);
		painter.setBrush(c.primary);
		painter.drawRoundedRect(tag, r, r);
		if (ctx.hasLogo) {
			painter.setBrush(c.logoBlock);
			painter.drawRoundedRect(l.rects[2], r * 0.7, r * 0.7);
		}
	}
	painter.restore();

	if (ctx.hasLogo)
		drawLogo(painter, ctx, l.logoBox, 0.3);

	drawText(painter, l.titleRect, ctx.titleFont, c.title, ctx.title);
	drawText(painter, l.textRect, ctx.textFont, c.text, ctx.text);
}

} // namespace sp
