// Stream Panels - data model.
//
// PanelModel is a plain value type describing *everything* needed to render a
// panel. It knows nothing about OBS, Qt widgets or rendering; it can be copied
// freely between threads and is serialized to/from JSON.
#pragma once

#include <QColor>
#include <QJsonObject>
#include <QString>

namespace sp {

enum class WidthMode { FitText, Custom };
enum class HeightMode { Auto, Custom };
enum class LogoAlign { Left, Center, Right };

// Reserved for the animation system (not rendered by the MVP, but persisted so
// files written today stay valid when animations are implemented).
enum class AnimationKind { None, SlideFromLeft, SlideFromRight, Fade };

struct FontSpec {
	QString family; // empty = system UI font
	int pixelSize = 32;
	bool bold = false;
	bool italic = false;

	bool operator==(const FontSpec &o) const
	{
		return family == o.family && pixelSize == o.pixelSize && bold == o.bold && italic == o.italic;
	}
	bool operator!=(const FontSpec &o) const { return !(*this == o); }

	QJsonObject toJson() const;
	static FontSpec fromJson(const QJsonObject &obj, const FontSpec &fallback);
};

struct PanelPalette {
	QColor primary;    // main accent / title strip
	QColor secondary;  // dividers, chips, highlights
	QColor title;      // title text
	QColor text;       // main text
	QColor background; // body background
	QColor logoBlock;  // block behind the logo

	bool operator==(const PanelPalette &o) const
	{
		return primary == o.primary && secondary == o.secondary && title == o.title && text == o.text &&
		       background == o.background && logoBlock == o.logoBlock;
	}
	bool operator!=(const PanelPalette &o) const { return !(*this == o); }

	QJsonObject toJson() const;
	static PanelPalette fromJson(const QJsonObject &obj, const PanelPalette &fallback);
};

struct AnimationSpec {
	AnimationKind kind = AnimationKind::None;
	int durationMs = 500;
};

// Hard limits used by sanitize() and by the UI spin boxes.
namespace limits {
constexpr int kMinDimension = 24;
constexpr int kMaxDimension = 4096;
constexpr int kMinFontPx = 6;
constexpr int kMaxFontPx = 400;
constexpr int kMaxPadding = 200;
} // namespace limits

struct PanelModel {
	static constexpr int kSchemaVersion = 1;

	// identity
	QString id;   // stable, generated
	QString name; // user-visible name in the panel list (may be empty -> falls back to title)

	// content
	QString templateId = QStringLiteral("classic");
	QString title;
	QString text;
	bool showTitle = true;
	bool showText = true;
	bool showLogo = true;
	bool titleUppercase = true;

	// logo (path is either absolute or relative to the plugin config dir, see LogoStore)
	QString logoPath;
	int logoScalePercent = 100; // 10..200, relative to the space the template gives the logo
	int logoPadding = 8;        // px, inner margin of the logo box
	LogoAlign logoAlign = LogoAlign::Center;

	// sizing
	WidthMode widthMode = WidthMode::FitText;
	int customWidth = 700;
	int minWidth = 240;
	int maxWidth = 1600;
	HeightMode heightMode = HeightMode::Auto;
	int customHeight = 140;
	int padding = -1; // -1 = template default

	// style
	bool useTemplateColors = true;
	PanelPalette colors;
	bool useTemplateFonts = true;
	FontSpec titleFont;
	FontSpec textFont;

	// reserved
	AnimationSpec animationIn;
	AnimationSpec animationOut;

	QString displayName() const;
	QString subtitle() const;

	// Clamps every field into a range the renderer can safely handle.
	void sanitize();

	QJsonObject toJson() const;
	// Never fails: missing/invalid fields fall back to defaults. The result is sanitized.
	static PanelModel fromJson(const QJsonObject &obj);

	bool operator==(const PanelModel &o) const { return toJson() == o.toJson(); }
	bool operator!=(const PanelModel &o) const { return !(*this == o); }

	static QString newId();
};

} // namespace sp
