#include "ui/template-picker.hpp"

#include "common/platform.hpp"
#include "render/panel-renderer.hpp"
#include "templates/template-registry.hpp"

#include <QGridLayout>
#include <QIcon>
#include <QPixmap>

namespace sp {

QString templateLabel(const QString &templateId)
{
	const QByteArray key = QByteArray("Template.") + templateId.toUtf8();
	const QString translated = T(key.constData());
	if (translated != QString::fromUtf8(key))
		return translated;
	return TemplateRegistry::instance().find(templateId)->defaultName();
}

TemplatePicker::TemplatePicker(QWidget *parent) : QWidget(parent)
{
	// 2 x 2 grid keeps the cards large enough to read even in a narrow dock.
	auto *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setHorizontalSpacing(6);
	layout->setVerticalSpacing(6);
	int index = 0;

	m_group.setExclusive(true);
	const QSize iconSize(112, 44);

	for (const QString &id : TemplateRegistry::instance().ids()) {
		auto *button = new QToolButton(this);
		button->setProperty("templateCard", true);
		button->setCheckable(true);
		button->setAutoRaise(false);
		button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
		button->setIconSize(iconSize);
		button->setText(templateLabel(id));
		button->setToolTip(templateLabel(id));
		button->setCursor(Qt::PointingHandCursor);
		button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
		button->setMinimumWidth(80);

		// Thumbnail: the real renderer on a sample panel, scaled down.
		PanelModel sample = TemplateRegistry::instance().makeDefaultModel(id);
		sample.widthMode = WidthMode::FitText;
		sample.showLogo = false;
		const RenderResult r = PanelRenderer::render(sample, 1.0);
		if (r.ok()) {
			const QImage scaled = r.image.scaled(iconSize * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			QPixmap pm = QPixmap::fromImage(scaled);
			pm.setDevicePixelRatio(2.0);
			button->setIcon(QIcon(pm));
		}

		m_group.addButton(button);
		layout->addWidget(button, index / 2, index % 2);
		++index;
		m_buttons.insert(id, button);

		connect(button, &QToolButton::clicked, this, [this, id]() {
			if (m_current == id)
				return;
			m_current = id;
			emit templateChosen(id);
		});
	}
}

void TemplatePicker::setCurrent(const QString &id)
{
	m_current = id;
	if (auto *b = m_buttons.value(id, nullptr))
		b->setChecked(true);
}

} // namespace sp
