#include "ui/panel-editor.hpp"

#include "common/platform.hpp"
#include "model/logo-store.hpp"
#include "model/panel-manager.hpp"
#include "render/logo-source.hpp"
#include "templates/template-registry.hpp"
#include "ui/color-button.hpp"
#include "ui/preview-widget.hpp"
#include "ui/template-picker.hpp"
#include "ui/theme.hpp"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace sp {

namespace {

enum ListRole { IdRole = Qt::UserRole + 1, SubtitleRole, ColorRole, DirtyRole };

// Two-line list rows: color dot, name, subtitle, "unapplied changes" marker.
class PanelListDelegate : public QStyledItemDelegate {
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return QSize(100, 46); }

	void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const override
	{
		p->save();
		p->setRenderHint(QPainter::Antialiasing, true);
		const QRectF r = QRectF(opt.rect).adjusted(2, 2, -2, -2);

		if (opt.state & QStyle::State_Selected) {
			p->setPen(QPen(QColor("#3B82F6"), 1));
			p->setBrush(QColor("#1E2A44"));
			p->drawRoundedRect(r, 6, 6);
		} else if (opt.state & QStyle::State_MouseOver) {
			p->setPen(Qt::NoPen);
			p->setBrush(QColor("#232733"));
			p->drawRoundedRect(r, 6, 6);
		}

		const QColor dot = index.data(ColorRole).value<QColor>();
		p->setPen(Qt::NoPen);
		p->setBrush(dot.isValid() ? dot : QColor("#8B93A7"));
		p->drawEllipse(QPointF(r.left() + 16, r.center().y()), 5.5, 5.5);

		const bool dirty = index.data(DirtyRole).toBool();
		const qreal textLeft = r.left() + 32;
		const qreal textRight = r.right() - (dirty ? 26 : 10);

		QFont nameFont = opt.font;
		nameFont.setPixelSize(13);
		nameFont.setBold(true);
		p->setFont(nameFont);
		p->setPen(QColor("#E6E8EE"));
		const QFontMetricsF nm(nameFont);
		p->drawText(QRectF(textLeft, r.top() + 5, textRight - textLeft, 18), Qt::AlignVCenter | Qt::AlignLeft,
			    nm.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textRight - textLeft));

		QFont subFont = opt.font;
		subFont.setPixelSize(11);
		p->setFont(subFont);
		p->setPen(QColor("#8B93A7"));
		const QFontMetricsF sm(subFont);
		p->drawText(QRectF(textLeft, r.top() + 23, textRight - textLeft, 16), Qt::AlignVCenter | Qt::AlignLeft,
			    sm.elidedText(index.data(SubtitleRole).toString(), Qt::ElideRight, textRight - textLeft));

		if (dirty) {
			p->setPen(Qt::NoPen);
			p->setBrush(QColor("#F59E0B"));
			p->drawEllipse(QPointF(r.right() - 14, r.center().y()), 4, 4);
		}
		p->restore();
	}
};

QLabel *caption(const QString &text, QWidget *parent = nullptr)
{
	auto *l = new QLabel(text, parent);
	l->setObjectName(QStringLiteral("FieldCaption"));
	return l;
}

QLabel *sectionTitle(const QString &text, QWidget *parent = nullptr)
{
	auto *l = new QLabel(text.toUpper(), parent);
	l->setObjectName(QStringLiteral("SectionTitle"));
	return l;
}

QSpinBox *makeSpin(int min, int max, int value, const QString &suffix = QStringLiteral(" px"))
{
	auto *s = new QSpinBox;
	s->setRange(min, max);
	s->setValue(value);
	s->setSuffix(suffix);
	s->setKeyboardTracking(false);
	return s;
}

QToolButton *makeToggle(const QString &text, const QString &tip, bool segment = false)
{
	auto *b = new QToolButton;
	b->setText(text);
	b->setToolTip(tip);
	b->setCheckable(true);
	b->setAutoRaise(false);
	b->setCursor(Qt::PointingHandCursor);
	if (segment)
		b->setProperty("segment", true);
	return b;
}

const PanelPalette &colorsOf(const PanelModel &m, const TemplateDefaults &d, PanelPalette &scratch)
{
	if (m.useTemplateColors)
		return scratch = d.palette;
	scratch = m.colors;
	if (!scratch.primary.isValid())
		scratch.primary = d.palette.primary;
	if (!scratch.secondary.isValid())
		scratch.secondary = d.palette.secondary;
	if (!scratch.title.isValid())
		scratch.title = d.palette.title;
	if (!scratch.text.isValid())
		scratch.text = d.palette.text;
	if (!scratch.background.isValid())
		scratch.background = d.palette.background;
	if (!scratch.logoBlock.isValid())
		scratch.logoBlock = d.palette.logoBlock;
	return scratch;
}

} // namespace

// ============================================================ construction

