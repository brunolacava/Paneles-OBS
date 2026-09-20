// Stream Panels - template registry.
//
// The only place that knows the list of available designs. Adding a template
// means adding one line to the constructor in template-registry.cpp.
#pragma once

#include "model/panel-model.hpp"
#include "templates/panel-template.hpp"

#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

namespace sp {

class TemplateRegistry {
public:
	static TemplateRegistry &instance();

	void registerTemplate(std::unique_ptr<PanelTemplate> tpl);

	// Never null (falls back to the first registered template).
	const PanelTemplate *find(const QString &id) const;
	QStringList ids() const; // in registration order

	// A brand new panel using the given design, filled with sample content.
	PanelModel makeDefaultModel(const QString &templateId) const;

private:
	TemplateRegistry();
	std::vector<std::unique_ptr<PanelTemplate>> m_templates;
};

} // namespace sp
