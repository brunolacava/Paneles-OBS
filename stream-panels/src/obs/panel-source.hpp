// Stream Panels - the OBS source ("Stream Panel").
//
// This is the OBS integration layer for rendering. It does not draw anything
// itself: it asks PanelRenderer for an image whenever the panel changes,
// uploads it as a gs_texture_t and draws that texture in video_render().
// The output is therefore a real OBS video source (Preview, Program,
// recording, streaming, filters, transforms...), not a captured Qt window.
#pragma once

#include <QString>

#include <functional>

namespace sp::obsapi {

extern const char *const kSourceId;

// Registers the source type with OBS. Call once from obs_module_load().
void registerPanelSource();

// Called by the properties dialog's "Edit panel" button.
void setOpenEditorCallback(std::function<void(const QString &panelId)> callback);

// ---- helpers used by the dock (through ObsSceneBridge) and by plugin-main
void refreshLinkedSources(const QString &panelId); // push the library version to every linked source
int countLinkedSources(const QString &panelId);
void unlinkSources(const QString &panelId);        // sources keep their current look

} // namespace sp::obsapi
