#pragma once

#include "templates/panel-template.hpp"

namespace sp {

// Panel 1 - Classic lower third
//
//   +-----------------------------------------+
//   | TITLE                                   |
//   +-------+---------------------------------+
//   | LOGO  | Main text                       |
//   +-------+---------------------------------+
class ClassicTemplate final : public PanelTemplate {
public:
	QString id() const override { return QStringLiteral("classic"); }
	QString defaultName() const override { return QStringLiteral("Classic lower third"); }
	TemplateDefaults defaults() const override;
	void paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &layout) const override;

protected:
	qreal naturalWidth(const PanelContext &ctx) const override;
	PanelLayout layoutAtWidth(const PanelContext &ctx, qreal width, std::optional<qreal> fixedHeight) const override;

private:
	static qreal logoSide(const PanelContext &ctx);
};

} // namespace sp
