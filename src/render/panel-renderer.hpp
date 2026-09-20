// Stream Panels - CPU renderer.
//
// Turns a PanelModel into a premultiplied RGBA image using the model's
// template. This is the *only* code path that produces pixels: the dock
// preview and the OBS source both call it, so what the operator sees in the
// editor is exactly what goes on air.
//
// Must be called from a thread that can use Qt font rendering (the UI thread).
#pragma once

#include "model/panel-model.hpp"

#include <QImage>
#include <QSizeF>

namespace sp {

struct RenderResult {
	QImage image;       // Format_RGBA8888_Premultiplied, transparent background. Null on failure.
	QSize logicalSize;  // size of the panel in panel pixels (image size / scale)
	qreal scale = 1.0;  // supersampling factor actually used

	bool ok() const { return !image.isNull(); }
};

class PanelRenderer {
public:
	// `scale` multiplies the resolution of the produced image (2.0 = crisp on
	// HiDPI / when the operator scales the source up in OBS). The logical size
	// is independent of `scale`.
	static RenderResult render(const PanelModel &model, qreal scale = 1.0);

	// Layout only (no pixels): handy for tests and for the "width: 700 px" label.
	static QSize measure(const PanelModel &model);
};

} // namespace sp
