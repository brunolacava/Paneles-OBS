#pragma once

#include <QButtonGroup>
#include <QHash>
#include <QString>
#include <QToolButton>
#include <QWidget>

namespace sp {

// Row of visual "cards", one per registered template, each with a thumbnail
// rendered by the real renderer.
class TemplatePicker : public QWidget {
	Q_OBJECT

public:
	explicit TemplatePicker(QWidget *parent = nullptr);

	QString current() const { return m_current; }
	void setCurrent(const QString &id); // does not emit templateChosen

signals:
	void templateChosen(const QString &id);

private:
	QButtonGroup m_group;
	QHash<QString, QToolButton *> m_buttons;
	QString m_current;
};

QString templateLabel(const QString &templateId);

} // namespace sp
