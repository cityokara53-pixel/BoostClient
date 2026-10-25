// SPDX-License-Identifier: GPL-3.0-only
#include "BoostTheme.h"

#include <QColor>
#include <QObject>
#include <utility>

namespace {
struct ThemeColors {
    QString background;
    QString sidebar;
    QString surface;
    QString elevated;
    QString border;
    QString accent;
    QString blue;
    QString violet;
    QString magenta;
    QString text;
    QString muted;
    QString selected;
    QString onAccent;
    QString heroEnd;
    QString link;
};

ThemeColors colorsFor(const QString& id)
{
    if (id == "violet-phantom") {
        return { "#100D1C", "#171126", "#211833", "#2B2042", "#413353", "#B47CFF", "#7667FF", "#9466FF",
                 "#E66DFF", "#F7F3FF", "#B4A8CC", "#302248", "#190F29", "#4A2762", "#D4A4FF" };
    }
    if (id == "crimson-velocity") {
        return { "#160D14", "#201018", "#2B1721", "#39202A", "#50303B", "#FF6B78", "#E84C71", "#A64D80",
                 "#FF9A68", "#FFF5F6", "#C5A7AE", "#40212A", "#2C1118", "#5A2736", "#FF8794" };
    }
    if (id == "emerald-reactor") {
        return { "#091511", "#0E1D18", "#14271F", "#1B352A", "#2B4A3D", "#42D9A0", "#30B8C6", "#4C9D9A",
                 "#8DDC78", "#F2FCF7", "#A2BDB1", "#1A3A30", "#09251A", "#1D5045", "#65E3C2" };
    }
    if (id == "arctic-light") {
        return { "#EEF3FA", "#E3EAF4", "#FFFFFF", "#F7FAFF", "#CAD5E4", "#1769E0", "#4E74E8", "#7556CF",
                 "#BD4AAE", "#17243A", "#5D6C82", "#DDEBFF", "#FFFFFF", "#DCE7F7", "#155CC2" };
    }
    return { "#0A0A0A", "#070707", "#141414", "#1D1D1D", "#303030", "#F28A1D", "#E87513", "#FFB347",
             "#F26B38", "#F5F5F5", "#A8A8A8", "#242424", "#101010", "#292929", "#FF9C3D" };
}

QString themeName(const QString& id)
{
    if (id == "violet-phantom")
        return QObject::tr("Violet Phantom");
    if (id == "crimson-velocity")
        return QObject::tr("Crimson Velocity");
    if (id == "emerald-reactor")
        return QObject::tr("Emerald Reactor");
    if (id == "arctic-light")
        return QObject::tr("Arctic Light");
    return QObject::tr("OG Client");
}
}  // namespace

BoostTheme::BoostTheme(QString themeId) : m_themeId(std::move(themeId)) {}

QString BoostTheme::id()
{
    return m_themeId;
}

QString BoostTheme::name()
{
    return themeName(m_themeId);
}

QString BoostTheme::tooltip()
{
    if (m_themeId == "arctic-light")
        return QObject::tr("A bright, clean interface with crisp blue accents.");
    if (m_themeId == "violet-phantom")
        return QObject::tr("A dark violet interface with vivid purple highlights.");
    if (m_themeId == "crimson-velocity")
        return QObject::tr("A dark crimson interface with warm, restrained highlights.");
    if (m_themeId == "emerald-reactor")
        return QObject::tr("A deep green interface with energetic teal accents.");
    return QObject::tr("A dark interface with orange accents.");
}

