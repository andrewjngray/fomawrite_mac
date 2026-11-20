QT += core gui widgets printsupport qml quick quickcontrols2 quickdialogs2
linux: QT += dbus

CONFIG += c++17 release
TARGET = fomawrite
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/documentviewcheck.h \
    src/spellcheck.h \
    src/harperscheme.h \
    src/harperengine.h \
    src/harperupdater.h \
    src/markdownhighlighter.h \
    src/outputcss.h \
    src/systemtheme.h

SOURCES += \
    src/main.cpp \
    src/documentviewcheck.cpp \
    src/spellcheck.cpp \
    src/harperscheme.cpp \
    src/harperengine.cpp \
    src/harperupdater.cpp \
    src/backend.cpp \
    src/markdownhighlighter.cpp \
    src/outputcss.cpp

macx {
    SOURCES += src/systemtheme_mac.cpp
    OBJECTIVE_SOURCES += src/windowchrome_mac.mm
    TARGET = Fomawrite
    QMAKE_INFO_PLIST = macos/Info.plist
    ICON = macos/Fomawrite.icns
    # Expose the dark runtime icon to external consumers such as Codex.
    # The .qrc copy remains for Fomawrite itself; browsers cannot read .icns.
    codex_icon.files = $$PWD/macos/FomawriteRunningIcon.png
    codex_icon.path = Contents/Resources
    QMAKE_BUNDLE_DATA += codex_icon
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.0
} else:win32 {
    SOURCES += src/systemtheme_win.cpp
} else:linux {
    SOURCES += src/systemtheme.cpp
} else {
    error("Unsupported platform: a SystemTheme implementation is required")
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

DISTFILES += src/editoracceptancecheck.inc src/liveacceptancecheck.inc src/panechromeacceptancecheck.inc src/editinglayoutacceptancecheck.inc

QT += pdfquick webenginequick

QT += webenginecore pdf
HEADERS += src/typorabase.h src/publishingthemes.h src/publishinghtml.h src/publishingpdf.h src/publisher.h src/macbridge.h src/editorbridge.h
QT += webchannel
SOURCES += src/typorabase.cpp src/publishingthemes.cpp src/publishinghtml.cpp src/publishingpdf.cpp src/publisher.cpp src/editorbridge.cpp src/vendor/md4c/md4c.c

DISTFILES += src/publishingthemeacceptancecheck.inc src/vendor/md4c/LICENSE.md

DISTFILES += src/publishingfolderacceptancecheck.inc src/pdfacceptancecheck.inc

# PDF reading (Cycle 160)
SOURCES += src/pdfsession.cpp src/pdfpageitem.cpp src/pdfnotes.cpp src/pdfmarks.cpp src/pdffiles.cpp
HEADERS += src/pdfsession.h src/pdfpageitem.h src/pdfnotes.h src/pdfmarks.h src/pdffiles.h

# PDF page operations over qpdf (Cycle 165). qpdf is found through pkg-config
# (Homebrew on macOS; vcpkg's lib/pkgconfig on Windows, with PKG_CONFIG_PATH
# set), or set QPDF_PREFIX=<prefix with include/ and lib/> to link directly.
SOURCES += src/pdfeditor.cpp
HEADERS += src/pdfeditor.h
isEmpty(QPDF_PREFIX): QPDF_PREFIX = $$(QPDF_PREFIX)
!isEmpty(QPDF_PREFIX) {
    INCLUDEPATH += $$QPDF_PREFIX/include
    LIBS += -L$$QPDF_PREFIX/lib -lqpdf
} else {
    # Not PKGCONFIG: that puts every cflag, including qpdf's -std=c++20, on the
    # C compiler line too (md4c.c). Take include paths and libraries, and keep any
    # other flag for C++ only; the app's own C++ standard stays as set above.
    QPDF_CFLAGS = $$system(pkg-config --cflags libqpdf)
    for(flag, QPDF_CFLAGS) {
        contains(flag, ^-I.*): INCLUDEPATH += $$replace(flag, ^-I, )
        else: !contains(flag, ^-std=.*): QMAKE_CXXFLAGS += $$flag
    }
    LIBS += $$system(pkg-config --libs libqpdf)
}
