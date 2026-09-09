QT += core gui widgets printsupport qml quick quickcontrols2 quickdialogs2
!macx: QT += dbus

CONFIG += c++17 release
TARGET = omawrite
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/markdownhighlighter.h \
    src/systemtheme.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/markdownhighlighter.cpp

macx {
    SOURCES += src/systemtheme_mac.cpp
    TARGET = Omawrite
    QMAKE_INFO_PLIST = macos/Info.plist
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
} else {
    SOURCES += src/systemtheme.cpp
}

RESOURCES += src/resources.qrc
