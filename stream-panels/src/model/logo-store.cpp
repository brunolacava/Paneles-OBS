#include "model/logo-store.hpp"

#include "common/platform.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace sp {

namespace {
QString g_baseDir;
constexpr qint64 kMaxImportBytes = 64ll * 1024 * 1024;
} // namespace

void LogoStore::setBaseDir(const QString &dir)
{
	g_baseDir = dir.isEmpty() ? QString() : QDir::cleanPath(dir);
}

QString LogoStore::baseDir()
{
	return g_baseDir;
}

bool LogoStore::svgSupported()
{
#ifdef SP_HAVE_QTSVG
	return true;
#else
	return false;
#endif
}

QString LogoStore::dialogFilter()
{
	QString exts = QStringLiteral("*.png *.jpg *.jpeg *.bmp *.webp");
	if (svgSupported())
		exts += QStringLiteral(" *.svg");
	return T("Logo.FileFilter") + QStringLiteral(" (") + exts + QStringLiteral(");;") + T("Logo.AllFiles") +
	       QStringLiteral(" (*)");
}

QString LogoStore::resolve(const QString &storedPath)
{
	const QString p = storedPath.trimmed();
	if (p.isEmpty())
		return QString();

	const QFileInfo fi(p);
	if (fi.isAbsolute())
		return QDir::cleanPath(p);

	if (g_baseDir.isEmpty())
		return QString();

	const QString abs = QDir::cleanPath(g_baseDir + QLatin1Char('/') + p);
	// Refuse anything that escapes the config directory ("../../secret.png").
	if (!abs.startsWith(g_baseDir + QLatin1Char('/')))
		return QString();
	return abs;
}

QString LogoStore::importFile(const QString &sourceFile)
{
	const QFileInfo src(sourceFile);
	if (!src.exists() || !src.isFile()) {
		log(LogLevel::Warning, QStringLiteral("Logo not found: %1").arg(sourceFile));
		return QString();
	}
	const QString absolute = QDir::cleanPath(src.absoluteFilePath());

	if (g_baseDir.isEmpty() || src.size() > kMaxImportBytes)
		return absolute;

	QFile in(absolute);
	if (!in.open(QIODevice::ReadOnly))
		return absolute;
	const QByteArray data = in.readAll();
	in.close();
	if (data.isEmpty())
		return absolute;

	const QString hash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
	const QString ext = src.suffix().toLower();
	const QString relative = QStringLiteral("logos/") + hash + (ext.isEmpty() ? QString() : QLatin1Char('.') + ext);
	const QString dest = g_baseDir + QLatin1Char('/') + relative;

	if (QFileInfo::exists(dest))
		return relative;

	if (!QDir().mkpath(g_baseDir + QStringLiteral("/logos"))) {
		log(LogLevel::Warning, QStringLiteral("Cannot create logos directory, using original path"));
		return absolute;
	}

	QSaveFile out(dest);
	if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size() || !out.commit()) {
		log(LogLevel::Warning, QStringLiteral("Cannot copy logo into the plugin config, using original path"));
		return absolute;
	}
	return relative;
}

} // namespace sp
