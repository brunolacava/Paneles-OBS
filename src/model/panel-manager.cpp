#include "model/panel-manager.hpp"

#include "common/platform.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaObject>
#include <QMutexLocker>
#include <QSaveFile>

namespace sp {

namespace {
PanelManager *g_instance = nullptr;
}

PanelManager &PanelManager::instance()
{
	if (!g_instance)
		g_instance = new PanelManager();
	return *g_instance;
}

void PanelManager::shutdown()
{
	if (!g_instance)
		return;
	g_instance->m_saveTimer.stop();
	g_instance->save();
	delete g_instance;
	g_instance = nullptr;
}

PanelManager::PanelManager()
{
	m_saveTimer.setSingleShot(true);
	m_saveTimer.setInterval(500);
	connect(&m_saveTimer, &QTimer::timeout, this, [this]() { save(); });
}

void PanelManager::setStoragePath(const QString &jsonFile)
{
	QMutexLocker lock(&m_mutex);
	m_file = jsonFile;
}

QString PanelManager::storagePath() const
{
	QMutexLocker lock(&m_mutex);
	return m_file;
}

bool PanelManager::load()
{
	QString file;
	{
		QMutexLocker lock(&m_mutex);
		file = m_file;
	}
	if (file.isEmpty())
		return false;

	QFile f(file);
	if (!f.exists())
		return true; // first run: empty library is fine

	if (!f.open(QIODevice::ReadOnly)) {
		log(LogLevel::Error, QStringLiteral("Cannot open %1: %2").arg(file, f.errorString()));
		return false;
	}
	const QByteArray data = f.readAll();
	f.close();

	QJsonParseError err;
	const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject()) {
		// Keep the broken file around instead of silently overwriting it on the next save.
		const QString backup =
			file + QStringLiteral(".corrupt-") + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
		QFile::rename(file, backup);
		log(LogLevel::Error,
		    QStringLiteral("Panel library is corrupt (%1). Moved to %2").arg(err.errorString(), backup));
		return false;
	}

	const QJsonObject root = doc.object();
	QVector<PanelModel> loaded;
	const QJsonArray arr = root.value(QStringLiteral("panels")).toArray();
	for (const QJsonValue &v : arr) {
		if (!v.isObject())
			continue;
		PanelModel m = PanelModel::fromJson(v.toObject());
		bool duplicateId = false;
		for (const PanelModel &existing : loaded)
			duplicateId |= (existing.id == m.id);
		if (duplicateId)
			m.id = PanelModel::newId();
		loaded.push_back(std::move(m));
	}

	{
		QMutexLocker lock(&m_mutex);
		m_panels = std::move(loaded);
		m_lastSelected = root.value(QStringLiteral("lastSelected")).toString();
	}
	log(LogLevel::Info, QStringLiteral("Loaded %1 panel(s)").arg(arr.size()));
	return true;
}

bool PanelManager::save()
{
	QString file;
	QJsonArray arr;
	QString last;
	{
		QMutexLocker lock(&m_mutex);
		file = m_file;
		last = m_lastSelected;
		for (const PanelModel &m : m_panels)
			arr.push_back(m.toJson());
	}
	if (file.isEmpty())
		return false;

	QJsonObject root;
	root.insert(QStringLiteral("version"), PanelModel::kSchemaVersion);
	root.insert(QStringLiteral("lastSelected"), last);
	root.insert(QStringLiteral("panels"), arr);

	QDir().mkpath(QFileInfo(file).absolutePath());

	// QSaveFile writes to a temporary file and renames on commit: a crash while
	// saving can never leave a half-written library behind.
	QSaveFile out(file);
	if (!out.open(QIODevice::WriteOnly)) {
		log(LogLevel::Error, QStringLiteral("Cannot write %1: %2").arg(file, out.errorString()));
		return false;
	}
	out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	if (!out.commit()) {
		log(LogLevel::Error, QStringLiteral("Cannot commit %1: %2").arg(file, out.errorString()));
		return false;
	}
	return true;
}

void PanelManager::scheduleSave()
{
	m_saveTimer.start();
}

int PanelManager::indexOf(const QString &id) const
{
	for (int i = 0; i < m_panels.size(); ++i)
		if (m_panels[i].id == id)
			return i;
	return -1;
}

QVector<PanelModel> PanelManager::panels() const
{
	QMutexLocker lock(&m_mutex);
	return m_panels;
}

bool PanelManager::contains(const QString &id) const
{
	QMutexLocker lock(&m_mutex);
	return indexOf(id) >= 0;
}

std::optional<PanelModel> PanelManager::panel(const QString &id) const
{
	QMutexLocker lock(&m_mutex);
	const int i = indexOf(id);
	if (i < 0)
		return std::nullopt;
	return m_panels[i];
}

int PanelManager::count() const
{
	QMutexLocker lock(&m_mutex);
	return m_panels.size();
}

QString PanelManager::lastSelectedId() const
{
	QMutexLocker lock(&m_mutex);
	return m_lastSelected;
}

void PanelManager::setLastSelectedId(const QString &id)
{
	{
		QMutexLocker lock(&m_mutex);
		if (m_lastSelected == id)
			return;
		m_lastSelected = id;
	}
	scheduleSave();
}

void PanelManager::add(PanelModel model)
{
	model.sanitize();
	{
		QMutexLocker lock(&m_mutex);
		if (indexOf(model.id) >= 0)
			model.id = PanelModel::newId();
		m_panels.push_back(std::move(model));
	}
	emit libraryChanged();
	scheduleSave();
}

void PanelManager::update(PanelModel model, bool notify)
{
	model.sanitize();
	const QString id = model.id;
	{
		QMutexLocker lock(&m_mutex);
		const int i = indexOf(id);
		if (i < 0) {
			m_panels.push_back(std::move(model));
		} else {
			m_panels[i] = std::move(model);
		}
	}
	if (notify)
		emit panelUpdated(id);
	scheduleSave();
}

void PanelManager::remove(const QString &id)
{
	{
		QMutexLocker lock(&m_mutex);
		const int i = indexOf(id);
		if (i < 0)
			return;
		m_panels.remove(i);
		if (m_lastSelected == id)
			m_lastSelected.clear();
	}
	emit panelRemoved(id);
	emit libraryChanged();
	scheduleSave();
}

QString PanelManager::duplicate(const QString &id)
{
	PanelModel copy;
	{
		QMutexLocker lock(&m_mutex);
		const int i = indexOf(id);
		if (i < 0)
			return QString();
		copy = m_panels[i];
	}
	copy.id = PanelModel::newId();
	const QString base = copy.displayName();
	copy.name = base + QStringLiteral(" ") + T("Panel.CopySuffix");
	const QString newId = copy.id;
	add(std::move(copy));
	return newId;
}

bool PanelManager::rename(const QString &id, const QString &name)
{
	{
		QMutexLocker lock(&m_mutex);
		const int i = indexOf(id);
		if (i < 0)
			return false;
		m_panels[i].name = name.trimmed();
	}
	emit libraryChanged();
	scheduleSave();
	return true;
}

bool PanelManager::importIfMissing(PanelModel model)
{
	model.sanitize();
	{
		QMutexLocker lock(&m_mutex);
		if (indexOf(model.id) >= 0)
			return false;
		m_panels.push_back(std::move(model));
	}
	// May run on any thread: hop to the manager's (UI) thread for signals/timers.
	QMetaObject::invokeMethod(
		this,
		[this]() {
			emit libraryChanged();
			scheduleSave();
		},
		Qt::QueuedConnection);
	return true;
}

} // namespace sp
