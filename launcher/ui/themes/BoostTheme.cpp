// SPDX-License-Identifier: GPL-3.0-only
#include "BoostTheme.h"

#include <QColor>
#include <QObject>

QString BoostTheme::id()
{
    return "boostclient";
}

QString BoostTheme::name()
{
    return QObject::tr("BoostClient");
}

QString BoostTheme::tooltip()
{
    return QObject::tr("A dark gaming theme with a cool teal accent.");
}

QString BoostTheme::appStyleSheet()
{
    return "QToolTip { color: #f1f7ff; background-color: #172337; border: 1px solid #47d7b8; padding: 5px; }"
           "QMainWindow, QDialog { background-color: #101521; }"
           "QToolBar { background-color: #131a27; border: 0; border-bottom: 1px solid #2b384b; spacing: 6px; padding: 4px; }"
           "QToolButton { color: #dfe8f3; background: transparent; border: 1px solid transparent; "
           "border-radius: 5px; padding: 6px; }"
           "QToolButton:hover { background-color: #202b3c; border-color: #344258; }"
           "QToolButton:checked { background-color: #173a37; border-color: #47d7b8; }"
           "QPushButton { color: #f1f7ff; background-color: #202b3c; border: 1px solid #344258; "
           "border-radius: 5px; padding: 6px 10px; }"
           "QPushButton:hover { background-color: #29394d; border-color: #47d7b8; }"
           "QPushButton:pressed { background-color: #163d3a; }"
           "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit, QPlainTextEdit { "
           "color: #e7edf5; background-color: #171e2b; border: 1px solid #344258; border-radius: 4px; "
           "padding: 4px; selection-background-color: #47d7b8; selection-color: #0b1820; }"
           "QTreeView::item:selected, QListView::item:selected, QTableView::item:selected { "
           "background-color: #173a37; color: #f1f7ff; }"
           "QHeaderView::section { background-color: #202b3c; border: 0; border-bottom: 1px solid #344258; "
           "padding: 5px; }"
           "QProgressBar { border: 1px solid #344258; border-radius: 4px; background-color: #151b28; "
           "text-align: center; color: #f1f7ff; }"
           "QProgressBar::chunk { background-color: #47d7b8; border-radius: 3px; }";
}

QPalette BoostTheme::colorScheme()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#101521"));
    palette.setColor(QPalette::WindowText, QColor("#e9f1fa"));
    palette.setColor(QPalette::Base, QColor("#171e2b"));
    palette.setColor(QPalette::AlternateBase, QColor("#1d2736"));
    palette.setColor(QPalette::ToolTipBase, QColor("#172337"));
    palette.setColor(QPalette::ToolTipText, QColor("#f1f7ff"));
    palette.setColor(QPalette::Text, QColor("#e7edf5"));
    palette.setColor(QPalette::Button, QColor("#202b3c"));
    palette.setColor(QPalette::ButtonText, QColor("#f1f7ff"));
    palette.setColor(QPalette::BrightText, QColor("#ff687a"));
    palette.setColor(QPalette::Link, QColor("#77b8ff"));
    palette.setColor(QPalette::Highlight, QColor("#47d7b8"));
    palette.setColor(QPalette::HighlightedText, QColor("#0b1820"));
    palette.setColor(QPalette::PlaceholderText, QColor("#8493a6"));
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double BoostTheme::fadeAmount()
{
    return 0.42;
}

QColor BoostTheme::fadeColor()
{
    return QColor("#101521");
}
