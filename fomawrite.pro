QT += core gui widgets printsupport qml quick quickcontrols2 quickdialogs2
!macx: QT += dbus

CONFIG += c++17 release
TARGET = fomawrite
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/markdownhighlighter.h \
    src/sourcevisualmapping.h \
    src/visualtexthighlighter.h \
    src/outputcss.h \
    src/systemtheme.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/markdownhighlighter.cpp \
    src/sourcevisualmapping.cpp \
    src/visualtexthighlighter.cpp \
    src/outputcss.cpp

macx {
    SOURCES += src/systemtheme_mac.cpp
    OBJECTIVE_SOURCES += src/windowchrome_mac.mm
    TARGET = Fomawrite
    QMAKE_INFO_PLIST = macos/Info.plist
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
} else {
    SOURCES += src/systemtheme.cpp
}

RESOURCES += src/resources.qrc

SOURCES += src/filelibrary.cpp
HEADERS += src/filelibrary.h

QT += concurrent

SOURCES += src/markdownextensions.cpp
HEADERS += src/markdownextensions.h

QT += network
SOURCES += src/workspace.cpp
HEADERS += src/workspace.h
