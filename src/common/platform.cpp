#include "common/platform.hpp"

#include <cstdio>

namespace sp {

namespace {

LogSink g_logSink = nullptr;
TextProvider g_textProvider = nullptr;

const char *levelName(LogLevel level)
{
	switch (level) {
	case LogLevel::Debug:
		return "debug";
	case LogLevel::Info:
		return "info";
	case LogLevel::Warning:
		return "warning";
	case LogLevel::Error:
		return "error";
	}
	return "?";
}

} // namespace

void setLogSink(LogSink sink)
{
	g_logSink = sink;
}

void setTextProvider(TextProvider provider)
{
	g_textProvider = provider;
}

void log(LogLevel level, const QString &message)
{
	const QByteArray utf8 = message.toUtf8();
	if (g_logSink) {
		g_logSink(level, utf8.constData());
		return;
	}
	std::fprintf(stderr, "[stream-panels] %s: %s\n", levelName(level), utf8.constData());
}

QString T(const char *key)
{
	if (g_textProvider) {
		if (const char *text = g_textProvider(key))
			return QString::fromUtf8(text);
	}
	return QString::fromUtf8(key);
}

} // namespace sp
