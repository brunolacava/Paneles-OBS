/*
Stream Panels
Copyright (C) 2026 Bruno Lacava

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/bmem.h>
#include <util/platform.h>

#include <QDockWidget>
#include <QMainWindow>
#include <QMetaObject>
#include <QPointer>

#include "common/platform.hpp"
#include "model/logo-store.hpp"
#include "model/panel-manager.hpp"
#include "obs/obs-scene-bridge.hpp"
#include "obs/panel-source.hpp"
#include "plugin-support.h"
#include "ui/panel-editor.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Plugin.Description");
}

namespace {

constexpr const char *kDockId = "com.brunolacava.streampanels.dock";

// The dock (and the editor inside it) is owned by OBS' main window. We only keep
// a guarded pointer: it becomes null automatically when OBS destroys the window.
QPointer<sp::PanelEditor> g_editor;
sp::obsapi::ObsSceneBridge g_bridge; // stateless, lives for the whole process
QMetaObject::Connection g_updateConnection;

void logSink(sp::LogLevel level, const char *message)
{
	int obsLevel = LOG_INFO;
	switch (level) {
	case sp::LogLevel::Debug:
		obsLevel = LOG_DEBUG;
		break;
	case sp::LogLevel::Info:
		obsLevel = LOG_INFO;
		break;
	case sp::LogLevel::Warning:
		obsLevel = LOG_WARNING;
		break;
	case sp::LogLevel::Error:
		obsLevel = LOG_ERROR;
		break;
	}
	obs_log(obsLevel, "%s", message);
}

const char *textProvider(const char *key)
{
	return obs_module_text(key);
}

void showEditor(const QString &panelId)
{
	if (!g_editor)
		return;
	if (!panelId.isEmpty())
		g_editor->showPanel(panelId);
	if (auto *dock = qobject_cast<QDockWidget *>(g_editor->parentWidget())) {
		dock->setVisible(true);
		dock->raise();
	}
}

void onFrontendEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_EXIT) {
		// Last chance to persist everything while Qt objects are still alive.
		if (g_editor)
			g_editor->flushDraftsQuietly();
		sp::PanelManager::instance().save();
	}
}

} // namespace

bool obs_module_load(void)
{
	sp::setLogSink(&logSink);
	sp::setTextProvider(&textProvider);
	sp::PanelManager::instance(); // create on the UI thread, before any other thread can touch it

	// ---- storage: <config>/obs-studio/plugin_config/stream-panels/
	char *configDir = obs_module_config_path("");
	if (configDir) {
		os_mkdirs(configDir);
		const QString dir = QString::fromUtf8(configDir);
		sp::LogoStore::setBaseDir(dir);
		sp::PanelManager::instance().setStoragePath(sp::LogoStore::baseDir() + QStringLiteral("/panels.json"));
		bfree(configDir);
	}
	sp::PanelManager::instance().load();

	// ---- the OBS source
	sp::obsapi::registerPanelSource();
	sp::obsapi::setOpenEditorCallback(&showEditor);

	// Library commits -> refresh every linked source.
	g_updateConnection = QObject::connect(&sp::PanelManager::instance(), &sp::PanelManager::panelUpdated,
					      &sp::PanelManager::instance(),
					      [](const QString &id) { sp::obsapi::refreshLinkedSources(id); });

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_post_load(void)
{
	// All modules are loaded and the main window exists: add the dock.
	if (!obs_frontend_get_main_window())
		return;

	auto *editor = new sp::PanelEditor(&g_bridge);
	g_editor = editor;

	// OBS wraps the widget in a QDockWidget and takes ownership of it.
	if (!obs_frontend_add_dock_by_id(kDockId, obs_module_text("Dock.Title"), editor)) {
		obs_log(LOG_WARNING, "could not add the Stream Panels dock");
		delete editor;
		g_editor = nullptr;
		return;
	}
	obs_frontend_add_event_callback(onFrontendEvent, nullptr);
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
	QObject::disconnect(g_updateConnection);
	sp::obsapi::setOpenEditorCallback(nullptr);

	// The dock/editor belong to OBS' main window and are destroyed by it; never
	// delete them here. Flush and release the library while Qt is still alive.
	sp::PanelManager::shutdown();
	obs_log(LOG_INFO, "plugin unloaded");
}