PanelEditor::PanelEditor(SceneBridge *bridge, QWidget *parent) : QWidget(parent), m_bridge(bridge)
{
	setObjectName(QStringLiteral("StreamPanelsRoot"));
	setStyleSheet(ui::styleSheet());
	setMinimumWidth(340);

	m_previewTimer.setSingleShot(true);
	m_previewTimer.setInterval(16);
	connect(&m_previewTimer, &QTimer::timeout, this, [this]() {
		if (hasCurrent())
			m_preview->setModel(draft());
		else
			m_preview->clear();
	});

	m_liveTimer.setSingleShot(true);
	m_liveTimer.setInterval(180);
	connect(&m_liveTimer, &QTimer::timeout, this, [this]() {
		if (hasCurrent() && isDirty(m_currentId))
			commitCurrent(true);
	});

	buildUi();

	auto &mgr = PanelManager::instance();
	connect(&mgr, &PanelManager::libraryChanged, this, [this]() { refreshList(); });
	connect(&mgr, &PanelManager::panelRemoved, this, [this](const QString &id) { m_drafts.remove(id); });

	// First run: give the operator something to look at.
	if (mgr.count() == 0) {
		PanelModel first = TemplateRegistry::instance().makeDefaultModel(QStringLiteral("classic"));
		mgr.add(first);
	}
	refreshList();

	QString start = mgr.lastSelectedId();
	if (!mgr.contains(start)) {
		const auto all = mgr.panels();
		start = all.isEmpty() ? QString() : all.first().id;
	}
	selectPanel(start);
}

PanelEditor::~PanelEditor() = default;

void PanelEditor::buildUi()
{
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->setSpacing(0);

	// ---- scrollable content
	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	auto *content = new QWidget;
	content->setObjectName(QStringLiteral("Content"));
	content->setMinimumWidth(0);
	scroll->setWidget(content);
	outer->addWidget(scroll, 1);

	auto *col = new QVBoxLayout(content);
	col->setContentsMargins(14, 14, 14, 10);
	col->setSpacing(10);

	auto *header = new QLabel(T("Header.Title"));
	header->setObjectName(QStringLiteral("Header"));
	col->addWidget(header);

	// ---- panel list
	col->addWidget(sectionTitle(T("Panels.Title")));
	m_list = new QListWidget;
	m_list->setItemDelegate(new PanelListDelegate(m_list));
	m_list->setMouseTracking(true);
	m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_list->setFixedHeight(4 * 46 + 12);
	col->addWidget(m_list);
	connect(m_list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item, QListWidgetItem *) {
		if (m_loading || !item)
			return;
		selectPanel(item->data(IdRole).toString());
	});
	connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { renamePanel(); });

	auto *listButtons = new QHBoxLayout;
	listButtons->setSpacing(6);
	m_newBtn = new QPushButton(T("Panels.New"));
	m_dupBtn = new QPushButton(T("Panels.Duplicate"));
	m_renameBtn = new QPushButton(T("Panels.Rename"));
	for (auto *b : {m_newBtn, m_dupBtn, m_renameBtn}) {
		b->setCursor(Qt::PointingHandCursor);
		listButtons->addWidget(b, 1);
	}
	col->addLayout(listButtons);
	connect(m_newBtn, &QPushButton::clicked, this, &PanelEditor::newPanel);
	connect(m_dupBtn, &QPushButton::clicked, this, &PanelEditor::duplicatePanel);
	connect(m_renameBtn, &QPushButton::clicked, this, &PanelEditor::renamePanel);

	// ---- preview
	col->addSpacing(2);
	col->addWidget(sectionTitle(T("Preview.Title")));
	m_preview = new PreviewWidget;
	col->addWidget(m_preview);

	// ---- tabs
	m_tabs = new QTabWidget;
	m_tabs->setDocumentMode(true);
	m_tabs->addTab(buildContentTab(), T("Tab.Content"));
	m_tabs->addTab(buildSizeTab(), T("Tab.Size"));
	m_tabs->addTab(buildStyleTab(), T("Tab.Style"));
	col->addWidget(m_tabs);
	col->addStretch(1);

	// ---- actions (always visible, outside the scroll area)
	auto *sep = new QFrame;
	sep->setObjectName(QStringLiteral("Separator"));
	sep->setFrameShape(QFrame::HLine);
	outer->addWidget(sep);

	auto *actions = new QWidget;
	auto *al = new QVBoxLayout(actions);
	al->setContentsMargins(14, 10, 14, 12);
	al->setSpacing(8);

	m_addBtn = new QPushButton(T("Action.AddToScene"));
	m_addBtn->setObjectName(QStringLiteral("Primary"));
	m_addBtn->setCursor(Qt::PointingHandCursor);
	al->addWidget(m_addBtn);

	auto *row = new QHBoxLayout;
	row->setSpacing(6);
	m_updateBtn = new QPushButton(T("Action.Update"));
	m_saveBtn = new QPushButton(T("Action.Save"));
	m_deleteBtn = new QPushButton(T("Action.Delete"));
	m_deleteBtn->setObjectName(QStringLiteral("Danger"));
	for (auto *b : {m_updateBtn, m_saveBtn, m_deleteBtn}) {
		b->setCursor(Qt::PointingHandCursor);
		row->addWidget(b, 1);
	}
	al->addLayout(row);

	auto *statusRow = new QHBoxLayout;
	m_liveCheck = new QCheckBox(T("Action.Live"));
	m_liveCheck->setToolTip(T("Action.LiveTip"));
	m_status = new QLabel;
	m_status->setObjectName(QStringLiteral("Status"));
	m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	statusRow->addWidget(m_liveCheck);
	statusRow->addWidget(m_status, 1);
	al->addLayout(statusRow);
	outer->addWidget(actions);

	connect(m_addBtn, &QPushButton::clicked, this, &PanelEditor::addToScene);
	connect(m_updateBtn, &QPushButton::clicked, this, [this]() { commitCurrent(true); });
	connect(m_saveBtn, &QPushButton::clicked, this, &PanelEditor::saveNow);
	connect(m_deleteBtn, &QPushButton::clicked, this, &PanelEditor::deletePanel);
	connect(m_liveCheck, &QCheckBox::toggled, this, [this](bool on) {
		if (on && hasCurrent() && isDirty(m_currentId))
			commitCurrent(true);
	});
}

