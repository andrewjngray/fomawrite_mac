#include "systemtheme.h"

#include <QGuiApplication>
#include <QStyleHints>

// macOS exposes appearance through Qt; it has no XDG settings portal.
SystemTheme::SystemTheme(QObject *parent) : QObject(parent) {
    refresh();
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &SystemTheme::refresh);
}

void SystemTheme::refresh() {
    setDarkMode(QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark);
}

void SystemTheme::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;
    m_darkMode = darkMode;
    emit darkModeChanged(m_darkMode);
}
