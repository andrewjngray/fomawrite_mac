QT += core gui widgets printsupport qml quick quickcontrols2 quickdialogs2
!macx: QT += dbus

CONFIG += c++17 release
TARGET = fomawrite
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/documentviewcheck.h \
    src/markdownhighlighter.h \
    src/outputcss.h \
    src/systemtheme.h

SOURCES += \
    src/main.cpp \
    src/documentviewcheck.cpp \
    src/backend.cpp \
    src/markdownhighlighter.cpp \
    src/outputcss.cpp

macx {
    SOURCES += src/systemtheme_mac.cpp
    OBJECTIVE_SOURCES += src/windowchrome_mac.mm
    TARGET = Fomawrite
    QMAKE_INFO_PLIST = macos/Info.plist
    ICON = macos/Fomawrite.icns
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

DISTFILES += src/editoracceptancecheck.inc src/panechromeacceptancecheck.inc src/editinglayoutacceptancecheck.inc

QT += pdfquick webenginequick

QT += webenginecore pdf
HEADERS += src/publishingthemes.h src/publishinghtml.h src/publishingpdf.h src/publisher.h src/macbridge.h src/editorbridge.h
QT += webchannel
SOURCES += src/publishingthemes.cpp src/publishinghtml.cpp src/publishingpdf.cpp src/publisher.cpp src/editorbridge.cpp src/vendor/md4c/md4c.c

DISTFILES += src/publishingthemeacceptancecheck.inc src/vendor/md4c/LICENSE.md

DISTFILES += src/publishingfolderacceptancecheck.inc
