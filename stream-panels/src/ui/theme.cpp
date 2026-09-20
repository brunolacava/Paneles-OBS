#include "ui/theme.hpp"

namespace sp::ui {

// Design tokens
//   bg        #14161A   surface   #1B1E25   raised    #232733
//   border    #2E3341   text      #E6E8EE   muted     #8B93A7
//   accent    #3B82F6   danger    #EF4444   ok        #22C55E   warn #F59E0B
QString styleSheet()
{
	return QStringLiteral(R"CSS(
#StreamPanelsRoot { background: #14161A; color: #E6E8EE; }
#StreamPanelsRoot QWidget { color: #E6E8EE; font-size: 12px; }
#StreamPanelsRoot QScrollArea, #StreamPanelsRoot #Content { background: transparent; border: none; }

#StreamPanelsRoot QLabel { background: transparent; }
#StreamPanelsRoot #Header { font-size: 13px; font-weight: 700; letter-spacing: 2px; color: #E6E8EE; }
#StreamPanelsRoot #SectionTitle { padding-top: 3px; font-size: 10px; font-weight: 700; letter-spacing: 1.5px; color: #8B93A7; }
#StreamPanelsRoot #FieldCaption { font-size: 11px; color: #8B93A7; }
#StreamPanelsRoot #Status { font-size: 11px; color: #8B93A7; }
#StreamPanelsRoot #Status[state="pending"] { color: #F59E0B; }
#StreamPanelsRoot #Status[state="ok"] { color: #22C55E; }
#StreamPanelsRoot #Status[state="error"] { color: #EF4444; }

#StreamPanelsRoot QLineEdit, #StreamPanelsRoot QPlainTextEdit, #StreamPanelsRoot QSpinBox,
#StreamPanelsRoot QComboBox {
	background: #1B1E25; border: 1px solid #2E3341; border-radius: 6px; padding: 5px 8px;
	selection-background-color: #3B82F6; selection-color: #FFFFFF;
}
#StreamPanelsRoot QLineEdit:focus, #StreamPanelsRoot QPlainTextEdit:focus, #StreamPanelsRoot QSpinBox:focus,
#StreamPanelsRoot QComboBox:focus { border: 1px solid #3B82F6; }
#StreamPanelsRoot QLineEdit:disabled, #StreamPanelsRoot QSpinBox:disabled, #StreamPanelsRoot QComboBox:disabled,
#StreamPanelsRoot QPlainTextEdit:disabled { color: #5A6275; background: #171A20; }
#StreamPanelsRoot QComboBox QAbstractItemView { background: #1B1E25; border: 1px solid #2E3341; selection-background-color: #3B82F6; }

#StreamPanelsRoot QListWidget {
	background: #1B1E25; border: 1px solid #2E3341; border-radius: 8px; outline: none; padding: 3px;
}
#StreamPanelsRoot QListWidget::item { border-radius: 6px; }

#StreamPanelsRoot QTabWidget::pane { border: 1px solid #2E3341; border-radius: 8px; background: #1B1E25; top: -1px; }
#StreamPanelsRoot QTabBar { background: transparent; }
#StreamPanelsRoot QTabBar::tab {
	background: transparent; color: #8B93A7; padding: 7px 14px; border: none; border-bottom: 2px solid transparent;
	font-weight: 600;
}
#StreamPanelsRoot QTabBar::tab:selected { color: #E6E8EE; border-bottom: 2px solid #3B82F6; }
#StreamPanelsRoot QTabBar::tab:hover:!selected { color: #C5CAD6; }

#StreamPanelsRoot QPushButton, #StreamPanelsRoot QToolButton {
	background: #232733; border: 1px solid #2E3341; border-radius: 6px; padding: 6px 12px; font-weight: 600;
}
#StreamPanelsRoot QPushButton:hover, #StreamPanelsRoot QToolButton:hover { background: #2B3040; }
#StreamPanelsRoot QPushButton:pressed, #StreamPanelsRoot QToolButton:pressed { background: #1D212B; }
#StreamPanelsRoot QPushButton:disabled, #StreamPanelsRoot QToolButton:disabled { color: #5A6275; background: #191C23; }
#StreamPanelsRoot QPushButton#Primary { background: #3B82F6; border-color: #3B82F6; color: #FFFFFF; padding: 9px 12px; }
#StreamPanelsRoot QPushButton#Primary:hover { background: #4F8FF7; }
#StreamPanelsRoot QPushButton#Primary:disabled { background: #22304A; border-color: #22304A; color: #6F7B94; }
#StreamPanelsRoot QPushButton#Danger { color: #F87171; }
#StreamPanelsRoot QPushButton#Danger:hover { background: #3A1E24; border-color: #7F1D1D; }
#StreamPanelsRoot QToolButton[templateCard="true"] {
	background: #1B1E25; border: 2px solid #2E3341; border-radius: 8px; padding: 6px 4px 4px 4px; font-weight: 500;
	color: #8B93A7; font-size: 11px;
}
#StreamPanelsRoot QToolButton[templateCard="true"]:hover { border-color: #46506A; }
#StreamPanelsRoot QToolButton[templateCard="true"]:checked { border-color: #3B82F6; background: #1E2636; color: #E6E8EE; }
#StreamPanelsRoot QToolButton[segment="true"] { padding: 4px 10px; border-radius: 0; margin: 0; }
#StreamPanelsRoot QToolButton[segment="true"]:checked { background: #3B82F6; border-color: #3B82F6; color: white; }

#StreamPanelsRoot QCheckBox, #StreamPanelsRoot QRadioButton { spacing: 7px; background: transparent; }
#StreamPanelsRoot QCheckBox::indicator, #StreamPanelsRoot QRadioButton::indicator {
	width: 14px; height: 14px; border: 1px solid #46506A; background: #1B1E25;
}
#StreamPanelsRoot QCheckBox::indicator { border-radius: 4px; }
#StreamPanelsRoot QRadioButton::indicator { border-radius: 8px; }
#StreamPanelsRoot QCheckBox::indicator:checked { background: #E6E8EE; border: 4px solid #3B82F6; }
#StreamPanelsRoot QRadioButton::indicator:checked { background: #3B82F6; border: 4px solid #1B1E25; outline: 1px solid #3B82F6; }
#StreamPanelsRoot QCheckBox::indicator:disabled, #StreamPanelsRoot QRadioButton::indicator:disabled { background: #171A20; border-color: #2E3341; }

#StreamPanelsRoot QSlider::groove:horizontal { height: 4px; background: #2E3341; border-radius: 2px; }
#StreamPanelsRoot QSlider::sub-page:horizontal { background: #3B82F6; border-radius: 2px; }
#StreamPanelsRoot QSlider::handle:horizontal { background: #E6E8EE; width: 12px; margin: -5px 0; border-radius: 6px; }

#StreamPanelsRoot QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
#StreamPanelsRoot QScrollBar::handle:vertical { background: #2E3341; border-radius: 5px; min-height: 30px; }
#StreamPanelsRoot QScrollBar::handle:vertical:hover { background: #46506A; }
#StreamPanelsRoot QScrollBar::add-line:vertical, #StreamPanelsRoot QScrollBar::sub-line:vertical { height: 0; }
#StreamPanelsRoot QFrame#Separator { background: #2E3341; max-height: 1px; min-height: 1px; border: none; }
)CSS");
}

} // namespace sp::ui