QWidget *PanelEditor::buildContentTab()
{
	auto *page = new QWidget;
	auto *v = new QVBoxLayout(page);
	v->setContentsMargins(12, 12, 12, 12);
	v->setSpacing(8);

	// design
	v->addWidget(caption(T("Field.Design")));
	m_picker = new TemplatePicker;
	v->addWidget(m_picker);
	connect(m_picker, &TemplatePicker::templateChosen, this, [this](const QString &id) {
		edit([&](PanelModel &m) {
			m.templateId = id;
			const TemplateDefaults d = TemplateRegistry::instance().find(id)->defaults();
			m.titleUppercase = d.titleUppercase;
			m.minWidth = d.minWidth;
			m.maxWidth = d.maxWidth;
		});
		loadControls(); // uppercase / min / max / effective colors changed
	});

	// title
	v->addSpacing(4);
	auto *titleHead = new QHBoxLayout;
	titleHead->addWidget(caption(T("Field.Title")));
	titleHead->addStretch(1);
	m_titleUpper = new QCheckBox(T("Field.Uppercase"));
	m_titleShow = new QCheckBox(T("Field.Show"));
	titleHead->addWidget(m_titleUpper);
	titleHead->addSpacing(8);
	titleHead->addWidget(m_titleShow);
	v->addLayout(titleHead);
	m_titleEdit = new QLineEdit;
	m_titleEdit->setPlaceholderText(T("Sample.Title"));
	v->addWidget(m_titleEdit);

	// text
	auto *textHead = new QHBoxLayout;
	textHead->addWidget(caption(T("Field.Text")));
	textHead->addStretch(1);
	m_textShow = new QCheckBox(T("Field.Show"));
	textHead->addWidget(m_textShow);
	v->addLayout(textHead);
	m_textEdit = new QPlainTextEdit;
	m_textEdit->setPlaceholderText(T("Sample.Text"));
	m_textEdit->setTabChangesFocus(true);
	m_textEdit->setFixedHeight(62);
	v->addWidget(m_textEdit);

	// logo
	v->addSpacing(4);
	auto *logoHead = new QHBoxLayout;
	logoHead->addWidget(caption(T("Logo.Title")));
	logoHead->addStretch(1);
	m_logoShow = new QCheckBox(T("Field.Show"));
	logoHead->addWidget(m_logoShow);
	v->addLayout(logoHead);

	auto *logoRow = new QHBoxLayout;
	logoRow->setSpacing(8);
	m_logoThumb = new QLabel;
	m_logoThumb->setFixedSize(46, 46);
	m_logoThumb->setAlignment(Qt::AlignCenter);
	m_logoThumb->setStyleSheet(QStringLiteral("background:#14161A;border:1px solid #2E3341;border-radius:6px;"));
	logoRow->addWidget(m_logoThumb);
	m_logoSelect = new QPushButton(T("Logo.Select"));
	m_logoRemove = new QPushButton(T("Logo.Remove"));
	logoRow->addWidget(m_logoSelect, 1);
	logoRow->addWidget(m_logoRemove);
	v->addLayout(logoRow);

	auto *grid = new QGridLayout;
	grid->setHorizontalSpacing(8);
	grid->setVerticalSpacing(6);
	grid->addWidget(caption(T("Logo.Scale")), 0, 0);
	m_logoScale = new QSlider(Qt::Horizontal);
	m_logoScale->setRange(10, 150);
	grid->addWidget(m_logoScale, 0, 1);
	m_logoScaleLabel = new QLabel;
	m_logoScaleLabel->setMinimumWidth(38);
	m_logoScaleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	grid->addWidget(m_logoScaleLabel, 0, 2);
	grid->addWidget(caption(T("Logo.Padding")), 1, 0);
	m_logoPadding = makeSpin(0, 60, 8);
	grid->addWidget(m_logoPadding, 1, 1, 1, 2);
	grid->addWidget(caption(T("Logo.Align")), 2, 0);
	auto *alignRow = new QHBoxLayout;
	alignRow->setSpacing(0);
	const QString names[3] = {T("Logo.AlignLeft"), T("Logo.AlignCenter"), T("Logo.AlignRight")};
	auto *alignGroup = new QButtonGroup(this);
	alignGroup->setExclusive(true);
	for (int i = 0; i < 3; ++i) {
		m_alignBtn[i] = makeToggle(names[i], names[i], true);
		alignGroup->addButton(m_alignBtn[i]);
		alignRow->addWidget(m_alignBtn[i]);
	}
	alignRow->addStretch(1);
	grid->addLayout(alignRow, 2, 1, 1, 2);
	grid->setColumnStretch(1, 1);
	v->addLayout(grid);

	// ---- wiring
	connect(m_titleEdit, &QLineEdit::textChanged, this, [this](const QString &t) { edit([&](PanelModel &m) { m.title = t; }); });
	connect(m_titleShow, &QCheckBox::toggled, this, [this](bool on) { edit([&](PanelModel &m) { m.showTitle = on; }); });
	connect(m_titleUpper, &QCheckBox::toggled, this, [this](bool on) { edit([&](PanelModel &m) { m.titleUppercase = on; }); });
	connect(m_textEdit, &QPlainTextEdit::textChanged, this,
		[this]() { edit([&](PanelModel &m) { m.text = m_textEdit->toPlainText(); }); });
	connect(m_textShow, &QCheckBox::toggled, this, [this](bool on) { edit([&](PanelModel &m) { m.showText = on; }); });
	connect(m_logoShow, &QCheckBox::toggled, this, [this](bool on) { edit([&](PanelModel &m) { m.showLogo = on; }); });
	connect(m_logoSelect, &QPushButton::clicked, this, &PanelEditor::chooseLogo);
	connect(m_logoRemove, &QPushButton::clicked, this, [this]() {
		edit([&](PanelModel &m) { m.logoPath.clear(); });
		updateLogoThumbnail();
		updateEnabledStates();
	});
	connect(m_logoScale, &QSlider::valueChanged, this, [this](int v) {
		m_logoScaleLabel->setText(QStringLiteral("%1%").arg(v));
		edit([&](PanelModel &m) { m.logoScalePercent = v; });
	});
	connect(m_logoPadding, &QSpinBox::valueChanged, this, [this](int v) { edit([&](PanelModel &m) { m.logoPadding = v; }); });
	for (int i = 0; i < 3; ++i) {
		connect(m_alignBtn[i], &QToolButton::clicked, this, [this, i]() {
			edit([&](PanelModel &m) {
				m.logoAlign = i == 0 ? LogoAlign::Left : i == 2 ? LogoAlign::Right : LogoAlign::Center;
			});
		});
	}
	return page;
}

