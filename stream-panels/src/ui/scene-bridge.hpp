// Stream Panels - the only door from the UI into OBS.
//
// The dock editor never includes libobs headers. Everything it needs from the
// host application goes through this small interface, implemented by
// obs/obs-scene-bridge.cpp in the plugin and by a stub in tests.
#pragma once

#include <QString>

namespace sp {

class SceneBridge {
public:
	virtual ~SceneBridge() = default;

	// Creates a "Stream Panel" source linked to `panelId` and adds it to the
	// scene the operator is working on (the preview scene in Studio Mode).
	// Returns false (and fills `error`) if that is not possible.
	virtual bool addToCurrentScene(const QString &panelId, const QString &sourceName, QString *error) = 0;

	// How many OBS sources are currently linked to this panel.
	virtual int usageCount(const QString &panelId) = 0;

	// Detaches every source from the panel; they keep their current look.
	virtual void unlinkSources(const QString &panelId) = 0;
};

} // namespace sp
