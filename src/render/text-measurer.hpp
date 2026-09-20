// Stream Panels - text measurement.
//
// Layout must be based on *real* glyph metrics (never on character counts).
// Everything that measures text goes through this class so that the layout
// pass and the paint pass agree on the numbers.
#pragma once

#include "model/panel-model.hpp"

#include <QFont>
#include <QSizeF>
#include <QString>

namespace sp {

class TextMeasurer {
public:
	// Text flags shared by measuring and painting (must stay identical).
	static constexpr int kWrapFlags = Qt::TextWordWrap | Qt::AlignLeft;

	// Builds a QFont from the model. Hinting is disabled so that metrics scale
	// linearly: the preview (any zoom) and the supersampled OBS texture lay out
	// identically.
	static QFont makeFont(const FontSpec &spec);

	// Height of a single line.
	qreal lineHeight(const QFont &font) const;

	// Width of the widest explicit line ('\n' separated), no wrapping.
	qreal naturalWidth(const QFont &font, const QString &text) const;

	// Size of the text wrapped at word boundaries into `maxWidth`.
	// Empty text has size (0, 0).
	QSizeF wrapped(const QFont &font, const QString &text, qreal maxWidth) const;
};

} // namespace sp