QWidget *PanelEditor::buildSizeTab()
{
	auto *page = new QWidget;
	auto *v = new QVBoxLayout(page);
	v->setContentsMargins(12, 12, 12, 12);
	v->setSpacing(8);

	v->addWidget(caption(T("Size.Width")));
	m_widthFit = new QRadioButton(T("Size.FitText"));
	m_widthCustom = new QRadioButton(T("Size.CustomWidth"));
	auto *widthGroup = new QButtonGroup(this);
	widthGroup->addButton(m_widthFit);
	widthGroup->addButton(m_widthCustom);
	v->addWidget(m_widthFit);
	v->addWidget(m_widthCustom);

	auto *grid = new QGridLayout;
	grid->setHorizontalSpacing(8);
	grid->setVerticalSpacing(6);
	grid->addWidget(caption(T("Size.CustomWidthValue")), 0, 0);
	m_widthSpin = makeSpin(limits::kMinDimension, limits::kMaxDimension, 700);
	grid->addWidget(m_widthSpin, 0, 1);
	grid->addWidget(caption(T("Size.Min")), 1, 0);
	m_minWidthSpin = makeSpin(limits::kMinDimension, limits::kMaxDimension, 240);
	grid->addWidget(m_minWidthSpin, 1, 1);
	grid->addWidget(caption(T("Size.Max")), 2, 0);
	m_maxWidthSpin = makeSpin(limits::kMinDimension, limits::kMaxDimension, 1600);
	grid->addWidget(m_maxWidthSpin, 2, 1);
	grid->setColumnStretch(1, 1);
	v->addLayout(grid);

	v->addSpacing(6);
	v->addWidget(caption(T("Size.Height")));
	m_heightAuto = new QRadioButton(T("Size.Auto"));
	m_heightCustom = new QRadioButton(T("Size.CustomHeight"));
	auto *heightGroup = new QButtonGroup(this);
	heightGroup->addButton(m_heightAuto);
	heightGroup->addButton(m_heightCustom);
	v->addWidget(m_heightAuto);
	v->addWidget(m_heightCustom);
	auto *hgrid = new QGridLayout;
	hgrid->setHorizontalSpacing(8);
	hgrid->addWidget(caption(T("Size.CustomHeightValue")), 0, 0);
	m_heightSpin = makeSpin(limits::kMinDimension, limits::kMaxDimension, 140);
	hgrid->addWidget(m_heightSpin, 0, 1);
	hgrid->setColumnStretch(1, 1);
	v->addLayout(hgrid);

	v->addSpacing(6);
	v->addWidget(caption(T("Size.Padding")));
	m_paddingSpin = makeSpin(-1, limits::kMaxPadding, -1);
	m_paddingSpin->setSpecialValueText(T("Size.PaddingAuto"));
	v->addWidget(m_paddingSpin);
	v->addStretch(1);

	connect(m_widthFit, &QRadioButton::toggled, this, [this](bool on) {
		if (on) {
			edit([&](PanelModel &m) { m.widthMode = WidthMode::FitText; });
			updateEnabledStates();
		}
	});
	connect(m_widthCustom, &QRadioButton::toggled, this, [this](bool on) {
		if (on) {
			edit([&](PanelModel &m) { m.widthMode = WidthMode::Custom; });
			updateEnabledStates();
		}
	});
	connect(m_widthSpin, &QSpinBox::valueChanged, this, [this](int v) { edit([&](PanelModel &m) { m.customWidth = v; }); });
	connect(m_minWidthSpin, &QSpinBox::valueChanged, this, [this](int v) {
		edit([&](PanelModel &m) {
			m.minWidth = v;
			if (m.maxWidth < v)
				m.maxWidth = v;
		});
		if (hasCurrent()) {
			const QSignalBlocker b(m_maxWidthSpin);
			m_maxWidthSpin->setValue(draft().maxWidth);
		}
	});
	connect(m_maxWidthSpin, &QSpinBox::valueChanged, this, [this](int v) {
		edit([&](PanelModel &m) {
			m.maxWidth = v;
			if (m.minWidth > v)
				m.minWidth = v;
		});
		if (hasCurrent()) {
			const QSignalBlocker b(m_minWidthSpin);
			m_minWidthSpin->setValue(draft().minWidth);
		}
	});
	connect(m_heightAuto, &QRadioButton::toggled, this, [this](bool on) {
		if (on) {
			edit([&](PanelModel &m) { m.heightMode = HeightMode::Auto; });
			updateEnabledStates();
		}
	});
	connect(m_heightCustom, &QRadioButton::toggled, this, [this](bool on) {
		if (on) {
			edit([&](PanelModel &m) { m.heightMode = HeightMode::Custom; });
			updateEnabledStates();
		}
	});
	connect(m_heightSpin, &QSpinBox::valueChanged, this, [this](int v) { edit([&](PanelModel &m) { m.customHeight = v; }); });
	connect(m_paddingSpin, &QSpinBox::valueChanged, this, [this](int v) { edit([&](PanelModel &m) { m.padding = v; }); });
	return page;
}

