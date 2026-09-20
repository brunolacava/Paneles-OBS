#pragma once

#include "templates/panel-template.hpp"

namespace sp {

// Panel 2 - Minimal
//
//   +--------------------------------------------+
//   | |  LOGO   Main text                        |
//   | |         Secondary title                  |
//   +--------------------------------------------+
class MinimalTemplate final : public PanelTemplate {
public:
	QString id() const override { return QStringLiteral("minimal"); }
	QString defaultName() const override { return QStringLiteral("Minimal"); }
	TemplateDefaults defaults() const override;
	void paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &layout) const override;

protected:
	qreal naturalWidth(const PanelContext &ctx) const override;
	PanelLayout layoutAtWidth(const PanelContext &ctx, qreal width, std::optional<qreal> fixedHeight) const override;

private:
	static qreal accentWidth(const PanelContext &ctx);
	static qreal logoSide(const PanelContext &ctx);
	static qreal textX(const PanelContext &ctx);
};

} // namespace sp
