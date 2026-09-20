#pragma once

#include "templates/panel-template.hpp"

namespace sp {

// Panel 3 - Tag / label
//
//   +--------------+
//   | [logo] TITLE |
//   +--------------+
//      Main text on its own strip
class TagTemplate final : public PanelTemplate {
public:
	QString id() const override { return QStringLiteral("tag"); }
	QString defaultName() const override { return QStringLiteral("Tag / label"); }
	TemplateDefaults defaults() const override;
	void paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &layout) const override;

protected:
	qreal naturalWidth(const PanelContext &ctx) const override;
	PanelLayout layoutAtWidth(const PanelContext &ctx, qreal width, std::optional<qreal> fixedHeight) const override;

private:
	struct Metrics;
	static Metrics metrics(const PanelContext &ctx);
	static qreal tagWidthNeeded(const PanelContext &ctx, const Metrics &m);
};

} // namespace sp
