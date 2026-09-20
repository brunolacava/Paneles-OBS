// Stream Panels - logo loading and drawing.
//
// A LogoSource is a cheap, copyable handle to a decoded logo (raster image or
// SVG). Decoding is cached by (path, size, mtime), so re-rendering a panel
// while the operator types does not touch the disk.
#pragma once

#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QSizeF>
#include <QString>

#include <memory>

namespace sp {

class LogoSource {
public:
	LogoSource() = default;

	// Loads `absolutePath` (through the cache). Returns an invalid source
	// (isValid() == false) for missing / unreadable / corrupt files; never throws.
	static LogoSource load(const QString &absolutePath);

	bool isValid() const { return static_cast<bool>(m_data); }
	QSizeF size() const;    // native size in px
	qreal aspect() const;   // width / height, 1.0 when invalid

	// Draws the logo scaled to fit *exactly* inside `target` (the caller has
	// already computed an aspect-correct rectangle, see fitRect()).
	void paint(QPainter &painter, const QRectF &target) const;

	// Largest rectangle with the logo's aspect ratio that fits in `box`,
	// scaled by `scale` (1.0 = fill), placed according to `align`.
	QRectF fitRect(const QRectF &box, qreal scale, Qt::Alignment align) const;

	struct Data; // implementation detail (public only so the cache in the .cpp can name it)

private:
	std::shared_ptr<const Data> m_data;
};

} // namespace sp
