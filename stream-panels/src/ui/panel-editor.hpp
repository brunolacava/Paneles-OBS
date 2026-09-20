// Stream Panels - the dock.
//
// Editing model: every panel being edited has a *draft* (a private copy).
// Controls modify the draft and the preview follows immediately. Nothing
// reaches OBS until the operator presses UPDATE (or enables "Live update"),
// so text can be prepared off-air. SAVE additionally flushes the library to
// disk; the library is also saved automatically on structural changes and on
// exit.
#pragma once

#include "model/panel-model.hpp"
#include "ui/scene-bridge.hpp"

#include <QHash>
#include <QTimer>
#include <QWidget>

#include <functional>

class QCheckBox;
class QFontComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;
class QTabWidget;
class QToolButton;

namespace sp {

class ColorButton;
class PreviewWidget;
class TemplatePicker;

class PanelEditor : public QWidget {
	Q_OBJECT

public:
	explicit PanelEditor(SceneBridge *bridge, QWidget *parent = nullptr);
	~PanelEditor() override;

	// Selects a panel (used by the source's "Edit panel" button).
	void showPanel(const QString &id);

	// Persists drafts into the library without touching live sources
	// (called when OBS is closing).
	void flushDraftsQuietly();

private:
	struct FontControls {
		QFontComboBox *family = nullptr;
		QSpinBox *size = nullptr;
		QToolButton *bold = nullptr;
		QToolButton *italic = nullptr;
	};

	// ---- construction
	void buildUi();
	QWidget *buildContentTab();
	QWidget *buildSizeTab();
	QWidget *buildStyleTab();
	FontControls buildFontRow(QWidget *parent, QGridLayout *grid, int row, const QString &caption);

	// ---- state
	bool hasCurrent() const { return !m_currentId.isEmpty() && m_drafts.contains(m_currentId); }
	PanelModel &draft() { return m_drafts[m_currentId]; }
	bool isDirty(const QString &id) const;

	void edit(const std::function<void(PanelModel &)> &change);
	void selectPanel(const QString &id);
	void refreshList();
	void refreshCurrentListItem();
	void loadControls();
	void updateEnabledStates();
	void updateStatus(const QString &message = QString(), const QString &state = QString());
	void updateLogoThumbnail();
	void schedulePreview();

	// ---- actions
	void newPanel();
	void duplicatePanel();
	void renamePanel();
	void deletePanel();
	void chooseLogo();
	void commitCurrent(bool notify);
	void addToScene();
	void saveNow();

	SceneBridge *m_bridge = nullptr;

	QString m_currentId;
	QHash<QString, PanelModel> m_drafts;
	bool m_loading = false;
	QString m_lastLogoDir;
	QTimer m_previewTimer;
	QTimer m_liveTimer;

	// widgets
	QListWidget *m_list = nullptr;
	QPushButton *m_newBtn = nullptr;
	QPushButton *m_dupBtn = nullptr;
	QPushButton *m_renameBtn = nullptr;
	PreviewWidget *m_preview = nullptr;
	QTabWidget *m_tabs = nullptr;

	TemplatePicker *m_picker = nullptr;
	QLineEdit *m_titleEdit = nullptr;
	QCheckBox *m_titleShow = nullptr;
	QCheckBox *m_titleUpper = nullptr;
	QPlainTextEdit *m_textEdit = nullptr;
	QCheckBox *m_textShow = nullptr;
	QLabel *m_logoThumb = nullptr;
	QPushButton *m_logoSelect = nullptr;
	QPushButton *m_logoRemove = nullptr;
	QCheckBox *m_logoShow = nullptr;
	QSlider *m_logoScale = nullptr;
	QLabel *m_logoScaleLabel = nullptr;
	QSpinBox *m_logoPadding = nullptr;
	QToolButton *m_alignBtn[3] = {nullptr, nullptr, nullptr};

	QRadioButton *m_widthFit = nullptr;
	QRadioButton *m_widthCustom = nullptr;
	QSpinBox *m_widthSpin = nullptr;
	QSpinBox *m_minWidthSpin = nullptr;
	QSpinBox *m_maxWidthSpin = nullptr;
	QRadioButton *m_heightAuto = nullptr;
	QRadioButton *m_heightCustom = nullptr;
	QSpinBox *m_heightSpin = nullptr;
	QSpinBox *m_paddingSpin = nullptr;

	QCheckBox *m_useTplColors = nullptr;
	ColorButton *m_colorBtn[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	QCheckBox *m_useTplFonts = nullptr;
	FontControls m_titleFont;
	FontControls m_textFont;

	QPushButton *m_addBtn = nullptr;
	QPushButton *m_updateBtn = nullptr;
	QPushButton *m_saveBtn = nullptr;
	QPushButton *m_deleteBtn = nullptr;
	QCheckBox *m_liveCheck = nullptr;
	QLabel *m_status = nullptr;
};

} // namespace sp
