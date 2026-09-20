#include "render/logo-source.hpp"

#include "common/platform.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QImageReader>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QTransform>

#ifdef SP_HAVE_QTSVG
#include <QSvgRenderer>
#endif

#include <algorithm>
#include <cmath>

namespace sp {

struct LogoSource::Data {
	QImage raster;
#ifdef SP_HAVE_QTSVG
	std::shared_ptr<QSvgRenderer> svg;
#endif
	QSizeF nativeSize;
};

namespace {

struct CacheEntry {
	qint64 mtime = 0;
	qint64 bytes = 0;
	std::shared_ptr<const LogoSource::Data> data; // null = failed to load (negative cache)
};

QMutex g_cacheMutex;
QMap<QString, CacheEntry> g_cache;
constexpr int kMaxCacheEntries = 24;
constexpr int kMaxRasterSide = 4096; // larger logos are downscaled on load

} // namespace

LogoSource LogoSource::load(const QString &absolutePath)
{
	LogoSource result;
	if (absolutePath.isEmpty())
		return result;

	const QFileInfo fi(absolutePath);
	if (!fi.exists() || !fi.isFile())
		return result;

	const qint64 mtime = fi.lastModified().toMSecsSinceEpoch();
	const qint64 bytes = fi.size();

	{
		QMutexLocker lock(&g_cacheMutex);
		const auto it = g_cache.constFind(absolutePath);
		if (it != g_cache.constEnd() && it->mtime == mtime && it->bytes == bytes) {
			result.m_data = it->data;
			return result;
		}
	}

	auto data = std::make_shared<Data>();
	bool ok = false;

	const QString suffix = fi.suffix().toLower();
	if (suffix == QLatin1String("svg") || suffix == QLatin1String("svgz")) {
#ifdef SP_HAVE_QTSVG
		auto renderer = std::make_shared<QSvgRenderer>(absolutePath);
		if (renderer->isValid()) {
			QSizeF sz = renderer->defaultSize();
			if (sz.width() <= 0 || sz.height() <= 0)
				sz = renderer->viewBoxF().size();
			if (sz.width() > 0 && sz.height() > 0) {
				data->svg = renderer;
				data->nativeSize = sz;
				ok = true;
			}
		}
#else
		log(LogLevel::Warning, QStringLiteral("SVG logos are not supported in this build"));
#endif
	} else {
		QImageReader reader(absolutePath);
		reader.setAutoTransform(true);
		const QSize native = reader.size();
		if (native.isValid() && std::max(native.width(), native.height()) > kMaxRasterSide) {
			QSize scaled = native;
			scaled.scale(kMaxRasterSide, kMaxRasterSide, Qt::KeepAspectRatio);
			reader.setScaledSize(scaled);
		}
		QImage img = reader.read();
		if (!img.isNull()) {
			data->raster = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
			data->nativeSize = QSizeF(data->raster.size());
			ok = !data->raster.isNull();
		} else {
			log(LogLevel::Warning,
			    QStringLiteral("Cannot read logo '%1': %2").arg(absolutePath, reader.errorString()));
		}
	}

	{
		QMutexLocker lock(&g_cacheMutex);
		if (g_cache.size() >= kMaxCacheEntries)
			g_cache.clear();
		CacheEntry entry;
		entry.mtime = mtime;
		entry.bytes = bytes;
		entry.data = ok ? std::shared_ptr<const Data>(data) : nullptr;
		g_cache.insert(absolutePath, entry);
	}

	if (ok)
		result.m_data = data;
	return result;
}

QSizeF LogoSource::size() const
{
	return m_data ? m_data->nativeSize : QSizeF();
}

qreal LogoSource::aspect() const
{
	if (!m_data || m_data->nativeSize.height() <= 0)
		return 1.0;
	return m_data->nativeSize.width() / m_data->nativeSize.height();
}

QRectF LogoSource::fitRect(const QRectF &box, qreal scale, Qt::Alignment align) const
{
	if (!m_data || box.width() <= 0 || box.height() <= 0)
		return QRectF(box.topLeft(), QSizeF(0, 0));

	QSizeF s = m_data->nativeSize;
	s.scale(box.size(), Qt::KeepAspectRatio);
	s *= std::clamp<qreal>(scale, 0.05, 2.0);
	// Never allow the logo to spill out of its box, even when scale > 1.
	s.setWidth(std::min(s.width(), box.width()));
	s.setHeight(std::min(s.height(), box.height()));
	// Re-apply aspect after clamping.
	QSizeF fitted = m_data->nativeSize;
	fitted.scale(s, Qt::KeepAspectRatio);

	qreal x = box.left() + (box.width() - fitted.width()) / 2.0;
	if (align & Qt::AlignLeft)
		x = box.left();
	else if (align & Qt::AlignRight)
		x = box.right() - fitted.width();
	const qreal y = box.top() + (box.height() - fitted.height()) / 2.0;
	return QRectF(QPointF(x, y), fitted);
}

void LogoSource::paint(QPainter &painter, const QRectF &target) const
{
	if (!m_data || target.width() <= 0 || target.height() <= 0)
		return;

	painter.save();
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	painter.setRenderHint(QPainter::Antialiasing, true);

#ifdef SP_HAVE_QTSVG
	if (m_data->svg) {
		m_data->svg->render(&painter, target);
		painter.restore();
		return;
	}
#endif

	// Raster: pre-scale with a high quality filter when shrinking, otherwise
	// bilinear sampling produces visible aliasing on big logos.
	const QTransform t = painter.transform();
	const qreal sx = std::hypot(t.m11(), t.m12());
	const qreal sy = std::hypot(t.m21(), t.m22());
	const QSize devSize(std::max(1, int(std::lround(target.width() * sx))),
			    std::max(1, int(std::lround(target.height() * sy))));

	if (devSize.width() < m_data->raster.width() || devSize.height() < m_data->raster.height()) {
		const QImage scaled = m_data->raster.scaled(devSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		painter.drawImage(target, scaled);
	} else {
		painter.drawImage(target, m_data->raster);
	}
	painter.restore();
}

} // namespace sp
