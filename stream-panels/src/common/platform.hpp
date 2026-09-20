// Stream Panels - platform glue.
//
// The core of the plugin (model, rendering, templates, UI) is deliberately
// independent from libobs. The only two things it needs from the host are
// logging and localized strings, which are injected through the small hooks
// declared here. plugin-main.cpp wires them to obs_log() / obs_module_text();
// the standalone test harness wires them to stderr / an .ini reader.
#pragma once

#include <QString>

namespace sp {

enum class LogLevel { Debug, Info, Warning, Error };

using LogSink = void (*)(LogLevel level, const char *utf8Message);
using TextProvider = const char *(*)(const char *key);

void setLogSink(LogSink sink);
void setTextProvider(TextProvider provider);

void log(LogLevel level, const QString &message);

// Localized UI string. Returns the key itself when no translation exists
// (which is also what obs_module_text() does).
QString T(const char *key);

} // namespace sp
