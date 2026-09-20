// Stream Panels - logo file management.
//
// Logos picked by the operator are *imported* (copied) into
// <plugin config dir>/logos/<sha1>.<ext>. Panels then store a path relative to
// the config dir, so they keep working if the original file is moved or
// deleted, and the panel library stays portable between machines.
//
// If the import is not possible (unwritable config dir, huge file...) the
// absolute path of the original file is stored instead. Both forms are
// understood by resolve().
#pragma once

#include <QString>

namespace sp {

class LogoStore {
public:
	// Base directory of the plugin configuration (…/plugin_config/stream-panels).
	static void setBaseDir(const QString &dir);
	static QString baseDir();

	// Stored path -> absolute path on disk. Empty if the input is empty or unsafe.
	static QString resolve(const QString &storedPath);

	// Copies `sourceFile` into the store. Returns the path to persist in the
	// panel (relative when imported, absolute as a fallback). Empty on failure.
	static QString importFile(const QString &sourceFile);

	// File dialog filter, e.g. "Images (*.png *.jpg ...)".
	static QString dialogFilter();
	static bool svgSupported();
};

} // namespace sp
