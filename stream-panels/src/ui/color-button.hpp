#pragma once

#include <QColor>
#include <QPushButton>

namespace sp {

// A flat button that shows the current color (with hex value) and opens a
// QColorDialog (with alpha channel) when clicked.
class ColorButton : public QPushButton {
	Q_OBJECT

public:
	explicit ColorButton(QWidget *parent = nullptr);

	QColor color() const { return m_color; }
	void setColor(const QColor &color); // does not emit colorChanged
	void setDialogTitle(const QString &title) { m_title = title; }

signals:
	void colorChanged(const QColor &color);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	void pick();

	QColor m_color;
	QString m_title;
};

} // namespace sp