PanelEditor::FontControls PanelEditor::buildFontRow(QWidget *, QGridLayout *grid, int row, const QString &captionText)
{
	FontControls f;
	grid->addWidget(caption(captionText), row, 0, 1, 4);
	f.family = new QFontComboBox;
	f.family->setEditable(false);
	grid->addWidget(f.family, row + 1, 0, 1, 4);
	f.size = makeSpin(limits::kMinFontPx, limits::kMaxFontPx, 32);
	grid->addWidget(f.size, row + 2, 0, 1, 2);
	f.bold = makeToggle(QStringLiteral("B"), T("Font.Bold"));
	f.italic = makeToggle(QStringLiteral("I"), T("Font.Italic"));
	QFont bf = f.bold->font();
	bf.setBold(true);
	f.bold->setFont(bf);
	QFont itf = f.italic->font();
	itf.setItalic(true);
	f.italic->setFont(itf);
	grid->addWidget(f.bold, row + 2, 2);
	grid->addWidget(f.italic, row + 2, 3);
	return f;
}

QWidget *PanelEditor::buildStyleTab()
{
	auto *page = new QWidget;
	auto *v = new QVBoxLayout(page);
	v->setContentsMargins(12, 12, 12, 12);
	v->setSpacing(8);

	// colors
	m_useTplColors = new QCheckBox(T("Style.UseTemplateColors"));
	v->addWidget(m_useTplColors);

	const char *keys[6] = {"Color.Primary", "Color.Secondary", "Color.Title", "Color.Text", "Color.Background", "Color.LogoBlock"};
	auto *grid = new QGridLayout;
	grid->setHorizontalSpacing(8);
	grid->setVerticalSpacing(4);
	for (int i = 0; i < 6; ++i) {
		const int r = (i / 2) * 2, c = i % 2;
		grid->addWidget(caption(T(keys[i])), r, c);
		m_colorBtn[i] = new ColorButton;
		m_colorBtn[i]->setDialogTitle(T(keys[i]));
		grid->addWidget(m_colorBtn[i], r + 1, c);
		connect(m_colorBtn[i], &ColorButton::colorChanged, this, [this, i](const QColor &color) {
			edit([&](PanelModel &m) {
				switch (i) {
				case 0: m.colors.primary = color; break;
				case 1: m.colors.secondary = color; break;
				case 2: m.colors.title = color; break;
				case 3: m.colors.text = color; break;
				case 4: m.colors.background = color; break;
				default: m.colors.logoBlock = color; break;
				}
			});
		});
	}
	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	v->addLayout(grid);

	auto *sep = new QFrame;
	sep->setObjectName(QStringLiteral("Separator"));
	sep->setFrameShape(QFrame::HLine);
	v->addSpacing(4);
	v->addWidget(sep);
	v->addSpacing(4);

	// fonts
	m_useTplFonts = new QCheckBox(T("Style.UseTemplateFonts"));
	v->addWidget(m_useTplFonts);
	auto *fontGrid = new QGridLayout;
	fontGrid->setHorizontalSpacing(6);
	fontGrid->setVerticalSpacing(4);
	m_titleFont = buildFontRow(page, fontGrid, 0, T("Font.Title"));
	fontGrid->setRowMinimumHeight(3, 8);
	m_textFont = buildFontRow(page, fontGrid, 4, T("Font.Text"));
	fontGrid->setColumnStretch(0, 1);
	v->addLayout(fontGrid);
	v->addStretch(1);

	connect(m_useTplColors, &QCheckBox::toggled, this, [this](bool on) {
		if (m_loading || !hasCurrent())
			return;
		edit([&](PanelModel &m) {
			m.useTemplateColors = on;
			if (!on) // start customizing from the look the template currently has
				m.colors = TemplateRegistry::instance().find(m.templateId)->defaults().palette;
		});
		loadControls();
	});
	connect(m_useTplFonts, &QCheckBox::toggled, this, [this](bool on) {
		if (m_loading || !hasCurrent())
			return;
		edit([&](PanelModel &m) {
			m.useTemplateFonts = on;
			if (!on) {
				const TemplateDefaults d = TemplateRegistry::instance().find(m.templateId)->defaults();
				m.titleFont = d.titleFont;
				m.textFont = d.textFont;
			}
		});
		loadControls();
	});

	auto wireFont = [this](FontControls &fc, bool isTitle) {
		auto fontOf = [isTitle](PanelModel &m) -> FontSpec & { return isTitle ? m.titleFont : m.textFont; };
		connect(fc.family, &QFontComboBox::currentFontChanged, this, [this, fontOf](const QFont &f) {
			edit([&](PanelModel &m) { fontOf(m).family = f.family(); });
		});
		connect(fc.size, &QSpinBox::valueChanged, this, [this, fontOf](int v) {
			edit([&](PanelModel &m) { fontOf(m).pixelSize = v; });
		});
		connect(fc.bold, &QToolButton::toggled, this, [this, fontOf](bool on) {
			edit([&](PanelModel &m) { fontOf(m).bold = on; });
		});
		connect(fc.italic, &QToolButton::toggled, this, [this, fontOf](bool on) {
			edit([&](PanelModel &m) { fontOf(m).italic = on; });
		});
	};
	wireFont(m_titleFont, true);
	wireFont(m_textFont, false);
	return page;
}

