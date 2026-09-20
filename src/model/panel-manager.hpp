// Stream Panels - panel library.
//
// PanelManager owns the list of panels ("Mis paneles") and persists it to a
// JSON file inside the plugin config directory. It is the single source of
// truth that both the dock UI and every OBS source read from.
//
// Threading: all getters are thread-safe (OBS may create/update sources from
// non-UI threads). Mutating calls must come from the UI thread, except
// importIfMissing() which may be called from anywhere.
#pragma once

#include "model/panel-model.hpp"

#include <QMutex>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>

#include <optional>

namespace sp {

class PanelManager : public QObject {
	Q_OBJECT

public:
	// Created on first use (must happen on the UI thread, before any other thread
	// touches it - plugin-main does it in obs_module_load) and destroyed by
	// shutdown() while Qt is still alive, so nothing static outlives QApplication.
	static PanelManager &instance();
	static void shutdown(); // stops timers, saves, deletes the instance

	// ---- persistence
	void setStoragePath(const QString &jsonFile);
	QString storagePath() const;
	bool load();
	bool save();
	void scheduleSave(); // debounced (UI thread)

	// ---- queries (thread-safe, return copies)
	QVector<PanelModel> panels() const;
	bool contains(const QString &id) const;
	std::optional<PanelModel> panel(const QString &id) const;
	int count() const;

	QString lastSelectedId() const;
	void setLastSelectedId(const QString &id);

	// ---- mutations (UI thread)
	void add(PanelModel model);
	// Replaces the stored panel with the same id. When `notify` is true
	// panelUpdated() fires so that live OBS sources refresh.
	void update(PanelModel model, bool notify = true);
	void remove(const QString &id);
	QString duplicate(const QString &id);
	bool rename(const QString &id, const QString &name);

	// Thread-safe: adds the panel only if its id is unknown (used when a scene
	// collection references a panel that does not exist in this machine's library).
	bool importIfMissing(PanelModel model);

signals:
	void libraryChanged();                // added / removed / renamed / reordered
	void panelUpdated(const QString &id); // content committed
	void panelRemoved(const QString &id);

private:
	PanelManager();
	int indexOf(const QString &id) const; // requires lock

	mutable QMutex m_mutex;
	QVector<PanelModel> m_panels;
	QString m_file;
	QString m_lastSelected;
	QTimer m_saveTimer;
};

} // namespace sp
