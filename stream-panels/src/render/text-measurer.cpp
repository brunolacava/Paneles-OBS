#include "render/text-measurer.hpp"

#include <QFontMetricsF>
#include <QGuiApplication>
#include <QRectF>
#include <QStringList>

#include <algorithm>
#include <cmath>

namespace sp {

QFont TextMeasurer::makeFont(const FontSpec &spec)
{
	QFont font;
	if (spec.family.isEmpty()) {
		font = QGuiApplication::font();
	} else {
		font = QFont(spec.family);
	}
	font.setPixelSize(std::max(1, spec.pixelSize));
	font.setBold(spec.bold);
	font.setItalic(spec.italic);
	font.setHintingPreference(QFont::PreferNoHinting);
	font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
	return font;
}

qreal TextMeasurer::lineHeight(const QFont &font) const
{
	return std::ceil(QFontMetricsF(font).lineSpacing());
}

qreal TextMeasurer::naturalWidth(const QFont &font, const QString &text) const
{
	if (text.isEmpty())
		return 0.0;
	const QFontMetricsF fm(font);
	qreal widest = 0.0;
	const QStringList lines = text.split(QLatin1Char('\n'));
	for (const QString &line : lines)
		widest = std::max(widest, fm.horizontalAdvance(line));
	return widest;
}

QSizeF TextMeasurer::wrapped(const QFont &font, const QString &text, qreal maxWidth) const
{
	if (text.isEmpty())
		return QSizeF(0, 0);
	const QFontMetricsF fm(font);
	const QRectF bounds(0, 0, std::max<qreal>(1.0, maxWidth), 100000.0);
	const QRectF r = fm.boundingRect(bounds, kWrapFlags, text);
	// Never report less than one full line (some fonts report shorter ink boxes).
	const int lines = text.count(QLatin1Char('\n')) + 1;
	const qreal minHeight = fm.lineSpacing() * lines;
	return QSizeF(std::ceil(r.width()), std::ceil(std::max(r.height(), minHeight)));
}

} // namespace sp
