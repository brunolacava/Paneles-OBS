#include "ui/preview-widget.hpp"

#include "common/platform.hpp"
#include "render/panel-renderer.hpp"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace sp {

PreviewWidget::PreviewWidget(QWidget *parent) : QWidget(parent)
{
	setMinimumHeight(130);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
}

void PreviewWidget::setModel(const PanelModel &model)
{
	// Render at 2x so the preview stays sharp on HiDPI screens and when the
	// widget is smaller than the panel (downscale looks better than upscale).
	const RenderResult r = PanelRenderer::render(model, 2.0);
	m_image = r.image;
	m_logical = r.logicalSize;
	update();
}

void PreviewWidget::clear()
{
	m_image = QImage();
	m_logical = QSize();
	update();
}

void PreviewWidget::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);

	const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
	QPainterPath path;
	path.addRoundedRect(frame, 8, 8);

	p.save();
	p.setClipPath(path);
	// Transparency checkerboard: makes translucent panels readable.
	const int cell = 10;
	for (int y = 0; y < height(); y += cell)
		for (int x = 0; x < width(); x += cell)
			p.fillRect(x, y, cell, cell, ((x / cell + y / cell) % 2) ? QColor("#2F3441") : QColor("#282C37"));

	if (!m_image.isNull() && m_logical.isValid()) {
		const QRectF avail = QRectF(rect()).adjusted(16, 16, -16, -30);
		qreal s = std::min(avail.width() / m_logical.width(), avail.height() / m_logical.height());
		s = std::min<qreal>(s, 1.0); // never blow small panels up beyond their real size
		const QSizeF shown(m_logical.width() * s, m_logical.height() * s);
		const QRectF target(avail.left() + (avail.width() - shown.width()) / 2.0,
				    avail.top() + (avail.height() - shown.height()) / 2.0, shown.width(), shown.height());
		p.drawImage(target, m_image);

		p.setPen(QColor("#8B93A7"));
		QFont f = font();
		f.setPixelSize(11);
		p.setFont(f);
		p.drawText(QRectF(12, height() - 24, width() - 24, 18), Qt::AlignRight | Qt::AlignVCenter,
			   QStringLiteral("%1 × %2 px").arg(m_logical.width()).arg(m_logical.height()));
	} else {
		p.setPen(QColor("#8B93A7"));
		p.drawText(rect(), Qt::AlignCenter, T("Panels.Empty"));
	}
	p.restore();

	p.setPen(QPen(QColor("#2E3341"), 1));
	p.setBrush(Qt::NoBrush);
	p.drawPath(path);
}

} // namespace sp
