#include "model/panel-model.hpp"

#include <QUuid>

#include <algorithm>

namespace sp {

namespace {

QString colorToString(const QColor &c)
{
	return c.isValid() ? c.name(QColor::HexArgb) : QString();
}

QColor colorFromJson(const QJsonValue &v, const QColor &fallback)
{
	if (!v.isString())
		return fallback;
	const QColor c(v.toString());
	return c.isValid() ? c : fallback;
}

int intFrom(const QJsonObject &o, const char *key, int def)
{
	const QJsonValue v = o.value(QLatin1String(key));
	return v.isDouble() ? v.toInt(def) : def;
}

bool boolFrom(const QJsonObject &o, const char *key, bool def)
{
	const QJsonValue v = o.value(QLatin1String(key));
	return v.isBool() ? v.toBool(def) : def;
}

QString stringFrom(const QJsonObject &o, const char *key, const QString &def = QString())
{
	const QJsonValue v = o.value(QLatin1String(key));
	return v.isString() ? v.toString() : def;
}

const char *widthModeName(WidthMode m)
{
	return m == WidthMode::Custom ? "custom" : "fit";
}
const char *heightModeName(HeightMode m)
{
	return m == HeightMode::Custom ? "custom" : "auto";
}
const char *logoAlignName(LogoAlign a)
{
	switch (a) {
	case LogoAlign::Left:
		return "left";
	case LogoAlign::Right:
		return "right";
	default:
		return "center";
	}
}
const char *animationName(AnimationKind k)
{
	switch (k) {
	case AnimationKind::SlideFromLeft:
		return "slide-left";
	case AnimationKind::SlideFromRight:
		return "slide-right";
	case AnimationKind::Fade:
		return "fade";
	default:
		return "none";
	}
}
AnimationKind animationFromName(const QString &s)
{
	if (s == QLatin1String("slide-left"))
		return AnimationKind::SlideFromLeft;
	if (s == QLatin1String("slide-right"))
		return AnimationKind::SlideFromRight;
	if (s == QLatin1String("fade"))
		return AnimationKind::Fade;
	return AnimationKind::None;
}

QJsonObject animationToJson(const AnimationSpec &a)
{
	QJsonObject o;
	o.insert(QStringLiteral("kind"), QLatin1String(animationName(a.kind)));
	o.insert(QStringLiteral("durationMs"), a.durationMs);
	return o;
}

AnimationSpec animationFromJson(const QJsonObject &o)
{
	AnimationSpec a;
	a.kind = animationFromName(stringFrom(o, "kind"));
	a.durationMs = std::clamp(intFrom(o, "durationMs", 500), 0, 10000);
	return a;
}

} // namespace

// ---------------------------------------------------------------- FontSpec

QJsonObject FontSpec::toJson() const
{
	QJsonObject o;
	o.insert(QStringLiteral("family"), family);
	o.insert(QStringLiteral("pixelSize"), pixelSize);
	o.insert(QStringLiteral("bold"), bold);
	o.insert(QStringLiteral("italic"), italic);
	return o;
}

FontSpec FontSpec::fromJson(const QJsonObject &obj, const FontSpec &fallback)
{
	FontSpec f = fallback;
	f.family = stringFrom(obj, "family", fallback.family);
	f.pixelSize = std::clamp(intFrom(obj, "pixelSize", fallback.pixelSize), limits::kMinFontPx, limits::kMaxFontPx);
	f.bold = boolFrom(obj, "bold", fallback.bold);
	f.italic = boolFrom(obj, "italic", fallback.italic);
	return f;
}

// ------------------------------------------------------------- PanelPalette

QJsonObject PanelPalette::toJson() const
{
	QJsonObject o;
	o.insert(QStringLiteral("primary"), colorToString(primary));
	o.insert(QStringLiteral("secondary"), colorToString(secondary));
	o.insert(QStringLiteral("title"), colorToString(title));
	o.insert(QStringLiteral("text"), colorToString(text));
	o.insert(QStringLiteral("background"), colorToString(background));
	o.insert(QStringLiteral("logoBlock"), colorToString(logoBlock));
	return o;
}

PanelPalette PanelPalette::fromJson(const QJsonObject &obj, const PanelPalette &fallback)
{
	PanelPalette p;
	p.primary = colorFromJson(obj.value(QStringLiteral("primary")), fallback.primary);
	p.secondary = colorFromJson(obj.value(QStringLiteral("secondary")), fallback.secondary);
	p.title = colorFromJson(obj.value(QStringLiteral("title")), fallback.title);
	p.text = colorFromJson(obj.value(QStringLiteral("text")), fallback.text);
	p.background = colorFromJson(obj.value(QStringLiteral("background")), fallback.background);
	p.logoBlock = colorFromJson(obj.value(QStringLiteral("logoBlock")), fallback.logoBlock);
	return p;
}

// -------------------------------------------------------------- PanelModel

QString PanelModel::newId()
{
	return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString PanelModel::displayName() const
{
	if (!name.trimmed().isEmpty())
		return name.trimmed();
	const QString firstLine = title.trimmed().section(QLatin1Char('\n'), 0, 0);
	if (!firstLine.isEmpty())
		return firstLine;
	const QString textLine = text.trimmed().section(QLatin1Char('\n'), 0, 0);
	return textLine;
}

QString PanelModel::subtitle() const
{
	// The list shows "name" and, below it, the most informative other string.
	const QString t = title.trimmed().section(QLatin1Char('\n'), 0, 0);
	const QString x = text.trimmed().section(QLatin1Char('\n'), 0, 0);
	if (!name.trimmed().isEmpty())
		return x.isEmpty() ? t : x;
	return x;
}

void PanelModel::sanitize()
{
	using namespace limits;
	if (id.isEmpty())
		id = newId();
	if (templateId.isEmpty())
		templateId = QStringLiteral("classic");

	logoScalePercent = std::clamp(logoScalePercent, 10, 200);
	logoPadding = std::clamp(logoPadding, 0, 100);

	customWidth = std::clamp(customWidth, kMinDimension, kMaxDimension);
	customHeight = std::clamp(customHeight, kMinDimension, kMaxDimension);
	minWidth = std::clamp(minWidth, kMinDimension, kMaxDimension);
	maxWidth = std::clamp(maxWidth, kMinDimension, kMaxDimension);
	if (maxWidth < minWidth)
		maxWidth = minWidth;
	padding = std::clamp(padding, -1, kMaxPadding);

	titleFont.pixelSize = std::clamp(titleFont.pixelSize, kMinFontPx, kMaxFontPx);
	textFont.pixelSize = std::clamp(textFont.pixelSize, kMinFontPx, kMaxFontPx);

	animationIn.durationMs = std::clamp(animationIn.durationMs, 0, 10000);
	animationOut.durationMs = std::clamp(animationOut.durationMs, 0, 10000);
}

QJsonObject PanelModel::toJson() const
{
	QJsonObject o;
	o.insert(QStringLiteral("schema"), kSchemaVersion);
	o.insert(QStringLiteral("id"), id);
	o.insert(QStringLiteral("name"), name);
	o.insert(QStringLiteral("template"), templateId);
	o.insert(QStringLiteral("title"), title);
	o.insert(QStringLiteral("text"), text);
	o.insert(QStringLiteral("showTitle"), showTitle);
	o.insert(QStringLiteral("showText"), showText);
	o.insert(QStringLiteral("showLogo"), showLogo);
	o.insert(QStringLiteral("titleUppercase"), titleUppercase);

	o.insert(QStringLiteral("logoPath"), logoPath);
	o.insert(QStringLiteral("logoScalePercent"), logoScalePercent);
	o.insert(QStringLiteral("logoPadding"), logoPadding);
	o.insert(QStringLiteral("logoAlign"), QLatin1String(logoAlignName(logoAlign)));

	o.insert(QStringLiteral("widthMode"), QLatin1String(widthModeName(widthMode)));
	o.insert(QStringLiteral("customWidth"), customWidth);
	o.insert(QStringLiteral("minWidth"), minWidth);
	o.insert(QStringLiteral("maxWidth"), maxWidth);
	o.insert(QStringLiteral("heightMode"), QLatin1String(heightModeName(heightMode)));
	o.insert(QStringLiteral("customHeight"), customHeight);
	o.insert(QStringLiteral("padding"), padding);

	o.insert(QStringLiteral("useTemplateColors"), useTemplateColors);
	o.insert(QStringLiteral("colors"), colors.toJson());
	o.insert(QStringLiteral("useTemplateFonts"), useTemplateFonts);
	o.insert(QStringLiteral("titleFont"), titleFont.toJson());
	o.insert(QStringLiteral("textFont"), textFont.toJson());

	o.insert(QStringLiteral("animationIn"), animationToJson(animationIn));
	o.insert(QStringLiteral("animationOut"), animationToJson(animationOut));
	return o;
}

PanelModel PanelModel::fromJson(const QJsonObject &o)
{
	PanelModel m;
	m.id = stringFrom(o, "id");
	m.name = stringFrom(o, "name");
	m.templateId = stringFrom(o, "template", m.templateId);
	m.title = stringFrom(o, "title");
	m.text = stringFrom(o, "text");
	m.showTitle = boolFrom(o, "showTitle", true);
	m.showText = boolFrom(o, "showText", true);
	m.showLogo = boolFrom(o, "showLogo", true);
	m.titleUppercase = boolFrom(o, "titleUppercase", true);

	m.logoPath = stringFrom(o, "logoPath");
	m.logoScalePercent = intFrom(o, "logoScalePercent", m.logoScalePercent);
	m.logoPadding = intFrom(o, "logoPadding", m.logoPadding);
	const QString align = stringFrom(o, "logoAlign");
	m.logoAlign = align == QLatin1String("left")    ? LogoAlign::Left
		      : align == QLatin1String("right") ? LogoAlign::Right
							: LogoAlign::Center;

	m.widthMode = stringFrom(o, "widthMode") == QLatin1String("custom") ? WidthMode::Custom : WidthMode::FitText;
	m.customWidth = intFrom(o, "customWidth", m.customWidth);
	m.minWidth = intFrom(o, "minWidth", m.minWidth);
	m.maxWidth = intFrom(o, "maxWidth", m.maxWidth);
	m.heightMode = stringFrom(o, "heightMode") == QLatin1String("custom") ? HeightMode::Custom : HeightMode::Auto;
	m.customHeight = intFrom(o, "customHeight", m.customHeight);
	m.padding = intFrom(o, "padding", m.padding);

	m.useTemplateColors = boolFrom(o, "useTemplateColors", true);
	m.colors = PanelPalette::fromJson(o.value(QStringLiteral("colors")).toObject(), PanelPalette());
	m.useTemplateFonts = boolFrom(o, "useTemplateFonts", true);
	m.titleFont = FontSpec::fromJson(o.value(QStringLiteral("titleFont")).toObject(), FontSpec());
	m.textFont = FontSpec::fromJson(o.value(QStringLiteral("textFont")).toObject(), FontSpec());

	m.animationIn = animationFromJson(o.value(QStringLiteral("animationIn")).toObject());
	m.animationOut = animationFromJson(o.value(QStringLiteral("animationOut")).toObject());

	m.sanitize();
	return m;
}

} // namespace sp
