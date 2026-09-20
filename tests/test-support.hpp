// Helpers shared by the test executables: loads data/locale/en-US.ini so the
// UI shows real strings instead of keys.
#pragma once

#include "common/platform.hpp"

#include <QByteArray>
#include <QFile>
#include <QHash>
#include <QString>

namespace testsupport {

inline QHash<QByteArray, QByteArray> &table()
{
	static QHash<QByteArray, QByteArray> t;
	return t;
}

inline const char *lookup(const char *key)
{
	const auto it = table().constFind(QByteArray(key));
	return it == table().constEnd() ? key : it->constData();
}

inline void loadLocale(const QString &lang = QStringLiteral("en-US"))
{
	QFile f(QStringLiteral(SP_LOCALE_DIR "/") + lang + QStringLiteral(".ini"));
	if (!f.open(QIODevice::ReadOnly))
		return;
	while (!f.atEnd()) {
		const QByteArray line = f.readLine().trimmed();
		const int eq = line.indexOf('=');
		if (eq <= 0 || line.startsWith(';') || line.startsWith('#'))
			continue;
		QByteArray key = line.left(eq).trimmed();
		QByteArray val = line.mid(eq + 1).trimmed();
		if (val.size() >= 2 && val.startsWith('"') && val.endsWith('"'))
			val = val.mid(1, val.size() - 2);
		val.replace("\\\"", "\"");
		val.replace("\\n", "\n");
		table().insert(key, val);
	}
	sp::setTextProvider(&lookup);
}

} // namespace testsupport