// ================================================================== state

bool PanelEditor::isDirty(const QString &id) const
{
	const auto it = m_drafts.constFind(id);
	if (it == m_drafts.constEnd())
		return false;
	const auto committed = PanelManager::instance().panel(id);
	return !committed || *committed != *it;
}

void PanelEditor::edit(const std::function<void(PanelModel &)> &change)
{
	if (m_loading || !hasCurrent())
		return;
	change(draft());
	draft().sanitize();
	refreshCurrentListItem();
	updateStatus();
	updateEnabledStates();
	schedulePreview();
	if (m_liveCheck && m_liveCheck->isChecked())
		m_liveTimer.start();
}

void PanelEditor::schedulePreview()
{
	m_previewTimer.start();
}

void PanelEditor::selectPanel(const QString &id)
{
	auto &mgr = PanelManager::instance();
	if (id.isEmpty() || !mgr.contains(id)) {
		m_currentId.clear();
		loadControls();
		m_preview->clear();
		updateEnabledStates();
		updateStatus();
		return;
	}
	m_currentId = id;
	if (!m_drafts.contains(id))
		m_drafts.insert(id, *mgr.panel(id));
	mgr.setLastSelectedId(id);

	// sync list selection without re-entering selectPanel()
	{
		const bool prev = m_loading;
		m_loading = true;
		for (int i = 0; i < m_list->count(); ++i) {
			if (m_list->item(i)->data(IdRole).toString() == id) {
				m_list->setCurrentRow(i);
				break;
			}
		}
		m_loading = prev;
	}
	loadControls();
	m_preview->setModel(draft());
	updateStatus();
}

void PanelEditor::refreshList()
{
	auto &mgr = PanelManager::instance();
	const bool prev = m_loading;
	m_loading = true;
	m_list->clear();
	for (const PanelModel &committed : mgr.panels()) {
		const PanelModel &m = m_drafts.contains(committed.id) ? m_drafts[committed.id] : committed;
		auto *item = new QListWidgetItem;
		item->setData(IdRole, committed.id);
		item->setData(Qt::DisplayRole, m.displayName().isEmpty() ? T("Panels.Untitled") : m.displayName());
		item->setData(SubtitleRole, m.subtitle());
		PanelPalette scratch;
		const TemplateDefaults d = TemplateRegistry::instance().find(m.templateId)->defaults();
		item->setData(ColorRole, colorsOf(m, d, scratch).primary);
		item->setData(DirtyRole, isDirty(committed.id));
		m_list->addItem(item);
		if (committed.id == m_currentId)
			m_list->setCurrentItem(item);
	}
	m_loading = prev;

	// The selected panel may have disappeared (deleted elsewhere).
	if (!m_currentId.isEmpty() && !mgr.contains(m_currentId)) {
		const auto all = mgr.panels();
		selectPanel(all.isEmpty() ? QString() : all.first().id);
	}
}

void PanelEditor::refreshCurrentListItem()
{
	if (!hasCurrent())
		return;
	for (int i = 0; i < m_list->count(); ++i) {
		QListWidgetItem *item = m_list->item(i);
		if (item->data(IdRole).toString() != m_currentId)
			continue;
		const PanelModel &m = draft();
		item->setData(Qt::DisplayRole, m.displayName().isEmpty() ? T("Panels.Untitled") : m.displayName());
		item->setData(SubtitleRole, m.subtitle());
		PanelPalette scratch;
		const TemplateDefaults d = TemplateRegistry::instance().find(m.templateId)->defaults();
		item->setData(ColorRole, colorsOf(m, d, scratch).primary);
		item->setData(DirtyRole, isDirty(m_currentId));
		break;
	}
	m_list->viewport()->update();
}

