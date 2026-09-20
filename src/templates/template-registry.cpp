#include "templates/template-registry.hpp"

#include "common/platform.hpp"
#include "templates/template-classic.hpp"
#include "templates/template-minimal.hpp"
#include "templates/template-modern.hpp"
#include "templates/template-tag.hpp"

namespace sp {

TemplateRegistry &TemplateRegistry::instance()
{
	static TemplateRegistry registry;
	return registry;
}

TemplateRegistry::TemplateRegistry()
{
	// ---- built-in designs (order = order shown in the UI)
	registerTemplate(std::make_unique<ClassicTemplate>());
	registerTemplate(std::make_unique<MinimalTemplate>());
	registerTemplate(std::make_unique<TagTemplate>());
	registerTemplate(std::make_unique<ModernTemplate>());
}

void TemplateRegistry::registerTemplate(std::unique_ptr<PanelTemplate> tpl)
{
	if (!tpl)
		return;
	for (const auto &existing : m_templates)
		if (existing->id() == tpl->id())
			return; // ids are unique
	m_templates.push_back(std::move(tpl));
}

const PanelTemplate *TemplateRegistry::find(const QString &id) const
{
	for (const auto &t : m_templates)
		if (t->id() == id)
			return t.get();
	return m_templates.front().get();
}

QStringList TemplateRegistry::ids() const
{
	QStringList out;
	for (const auto &t : m_templates)
		out << t->id();
	return out;
}

PanelModel TemplateRegistry::makeDefaultModel(const QString &templateId) const
{
	const PanelTemplate *tpl = find(templateId);
	const TemplateDefaults d = tpl->defaults();

	PanelModel m;
	m.id = PanelModel::newId();
	m.templateId = tpl->id();
	m.title = T("Sample.Title");
	m.text = T("Sample.Text");
	m.titleUppercase = d.titleUppercase;
	m.padding = -1;
	m.minWidth = d.minWidth;
	m.maxWidth = d.maxWidth;
	m.useTemplateColors = true;
	m.colors = d.palette;
	m.useTemplateFonts = true;
	m.titleFont = d.titleFont;
	m.textFont = d.textFont;
	m.sanitize();
	return m;
}

} // namespace sp