QString BoostTheme::appStyleSheet()
{
    const auto c = colorsFor(m_themeId);
    QString style = QStringLiteral(R"qss(
        QMainWindow {
            color: @text;
            background: @background;
        }
        QWidget {
            color: @text;
        }
        QDialog {
            color: @text;
            background: @surface;
            border: 1px solid @border;
        }
        QToolTip {
            color: @text;
            background-color: @elevated;
            border: 1px solid @border;
            border-radius: 7px;
            padding: 6px 9px;
        }
        QMenuBar {
            color: @muted;
            background-color: @sidebar;
            border-bottom: 1px solid @border;
            padding: 4px 8px;
        }
        QMenuBar::item {
            background: transparent;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QMenuBar::item:selected, QMenuBar::item:pressed {
            color: @text;
            background-color: @elevated;
        }
        QMenu {
            color: @text;
            background-color: @surface;
            border: 1px solid @border;
            padding: 5px;
        }
        QMenu::item {
            border-radius: 5px;
            padding: 7px 30px 7px 10px;
        }
        QMenu::item:selected {
            color: @accent;
            background-color: @selected;
        }
        QMenu::separator {
            height: 1px;
            background-color: @border;
            margin: 5px 7px;
        }
        QToolBar {
            color: @text;
            background-color: @sidebar;
            border: 0;
            border-bottom: 1px solid @border;
            spacing: 7px;
            padding: 8px 12px;
        }
        QToolBar::separator {
            width: 1px;
            background-color: @border;
            margin: 5px 7px;
        }
        QToolButton {
            color: @muted;
            background: transparent;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 7px 9px;
        }
        QToolButton:hover, QToolButton:pressed {
            color: @text;
            background-color: @elevated;
            border-color: @border;
        }
        QToolButton:checked {
            color: @accent;
            background-color: @selected;
        }
        QToolButton:disabled {
            color: @muted;
        }
        QToolButton:focus {
            border-color: @accent;
        }
        QStatusBar {
            color: @muted;
            background-color: @sidebar;
            border-top: 1px solid @border;
            padding: 4px 8px;
        }
        QStatusBar QLabel {
            color: @muted;
            background: transparent;
        }
        QStatusBar::item {
            border: 0;
        }
        QPushButton {
            color: @text;
            background-color: @elevated;
            border: 1px solid @border;
            border-radius: 8px;
            padding: 8px 13px;
            min-height: 18px;
        }
        QPushButton:hover {
            background-color: @selected;
            border-color: @accent;
        }
        QPushButton:pressed {
            background-color: @surface;
        }
        QPushButton:focus {
            border-color: @accent;
        }
        QPushButton:disabled {
            color: @muted;
            background-color: @surface;
            border-color: @border;
        }
        QPushButton#boostPlayButton {
            color: @onAccent;
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 @accent, stop:1 @blue);
            border: 0;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 700;
            padding: 10px 25px;
        }
        QPushButton#boostPlayButton:hover {
            color: @onAccent;
            background-color: @link;
        }
        QPushButton#boostPlayButton:pressed {
            background-color: @blue;
        }
        QPushButton#boostPlayButton:disabled {
            color: @muted;
            background-color: @elevated;
        }
        QPushButton#boostSecondaryButton {
            background-color: @surface;
        }
        QWidget#boostSidebar {
            background-color: @sidebar;
            border-right: 1px solid @border;
        }
        QScrollArea#boostSidebarScroll {
            background-color: @sidebar;
            border: 0;
        }
        QLabel#boostBrandMark {
            background: @surface;
            color: @text;
            border: 1px solid @border;
            border-radius: 9px;
            font-size: 12px;
            font-weight: 800;
        }
        QLabel#boostBrand {
            color: @text;
            background: transparent;
            font-size: 15px;
            font-weight: 800;
            letter-spacing: 1px;
        }
        QLabel#boostBrandCaption, QLabel#boostNavGroup, QLabel#boostKicker {
            color: @muted;
            background: transparent;
            font-size: 9px;
            font-weight: 700;
            letter-spacing: 1px;
        }
        QToolButton#boostNavButton {
            color: @muted;
            text-align: left;
            background: transparent;
            border: 1px solid transparent;
            border-left: 2px solid transparent;
            border-radius: 8px;
            padding: 9px 12px;
            min-height: 20px;
        }
        QToolButton#boostNavButton:hover {
            color: @text;
            background-color: @surface;
            border-color: @border;
            border-left-color: @accent;
        }
        QToolButton#boostNavButton:checked {
            color: @text;
            background-color: @selected;
            border-color: @border;
            border-left-color: @text;
            font-weight: 700;
        }
        QLabel#boostHomeTitle {
            color: @text;
            background: transparent;
            font-size: 31px;
            font-weight: 700;
        }
        QLabel#boostHomeIntro, QLabel#boostProfileStatus, QLabel#boostPanelCaption {
            color: @muted;
            background: transparent;
            font-size: 12px;
        }
        QLabel#boostProfileName {
            color: @text;
            background: transparent;
            font-size: 21px;
            font-weight: 700;
        }
        QLabel#boostSectionTitle {
            color: @text;
            background: transparent;
            font-size: 15px;
            font-weight: 700;
        }
        QFrame#boostHero {
            background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 @surface, stop:0.58 @elevated, stop:1 @heroEnd);
            border: 1px solid @border;
            border-radius: 18px;
        }
        QLabel#boostHeroArtwork {
            background-color: qlineargradient(x1:0, y1:1, x2:1, y2:0, stop:0 @violet, stop:0.4 @magenta, stop:0.72 @blue, stop:1 @accent);
            border: 1px solid @link;
            border-radius: 18px;
        }
        QComboBox#boostProfileSelector {
            background-color: @elevated;
            border-color: @link;
            font-weight: 600;
            min-height: 24px;
            padding: 8px 10px;
        }
        QFrame#boostDashboardPanel {
            background-color: @surface;
            border: 1px solid @border;
            border-radius: 13px;
        }
        QLabel#boostPanelMetric {
            color: @text;
            background: transparent;
            font-size: 23px;
            font-weight: 700;
        }
        QLabel#boostLibraryTitle {
            color: @text;
            background: transparent;
            font-size: 25px;
            font-weight: 700;
        }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateEdit, QDateTimeEdit, QTextEdit, QPlainTextEdit {
            color: @text;
            background-color: @surface;
            border: 1px solid @border;
            border-radius: 7px;
            padding: 6px 8px;
            selection-background-color: @selected;
            selection-color: @text;
        }
        QLineEdit:hover, QComboBox:hover, QSpinBox:hover, QDoubleSpinBox:hover,
        QDateEdit:hover, QDateTimeEdit:hover, QTextEdit:hover, QPlainTextEdit:hover {
            border-color: @violet;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus,
        QDateEdit:focus, QDateTimeEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: @accent;
        }
        QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled,
        QDateEdit:disabled, QDateTimeEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {
            color: @muted;
            background-color: @background;
        }
        QComboBox::drop-down, QDateEdit::drop-down, QDateTimeEdit::drop-down {
            width: 24px;
            border: 0;
            border-left: 1px solid @border;
        }
        QComboBox QAbstractItemView {
            color: @text;
            background-color: @surface;
            border: 1px solid @border;
            selection-background-color: @selected;
        }
        QAbstractItemView {
            color: @text;
            background-color: @sidebar;
            alternate-background-color: @surface;
            show-decoration-selected: 1;
            selection-background-color: @selected;
            selection-color: @text;
        }
        QTreeView::item, QListView::item, QTableView::item {
            padding: 5px;
            border-radius: 4px;
        }
        QTreeView::item:hover, QListView::item:hover, QTableView::item:hover {
            background-color: @elevated;
        }
        QTreeView::item:selected, QListView::item:selected, QTableView::item:selected {
            background-color: @selected;
            color: @text;
        }
        QHeaderView::section {
            color: @muted;
            background-color: @surface;
            border: 0;
            border-right: 1px solid @border;
            border-bottom: 1px solid @border;
            padding: 8px 10px;
            font-weight: 600;
        }
        QTabWidget::pane {
            background-color: @background;
            border: 1px solid @border;
            border-radius: 7px;
            top: -1px;
        }
        QTabBar::tab {
            color: @muted;
            background-color: @sidebar;
            border: 1px solid transparent;
            border-bottom: 2px solid transparent;
            padding: 9px 13px;
            margin-right: 3px;
        }
        QTabBar::tab:hover {
            color: @text;
            background-color: @surface;
        }
        QTabBar::tab:selected {
            color: @accent;
            background-color: @surface;
            border-bottom-color: @accent;
        }
        QGroupBox {
            color: @text;
            background: transparent;
            border: 1px solid @border;
            border-radius: 8px;
            margin-top: 12px;
            padding: 12px 9px 8px;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
            color: @muted;
            background-color: @background;
        }
        QCheckBox, QRadioButton {
            color: @text;
            background: transparent;
            spacing: 8px;
        }
        QCheckBox:disabled, QRadioButton:disabled {
            color: @muted;
        }
        QCheckBox::indicator, QRadioButton::indicator {
            width: 15px;
            height: 15px;
            background-color: @surface;
            border: 1px solid @border;
        }
        QCheckBox::indicator {
            border-radius: 4px;
        }
        QRadioButton::indicator {
            border-radius: 8px;
        }
        QCheckBox::indicator:hover, QRadioButton::indicator:hover {
            border-color: @accent;
        }
        QCheckBox::indicator:checked, QRadioButton::indicator:checked {
            background-color: @accent;
            border-color: @accent;
        }
        QProgressBar {
            color: @text;
            background-color: @surface;
            border: 1px solid @border;
            border-radius: 5px;
            text-align: center;
            min-height: 10px;
        }
        QProgressBar::chunk {
            background-color: @accent;
            border-radius: 4px;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            background: transparent;
            margin: 2px;
        }
        QScrollBar:vertical {
            width: 10px;
        }
        QScrollBar:horizontal {
            height: 10px;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background-color: @border;
            border-radius: 4px;
            min-width: 24px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {
            background-color: @muted;
        }
        QScrollBar::add-line, QScrollBar::sub-line {
            width: 0;
            height: 0;
            border: none;
            background: none;
        }
        QScrollBar::add-page, QScrollBar::sub-page {
            background: none;
        }
        QSlider::groove:horizontal {
            height: 4px;
            background-color: @border;
            border-radius: 2px;
        }
        QSlider::sub-page:horizontal {
            background-color: @accent;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            width: 14px;
            margin: -5px 0;
            background-color: @text;
            border: 2px solid @accent;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background-color: @accent;
        }
        QSplitter::handle {
            background-color: @elevated;
        }
        QSplitter::handle:hover {
            background-color: @accent;
        }
        QFrame#boostHomeScroll, QScrollArea#boostHomeScroll {
            background: transparent;
            border: 0;
        }
    )qss");

    style.replace("@background", c.background);
    style.replace("@sidebar", c.sidebar);
    style.replace("@surface", c.surface);
    style.replace("@elevated", c.elevated);
    style.replace("@border", c.border);
    style.replace("@accent", c.accent);
    style.replace("@blue", c.blue);
    style.replace("@violet", c.violet);
    style.replace("@magenta", c.magenta);
    style.replace("@text", c.text);
    style.replace("@muted", c.muted);
    style.replace("@selected", c.selected);
    style.replace("@onAccent", c.onAccent);
    style.replace("@heroEnd", c.heroEnd);
    style.replace("@link", c.link);
    return style;
}

