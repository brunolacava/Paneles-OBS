// Stream Panels - template abstraction.
//
// A PanelTemplate is a *design*: it decides where the title, text and logo go,
// how big the panel must be for a given content, and how everything is drawn.
// It never touches the model directly; it receives a fully resolved
// PanelContext and returns / consumes a PanelLayout.
//
// To add a new design:
//   1. derive from PanelTemplate (see template-tag.cpp for the smallest one),
//   2. implement id(), defaultName(), defaults(), naturalWidth(),
//      layoutAtWidth() and paint(),
//   3. register it in TemplateRegistry's constructor.
// Nothing else in the plugin has to change.
#pragma once

#include "model/panel-model.hpp"
#include "render/logo-source.hpp"
#include "render/text-measurer.hpp"

#include <QFont>
#include <QPainter>
#include <QPolygonF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QVector>

#include <optional>

namespace sp {

// Everything a template needs to know about the panel it is laying out.
struct PanelContext {
	const PanelModel *model = nullptr;
	const TextMeasurer *measurer = nullptr;

	PanelPalette palette; // effective colors (template defaults or user colors)
	QFont titleFont;
	QFont textFont;
	qreal pad = 20.0;     // effective base padding (px)

	QString title; // final strings (trimmed, uppercased when requested)
	QString text;
	bool hasTitle = false; // shown AND non-empty
	bool hasText = false;
	bool hasLogo = false;  // shown AND decodable
	LogoSource logo;

	qreal titleLine = 0.0; // line heights, handy for fixed-size elements
	qreal textLine = 0.0;
};

// Result of the layout pass. Rectangles are in panel coordinates (logical px,
// origin top-left). `polys` / `rects` are free slots a template can use to hand
// custom geometry from layout to paint.
struct PanelLayout {
	QSizeF size;
	QRectF titleRect; // where title text is drawn
	QRectF textRect;  // where main text is drawn
	QRectF logoBox;   // box the logo is fitted into (before padding/scale)
	QVector<QRectF> rects;
	QVector<QPolygonF> polys;
};

// Per-template defaults. Applied when a panel is created and whenever the
// "use template colors / typography" switches are on.
struct TemplateDefaults {
	PanelPalette palette;
	FontSpec titleFont;
	FontSpec textFont;
	int padding = 20;
	bool titleUppercase = false;
	int minWidth = 240;
	int maxWidth = 1600;
};

class PanelTemplate {
public:
	virtual ~PanelTemplate() = default;

	virtual QString id() const = 0;
	virtual QString defaultName() const = 0; // English fallback for the UI
	virtual TemplateDefaults defaults() const = 0;

	// Full layout pass: resolves the width (fit-to-text or custom), the height
	// (auto or custom) and delegates the geometry to layoutAtWidth().
	PanelLayout layout(const PanelContext &ctx) const;

	virtual void paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &layout) const = 0;

protected:
	// Width the design needs so that no text wraps (excluding min/max clamps).
	virtual qreal naturalWidth(const PanelContext &ctx) const = 0;

	// Geometry for a given final width. `fixedHeight` is set when the user
	// chose a custom height; otherwise the template returns its natural height.
	virtual PanelLayout layoutAtWidth(const PanelContext &ctx, qreal width,
					  std::optional<qreal> fixedHeight) const = 0;

	// ---- helpers shared by all templates
	static constexpr qreal kSlack = 2.0; // avoids "last word wraps" from rounding

	static qreal snap(qreal v);                  // round to a whole pixel
	static qreal atLeast(qreal v, qreal floor);  // max(v, floor)
	static QSizeF block(const PanelContext &ctx, const QFont &font, const QString &text, qreal width);
	static qreal naturalTextWidth(const PanelContext &ctx, const QFont &font, const QString &text);

	// Draws text clipped to `rect`, vertically centered unless `vAlign` says otherwise.
	static void drawText(QPainter &painter, const QRectF &rect, const QFont &font, const QColor &color,
			     const QString &text, Qt::Alignment vAlign = Qt::AlignVCenter);

	// Fits the logo inside `box` honoring the model's padding / scale / alignment.
	// `paddingScale` lets compact designs shrink the user's logo padding proportionally.
	static void drawLogo(QPainter &painter, const PanelContext &ctx, const QRectF &box, qreal paddingScale = 1.0);
};

} // namespace sp