void PanelEditor::loadControls()
{
	m_loading = true;
	if (!hasCurrent()) {
		m_titleEdit->clear();
		m_textEdit->clear();
		updateLogoThumbnail();
		m_loading = false;
		updateEnabledStates();
		return;
	}
	const PanelModel &m = draft();
	const TemplateDefaults d = TemplateRegistry::instance().find(m.templateId)->defaults();

	m_picker->setCurrent(m.templateId);
	if (m_titleEdit->text() != m.title)
		m_titleEdit->setText(m.title);
	m_titleShow->setChecked(m.showTitle);
	m_titleUpper->setChecked(m.titleUppercase);
	if (m_textEdit->toPlainText() != m.text)
		m_textEdit->setPlainText(m.text);
	m_textShow->setChecked(m.showText);
	m_logoShow->setChecked(m.showLogo);
	m_logoScale->setValue(m.logoScalePercent);
	m_logoScaleLabel->setText(QStringLiteral("%1%").arg(m.logoScalePercent));
	m_logoPadding->setValue(m.logoPadding);
	m_alignBtn[0]->setChecked(m.logoAlign == LogoAlign::Left);
	m_alignBtn[1]->setChecked(m.logoAlign == LogoAlign::Center);
	m_alignBtn[2]->setChecked(m.logoAlign == LogoAlign::Right);
	updateLogoThumbnail();

	m_widthFit->setChecked(m.widthMode == WidthMode::FitText);
	m_widthCustom->setChecked(m.widthMode == WidthMode::Custom);
	m_widthSpin->setValue(m.customWidth);
	m_minWidthSpin->setValue(m.minWidth);
	m_maxWidthSpin->setValue(m.maxWidth);
	m_heightAuto->setChecked(m.heightMode == HeightMode::Auto);
	m_heightCustom->setChecked(m.heightMode == HeightMode::Custom);
	m_heightSpin->setValue(m.customHeight);
	m_paddingSpin->setValue(m.padding);

	m_useTplColors->setChecked(m.useTemplateColors);
	PanelPalette scratch;
	const PanelPalette &c = colorsOf(m, d, scratch);
	m_colorBtn[0]->setColor(c.primary);
	m_colorBtn[1]->setColor(c.secondary);
	m_colorBtn[2]->setColor(c.title);
	m_colorBtn[3]->setColor(c.text);
	m_colorBtn[4]->setColor(c.background);
	m_colorBtn[5]->setColor(c.logoBlock);

	m_useTplFonts->setChecked(m.useTemplateFonts);
	const FontSpec tf = m.useTemplateFonts ? d.titleFont : m.titleFont;
	const FontSpec xf = m.useTemplateFonts ? d.textFont : m.textFont;
	auto loadFont = [](FontControls &fc, const FontSpec &f) {
		fc.family->setCurrentFont(f.family.isEmpty() ? QApplication::font() : QFont(f.family));
		fc.size->setValue(f.pixelSize);
		fc.bold->setChecked(f.bold);
		fc.italic->setChecked(f.italic);
	};
	loadFont(m_titleFont, tf);
	loadFont(m_textFont, xf);

	m_loading = false;
	updateEnabledStates();
}

void PanelEditor::updateEnabledStates()
{
	const bool has = hasCurrent();
	const PanelModel *m = has ? &draft() : nullptr;

	m_tabs->setEnabled(has);
	m_addBtn->setEnabled(has && m_bridge);
	m_updateBtn->setEnabled(has);
	m_saveBtn->setEnabled(has);
	m_deleteBtn->setEnabled(has);
	m_dupBtn->setEnabled(has);
	m_renameBtn->setEnabled(has);
	m_liveCheck->setEnabled(has);
	if (!has)
		return;

	const bool hasLogo = !m->logoPath.isEmpty();
	m_logoRemove->setEnabled(hasLogo);
	m_logoShow->setEnabled(hasLogo);
	m_logoScale->setEnabled(hasLogo);
	m_logoPadding->setEnabled(hasLogo);
	for (auto *b : m_alignBtn)
		b->setEnabled(hasLogo);

	const bool fit = m->widthMode == WidthMode::FitText;
	m_widthSpin->setEnabled(!fit);
	m_minWidthSpin->setEnabled(fit);
	m_maxWidthSpin->setEnabled(fit);
	m_heightSpin->setEnabled(m->heightMode == HeightMode::Custom);

	for (auto *b : m_colorBtn)
		b->setEnabled(!m->useTemplateColors);
	for (FontControls *fc : {&m_titleFont, &m_textFont}) {
		fc->family->setEnabled(!m->useTemplateFonts);
		fc->size->setEnabled(!m->useTemplateFonts);
		fc->bold->setEnabled(!m->useTemplateFonts);
		fc->italic->setEnabled(!m->useTemplateFonts);
	}
}

void PanelEditor::updateStatus(const QString &message, const QString &state)
{
	QString text = message, st = state;
	if (text.isEmpty()) {
		if (!hasCurrent()) {
			text.clear();
		} else if (isDirty(m_currentId)) {
			text = T("Status.Pending");
			st = QStringLiteral("pending");
		} else {
			text = T("Status.Applied");
			st = QStringLiteral("ok");
		}
	}
	m_status->setText(text);
	m_status->setProperty("state", st);
	m_status->style()->unpolish(m_status);
	m_status->style()->polish(m_status);
}

