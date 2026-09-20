#pragma once

#include "model/panel-model.hpp"

#include <QImage>
#include <QWidget>

namespace sp {

// Live preview of a panel. Uses PanelRenderer - the very same code path as the
// OBS source - so what you see here is what goes on air.
class PreviewWidget : public QWidget {
	Q_OBJECT

public:
	explicit PreviewWidget(QWidget *parent = nullptr);

	void setModel(const PanelModel &model); // re-renders immediately
	void clear();

	QSize sizeHint() const override { return QSize(360, 170); }
	QSize minimumSizeHint() const override { return QSize(200, 130); }

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	QImage m_image;
	QSize m_logical;
};

} // namespace sp
