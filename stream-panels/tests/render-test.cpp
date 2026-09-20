// Renders every template in several situations to PNG files and checks the
// sizing rules (auto width grows with the text, custom width is respected,
// empty / broken inputs never crash).
#include "model/logo-store.hpp"
#include "model/panel-model.hpp"
#include "render/panel-renderer.hpp"
#include "templates/template-registry.hpp"
#include "test-support.hpp"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QTemporaryDir>
#include <cstdio>

using namespace sp;

static int g_failures = 0;
#define CHECK(cond)                                                                       \
	do {                                                                              \
		if (!(cond)) {                                                            \
			std::fprintf(stderr, "CHECK FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); \
			++g_failures;                                                     \
		}                                                                         \
	} while (0)

static QString makeLogos(const QString &dir)
{
	QImage img(256, 256, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::transparent);
	QPainter p(&img);
	p.setRenderHint(QPainter::Antialiasing);
	p.setBrush(QColor("#FF7A00"));
	p.setPen(Qt::NoPen);
	p.drawEllipse(QRectF(16, 16, 224, 224));
	p.setBrush(Qt::white);
	p.drawEllipse(QRectF(88, 88, 80, 80));
	p.end();
	const QString png = dir + "/logo.png";
	img.save(png);

	QFile svg(dir + "/logo.svg");
	if (svg.open(QIODevice::WriteOnly)) {
		svg.write("<svg xmlns='http://www.w3.org/2000/svg' width='200' height='100' viewBox='0 0 200 100'>"
			  "<rect width='200' height='100' rx='16' fill='#16A34A'/>"
			  "<text x='100' y='64' font-size='48' text-anchor='middle' fill='white'>SVG</text></svg>");
	}
	return png;
}

int main(int argc, char **argv)
{
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QGuiApplication app(argc, argv);
	testsupport::loadLocale();

	const QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("render-out");
	QDir().mkpath(outDir);
	QTemporaryDir tmp;
	const QString png = makeLogos(tmp.path());
	const QString svg = tmp.path() + "/logo.svg";

	auto &reg = TemplateRegistry::instance();
	const QStringList ids = reg.ids();
	CHECK(ids.size() >= 4);

	// A sheet with all templates x several situations, stacked vertically.
	struct Scenario {
		QString label;
		std::function<void(PanelModel &)> apply;
	};
	const std::vector<Scenario> scenarios = {
		{"default", [](PanelModel &) {}},
		{"logo-png", [&](PanelModel &m) { m.logoPath = png; }},
		{"logo-svg", [&](PanelModel &m) { m.logoPath = svg; }},
		{"short", [](PanelModel &m) { m.title = "Hi"; m.text = "Hola"; }},
		{"long", [](PanelModel &m) {
			 m.title = "ENTREVISTA EXCLUSIVA CON EL DIRECTOR";
			 m.text = "Bienvenidos a nuestra transmisión especial de esta noche";
		 }},
		{"very-long-wrapped", [&](PanelModel &m) {
			 m.logoPath = png;
			 m.title = "Mesa redonda";
			 m.text = "Este es un texto extremadamente largo que no cabe en el ancho máximo del panel y "
				  "por lo tanto debe partirse en varias líneas sin cortarse ni salirse";
		 }},
		{"custom-width", [&](PanelModel &m) { m.logoPath = png; m.widthMode = WidthMode::Custom; m.customWidth = 900; }},
		{"custom-size", [&](PanelModel &m) {
			 m.widthMode = WidthMode::Custom;
			 m.customWidth = 600;
			 m.heightMode = HeightMode::Custom;
			 m.customHeight = 220;
			 m.logoPath = png;
		 }},
		{"no-title", [&](PanelModel &m) { m.showTitle = false; m.logoPath = png; }},
		{"no-text", [&](PanelModel &m) { m.showText = false; }},
		{"empty-all", [](PanelModel &m) { m.title.clear(); m.text.clear(); }},
		{"missing-logo", [](PanelModel &m) { m.logoPath = "/does/not/exist.png"; }},
		{"custom-colors", [](PanelModel &m) {
			 m.useTemplateColors = false;
			 m.colors.primary = QColor("#111111");
			 m.colors.secondary = QColor("#00FF88");
			 m.colors.title = QColor("#00FF88");
			 m.colors.text = QColor("#222222");
			 m.colors.background = QColor("#EEEEEE");
			 m.colors.logoBlock = QColor("#CCCCCC");
		 }},
		{"custom-font", [](PanelModel &m) {
			 m.useTemplateFonts = false;
			 m.titleFont = FontSpec{"DejaVu Serif", 30, true, true};
			 m.textFont = FontSpec{"DejaVu Sans Mono", 44, false, false};
		 }},
	};

	QVector<QImage> rows;
	for (const QString &id : ids) {
		for (const Scenario &sc : scenarios) {
			PanelModel m = reg.makeDefaultModel(id);
			m.title = "Entrevista";
			m.text = "Bienvenidos al programa";
			sc.apply(m);
			const RenderResult r = PanelRenderer::render(m, 1.0);
			if (!r.ok()) {
				std::fprintf(stderr, "render failed: %s / %s\n", qPrintable(id), qPrintable(sc.label));
				++g_failures;
				continue;
			}
			CHECK(r.image.width() == r.logicalSize.width());
			CHECK(r.logicalSize.width() >= 24 && r.logicalSize.height() >= 10);
			if (sc.label == "custom-width")
				CHECK(r.logicalSize.width() == 900);
			if (sc.label == "custom-size")
				CHECK(r.logicalSize.width() == 600 && r.logicalSize.height() == 220);
			if (sc.label == "very-long-wrapped")
				CHECK(r.logicalSize.width() <= m.maxWidth);
			rows.push_back(r.image);
			std::printf("%-8s %-18s %4d x %-4d\n", qPrintable(id), qPrintable(sc.label), r.logicalSize.width(),
				    r.logicalSize.height());
		}

		// ---- auto-width must follow the real text width
		PanelModel a = reg.makeDefaultModel(id);
		a.text = "Hola";
		a.title = "Hi";
		a.minWidth = 24;
		PanelModel b = a;
		b.text = "Bienvenidos a nuestra transmisión";
		PanelModel c = a;
		c.text = "iiiiiiiiiiiiiiiiii"; // narrow glyphs: char-count based sizing would over-estimate
		PanelModel d = a;
		d.text = "WWWWWWWWWWWWWWWWWW"; // wide glyphs, same char count as c
		const int wa = PanelRenderer::measure(a).width();
		const int wb = PanelRenderer::measure(b).width();
		const int wc = PanelRenderer::measure(c).width();
		const int wd = PanelRenderer::measure(d).width();
		std::printf("%-8s autowidth: hola=%d long=%d narrow=%d wide=%d\n", qPrintable(id), wa, wb, wc, wd);
		CHECK(wb > wa);
		CHECK(wd > wc); // real glyph measurement, not character count

		// ---- max width clamps and wraps (never wider than max, never cut)
		PanelModel e = b;
		e.text = QString("palabra ").repeated(40);
		e.maxWidth = 800;
		const QSize se = PanelRenderer::measure(e);
		CHECK(se.width() <= 800);
		CHECK(se.height() > PanelRenderer::measure(b).height());

		// ---- min width
		PanelModel f = a;
		f.minWidth = 500;
		CHECK(PanelRenderer::measure(f).width() >= 500);

		// ---- supersampling keeps the logical size
		const RenderResult r1 = PanelRenderer::render(b, 1.0);
		const RenderResult r2 = PanelRenderer::render(b, 2.0);
		CHECK(r1.logicalSize == r2.logicalSize);
		CHECK(r2.image.width() == r1.image.width() * 2);

		// ---- json round trip
		PanelModel g = reg.makeDefaultModel(id);
		g.logoPath = png;
		g.useTemplateColors = false;
		g.colors = reg.find(id)->defaults().palette;
		const PanelModel g2 = PanelModel::fromJson(g.toJson());
		CHECK(g == g2);
	}

	// ---- garbage JSON never crashes and yields a renderable panel
	{
		QJsonObject junk;
		junk.insert("widthMode", 42);
		junk.insert("customWidth", -5);
		junk.insert("colors", "nope");
		junk.insert("titleFont", QJsonObject{{"pixelSize", 100000}});
		const PanelModel m = PanelModel::fromJson(junk);
		CHECK(PanelRenderer::render(m, 1.0).ok());
	}

	// ---- stitch a contact sheet
	int W = 0, H = 0;
	for (const QImage &im : rows) {
		W = std::max(W, im.width());
		H += im.height() + 14;
	}
	QImage sheet(W + 40, H + 20, QImage::Format_ARGB32_Premultiplied);
	sheet.fill(QColor("#4a5568"));
	{
		QPainter p(&sheet);
		int y = 10;
		for (const QImage &im : rows) {
			p.drawImage(20, y, im);
			y += im.height() + 14;
		}
	}
	const int perColumn = int(scenarios.size());
	// split into one PNG per template for easier viewing
	int idx = 0;
	for (const QString &id : ids) {
		int h = 20, w = 0;
		for (int i = 0; i < perColumn; ++i) {
			h += rows[idx + i].height() + 14;
			w = std::max(w, rows[idx + i].width());
		}
		QImage part(w + 40, h, QImage::Format_ARGB32_Premultiplied);
		part.fill(QColor("#4a5568"));
		QPainter p(&part);
		int y = 10;
		for (int i = 0; i < perColumn; ++i) {
			p.drawImage(20, y, rows[idx + i]);
			y += rows[idx + i].height() + 14;
		}
		p.end();
		part.save(outDir + "/template-" + id + ".png");
		idx += perColumn;
	}

	std::printf(g_failures ? "\n%d FAILURE(S)\n" : "\nALL CHECKS PASSED\n", g_failures);
	return g_failures ? 1 : 0;
}