void PanelEditor::updateLogoThumbnail()
{
	QPixmap pm(46 * 2, 46 * 2);
	pm.setDevicePixelRatio(2.0);
	pm.fill(Qt::transparent);
	if (hasCurrent() && !draft().logoPath.isEmpty()) {
		const LogoSource logo = LogoSource::load(LogoStore::resolve(draft().logoPath));
		if (logo.isValid()) {
			QPainter p(&pm);
			p.setRenderHint(QPainter::Antialiasing, true);
			p.scale(2.0, 2.0);
			logo.paint(p, logo.fitRect(QRectF(5, 5, 36, 36), 1.0, Qt::AlignHCenter));
			p.end();
			m_logoThumb->setPixmap(pm);
			m_logoThumb->setToolTip(draft().logoPath);
			return;
		}
		m_logoThumb->setToolTip(T("Logo.Missing"));
		m_logoThumb->setPixmap(QPixmap());
		m_logoThumb->setText(QStringLiteral("!"));
		return;
	}
	m_logoThumb->setToolTip(QString());
	m_logoThumb->setPixmap(QPixmap());
	m_logoThumb->setText(QStringLiteral("—"));
}

// ================================================================ actions

void PanelEditor::newPanel()
{
	const QString tpl = hasCurrent() ? draft().templateId : QStringLiteral("classic");
	PanelModel m = TemplateRegistry::instance().makeDefaultModel(tpl);
	const QString id = m.id;
	PanelManager::instance().add(m);
	refreshList();
	selectPanel(id);
	m_tabs->setCurrentIndex(0);
	m_titleEdit->setFocus();
	m_titleEdit->selectAll();
}

void PanelEditor::duplicatePanel()
{
	if (!hasCurrent())
		return;
	PanelModel copy = draft(); // duplicate what the operator sees, drafts included
	copy.id = PanelModel::newId();
	copy.name = copy.displayName() + QStringLiteral(" ") + T("Panel.CopySuffix");
	const QString id = copy.id;
	PanelManager::instance().add(copy);
	refreshList();
	selectPanel(id);
}

void PanelEditor::renamePanel()
{
	if (!hasCurrent())
		return;
	bool ok = false;
	const QString name = QInputDialog::getText(this, T("Panels.RenameTitle"), T("Panels.RenamePrompt"), QLineEdit::Normal,
						   draft().displayName(), &ok);
	if (!ok)
		return;
	draft().name = name.trimmed();
	PanelManager::instance().rename(m_currentId, name);
	refreshCurrentListItem();
	updateStatus();
}

void PanelEditor::deletePanel()
{
	if (!hasCurrent())
		return;
	const QString id = m_currentId;
	const int used = m_bridge ? m_bridge->usageCount(id) : 0;
	QString message = T("Delete.Confirm").arg(draft().displayName());
	if (used > 0)
		message += QStringLiteral("\n\n") + T("Delete.InUse").arg(used);

	const auto answer = QMessageBox::question(this, T("Delete.Title"), message, QMessageBox::Yes | QMessageBox::Cancel,
						  QMessageBox::Cancel);
	if (answer != QMessageBox::Yes)
		return;

	if (m_bridge)
		m_bridge->unlinkSources(id);
	m_drafts.remove(id);
	m_currentId.clear();
	PanelManager::instance().remove(id); // emits libraryChanged -> refreshList()

	const auto all = PanelManager::instance().panels();
	selectPanel(all.isEmpty() ? QString() : all.last().id);
	refreshList();
}

void PanelEditor::chooseLogo()
{
	if (!hasCurrent())
		return;
	const QString start = m_lastLogoDir.isEmpty() ? QString() : m_lastLogoDir;
	const QString file = QFileDialog::getOpenFileName(this, T("Logo.PickTitle"), start, LogoStore::dialogFilter());
	if (file.isEmpty())
		return;
	m_lastLogoDir = QFileInfo(file).absolutePath();

	const QString stored = LogoStore::importFile(file);
	if (stored.isEmpty() || !LogoSource::load(LogoStore::resolve(stored)).isValid()) {
		QMessageBox::warning(this, T("Logo.PickTitle"), T("Logo.LoadError"));
		return;
	}
	edit([&](PanelModel &m) {
		m.logoPath = stored;
		m.showLogo = true;
	});
	loadControls();
}

void PanelEditor::commitCurrent(bool notify)
{
	if (!hasCurrent())
		return;
	draft().sanitize();
	PanelManager::instance().update(draft(), notify);
	refreshCurrentListItem();
	updateStatus();
}

void PanelEditor::addToScene()
{
	if (!hasCurrent() || !m_bridge)
		return;
	commitCurrent(true); // add what the operator sees, not an older version
	QString error;
	const QString name = draft().displayName().isEmpty() ? T("Panels.Untitled") : draft().displayName();
	if (m_bridge->addToCurrentScene(m_currentId, name, &error))
		updateStatus(T("Status.Added"), QStringLiteral("ok"));
	else
		updateStatus(error.isEmpty() ? T("Status.AddFailed") : error, QStringLiteral("error"));
}

void PanelEditor::saveNow()
{
	if (!hasCurrent())
		return;
	commitCurrent(true);
	if (PanelManager::instance().save())
		updateStatus(T("Status.Saved"), QStringLiteral("ok"));
	else
		updateStatus(T("Status.SaveFailed"), QStringLiteral("error"));
}

void PanelEditor::showPanel(const QString &id)
{
	if (PanelManager::instance().contains(id))
		selectPanel(id);
}

void PanelEditor::flushDraftsQuietly()
{
	auto &mgr = PanelManager::instance();
	for (auto it = m_drafts.constBegin(); it != m_drafts.constEnd(); ++it) {
		if (mgr.contains(it.key()) && isDirty(it.key()))
			mgr.update(it.value(), /*notify=*/false);
	}
}

} // namespace sp
