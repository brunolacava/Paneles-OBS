#pragma once

#include "ui/scene-bridge.hpp"

namespace sp::obsapi {

// SceneBridge implementation backed by libobs + obs-frontend-api.
class ObsSceneBridge final : public SceneBridge {
public:
	bool addToCurrentScene(const QString &panelId, const QString &sourceName, QString *error) override;
	int usageCount(const QString &panelId) override;
	void unlinkSources(const QString &panelId) override;
};

} // namespace sp::obsapi
