// Renders the dock offscreen and saves screenshots (one per tab) so the UI can
// be reviewed without OBS. Also exercises the editor's main flows.
#include "model/panel-manager.hpp"
#include "model/logo-store.hpp"
#include "templates/template-registry.hpp"
#include "ui/panel-editor.hpp"
#include "test-support.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

using namespace sp;

class StubBridge : public SceneBridge {
public:
	int added = 0;
	bool addToCurrentScene(const QString &, const QString &, QString *) override { ++added; return true; }
	int usageCount(const QString &) override { return 2; }
	void unlinkSources(const QString &) override {}
};

int main(int argc, char **argv)
{
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QApplication app(argc, argv);
	const QString lang = argc > 2 ? QString::fromLocal8Bit(argv[2]) : QStringLiteral("en-US");
	testsupport::loadLocale(lang);

	const QString outDir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("ui-out");
	QDir().mkpath(outDir);
	QTemporaryDir cfg;
	LogoStore::setBaseDir(cfg.path());
	auto &mgr = PanelManager::instance();
	mgr.setStoragePath(cfg.path() + "/panels.json");
	mgr.load();

	// a few panels so the list looks real
	{
		PanelModel a = TemplateRegistry::instance().makeDefaultModel("classic");
		a.name = "Juan Pérez"; a.title = "Director"; a.text = "Entrevista de apertura";
		mgr.add(a);
		PanelModel b = TemplateRegistry::instance().makeDefaultModel("modern");
		b.name = "María López"; b.title = "Conductora"; b.text = "Bienvenidos al programa";
		mgr.add(b);
		PanelModel c = TemplateRegistry::instance().makeDefaultModel("tag");
		c.name = "Invitado especial"; c.title = "Nombre"; c.text = "Cargo o descripción";
		mgr.add(c);
	}

	// logo
	QImage img(200, 200, QImage::Format_ARGB32);
	img.fill(Qt::transparent);
	{
		QPainter p(&img);
		p.setRenderHint(QPainter::Antialiasing);
		p.setBrush(QColor("#FF7A00")); p.setPen(Qt::NoPen);
		p.drawEllipse(10, 10, 180, 180);
		p.setBrush(Qt::white); p.drawEllipse(70, 70, 60, 60);
	}
	const QString logoFile = cfg.path() + "/src-logo.png";
	img.save(logoFile);

	StubBridge bridge;
	PanelEditor editor(&bridge);
	editor.resize(380, 980);
	editor.show();
	app.processEvents();

	auto shot = [&](const QString &name) {
		QTimer::singleShot(0, [] {});
		app.processEvents();
		editor.grab().save(outDir + "/" + name + ".png");
	};
	shot("1-content");

	auto *tabs = editor.findChild<QTabWidget *>();
	tabs->setCurrentIndex(1);
	shot("2-size");
	tabs->setCurrentIndex(2);
	shot("3-style");
	tabs->setCurrentIndex(0);

	// ---- interaction flow: edit -> draft only -> UPDATE commits -> ADD TO SCENE uses bridge
	auto button = [&](const QString &text) -> QPushButton * {
		for (auto *b : editor.findChildren<QPushButton *>())
			if (b->text() == text) return b;
		return nullptr;
	};
	int failures = 0;
	auto expect = [&](bool ok, const char *what) { if (!ok) { std::printf("FAIL: %s\n", what); ++failures; } };

	if (!button("UPDATE")) // flow assertions look buttons up by their English captions
		return 0;
	const QString firstId = mgr.panels().first().id;
	QLineEdit *titleEdit = nullptr;
	for (auto *le : editor.findChildren<QLineEdit *>())
		if (le->placeholderText() == "TITLE") titleEdit = le;
	expect(titleEdit != nullptr, "title field exists");
	titleEdit->setText("ENTREVISTA");
	app.processEvents();
	expect(mgr.panel(firstId)->title != "ENTREVISTA", "edit must stay in the draft until UPDATE");
	button("UPDATE")->click();
	expect(mgr.panel(firstId)->title == "ENTREVISTA", "UPDATE commits the draft");

	int updates = 0;
	QObject::connect(&mgr, &PanelManager::panelUpdated, [&](const QString &) { ++updates; });
	button("ADD TO SCENE")->click();
	expect(bridge.added == 1, "ADD TO SCENE calls the bridge");
	expect(updates == 1, "ADD TO SCENE commits first");

	button("Duplicate")->click();
	expect(mgr.count() == 4, "duplicate adds a panel");
	button("+ New")->click();
	expect(mgr.count() == 5, "new adds a panel");

	// logo import (goes through LogoStore, relative path in the model)
	const QString stored = LogoStore::importFile(logoFile);
	expect(stored.startsWith("logos/"), "logo imported into config dir");
	expect(QFile::exists(LogoStore::resolve(stored)), "imported logo resolves");
	expect(LogoStore::resolve("../../etc/passwd").isEmpty(), "path traversal refused");

	// persistence round trip
	expect(mgr.save(), "save works");
	editor.flushDraftsQuietly();
	{
		QFile f(cfg.path() + "/panels.json");
		expect(f.exists() && f.size() > 100, "panels.json written");
	}
	{
		// reload from disk: same panels, same content
		const auto before = mgr.panels();
		expect(mgr.load(), "load works");
		const auto after = mgr.panels();
		expect(before.size() == after.size(), "reload keeps the panel count");
		bool same = before.size() == after.size();
		for (int i = 0; same && i < before.size(); ++i)
			same = before[i] == after[i];
		expect(same, "reload keeps panel content");
		// a corrupt file is moved aside instead of being overwritten
		QFile f(cfg.path() + "/panels.json");
		f.open(QIODevice::WriteOnly | QIODevice::Truncate);
		f.write("{ not json");
		f.close();
		expect(!mgr.load(), "corrupt file is reported");
		expect(!QFile::exists(cfg.path() + "/panels.json"), "corrupt file was moved aside");
	}
	std::printf("panels in library: %d\n", mgr.count());
	std::printf("failures: %d\n", failures);
	std::printf("added via bridge: %d\n", bridge.added);
	return failures;
}
