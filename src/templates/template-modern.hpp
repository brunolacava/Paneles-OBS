#pragma once

#include "templates/panel-template.hpp"

namespace sp {

// Panel 4 - Modern / dynamic
//
// A slanted title tab with an accent chip sits on top of a body block whose
// right edge (and the logo block's right edge) is cut diagonally. A thin
// accent stripe runs along the bottom. All angles share one slope (kSlope).
class ModernTemplate final : public PanelTemplate {
public:
	QString id() const override { return QStringLiteral("modern"); }
	QString defaultName() const override { return QStringLiteral("Modern / dynamic"); }
	TemplateDefaults defaults() const override;
	void paint(QPainter &painter, const PanelContext &ctx, const PanelLayout &layout) const override;

protected:
	qreal naturalWidth(const PanelContext &ctx) const override;
	PanelLayout layoutAtWidth(const PanelContext &ctx, qreal width, std::optional<qreal> fixedHeight) const override;

private:
	static constexpr qreal kSlope = 0.42; // horizontal shift per vertical px of the diagonal cuts
	static qreal logoSide(const PanelContext &ctx);
};

} // namespace sp
