#include "systemtheme.h"

#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

// Qt's Windows platform theme follows the user's app colour preference and
// emits colorSchemeChanged on changes. Qt already applies display DPI scaling;
// applying a second DPI multiplier here would enlarge the interface twice.
SystemTheme::SystemTheme(QObject *parent) : QObject(parent) {
    refresh();
    if (QGuiApplication::styleHints())
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                this, &SystemTheme::refresh);
}

void SystemTheme::refresh() {
    setDarkMode(detectDarkMode());
}

bool SystemTheme::detectDarkMode() const {
    bool known = false;
    const bool dark = qtDarkMode(&known);
    return known ? dark : QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

bool SystemTheme::qtDarkMode(bool *known) const {
    *known = false;
    if (!QGuiApplication::styleHints()) return false;
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    *known = scheme == Qt::ColorScheme::Dark || scheme == Qt::ColorScheme::Light;
    return scheme == Qt::ColorScheme::Dark;
}

void SystemTheme::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode) return;
    m_darkMode = darkMode;
    emit darkModeChanged(m_darkMode);
}
