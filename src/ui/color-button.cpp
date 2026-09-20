#include "ui/color-button.hpp"

#include <QColorDialog>
#include <QPainter>
#include <QPainterPath>

namespace sp {

ColorButton::ColorButton(QWidget *parent) : QPushButton(parent)
{
	setCursor(Qt::PointingHandCursor);
	setMinimumHeight(28);
	connect(this, &QPushButton::clicked, this, &ColorButton::pick);
}

void ColorButton::setColor(const QColor &color)
{
	if (m_color == color)
		return;
	m_color = color;
	update();
}

void ColorButton::pick()
{
	QColorDialog dialog(m_color.isValid() ? m_color : QColor(Qt::white), this);
	dialog.setOption(QColorDialog::ShowAlphaChannel, true);
	if (!m_title.isEmpty())
		dialog.setWindowTitle(m_title);
	if (dialog.exec() == QDialog::Accepted) {
		const QColor c = dialog.selectedColor();
		if (c.isValid() && c != m_color) {
			m_color = c;
			update();
			emit colorChanged(c);
		}
	}
}

void ColorButton::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
	QPainterPath path;
	path.addRoundedRect(r, 6, 6);

	p.save();
	p.setClipPath(path);
	p.fillRect(r, QColor("#1B1E25"));
	if (m_color.isValid()) {
		// checkerboard behind translucent colors
		if (m_color.alpha() < 255) {
			for (int y = 0; y < height(); y += 6)
				for (int x = 0; x < width(); x += 6)
					p.fillRect(x, y, 6, 6, ((x / 6 + y / 6) % 2) ? QColor("#3A3F4D") : QColor("#2A2E39"));
		}
		p.fillRect(QRectF(r.left(), r.top(), 26, r.height()), m_color);
	}
	p.restore();

	p.setPen(QPen(isEnabled() ? QColor("#2E3341") : QColor("#232733"), 1));
	p.setBrush(Qt::NoBrush);
	p.drawPath(path);

	p.setPen(isEnabled() ? QColor("#C5CAD6") : QColor("#5A6275"));
	p.drawText(QRectF(r.left() + 34, r.top(), r.width() - 38, r.height()), Qt::AlignVCenter | Qt::AlignLeft,
		   m_color.isValid() ? m_color.name(m_color.alpha() < 255 ? QColor::HexArgb : QColor::HexRgb).toUpper()
				     : QStringLiteral("—"));
}

} // namespace sp