QPalette BoostTheme::colorScheme()
{
    const auto c = colorsFor(m_themeId);
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(c.background));
    palette.setColor(QPalette::WindowText, QColor(c.text));
    palette.setColor(QPalette::Base, QColor(c.sidebar));
    palette.setColor(QPalette::AlternateBase, QColor(c.surface));
    palette.setColor(QPalette::ToolTipBase, QColor(c.elevated));
    palette.setColor(QPalette::ToolTipText, QColor(c.text));
    palette.setColor(QPalette::Text, QColor(c.text));
    palette.setColor(QPalette::Button, QColor(c.elevated));
    palette.setColor(QPalette::ButtonText, QColor(c.text));
    palette.setColor(QPalette::BrightText, QColor("#FF6885"));
    palette.setColor(QPalette::Link, QColor(c.link));
    palette.setColor(QPalette::Highlight, QColor(c.selected));
    palette.setColor(QPalette::HighlightedText, QColor(c.text));
    palette.setColor(QPalette::PlaceholderText, QColor(c.muted));
    return fadeInactive(palette, fadeAmount(), QColor(c.background));
}

double BoostTheme::fadeAmount()
{
    return m_themeId == "arctic-light" ? 0.15 : 0.22;
}

QColor BoostTheme::fadeColor()
{
    return QColor(colorsFor(m_themeId).background);
}
