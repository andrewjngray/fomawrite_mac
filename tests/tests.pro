QT += core gui quick testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_fomawrite

# Native ARM64 hit MSVC C1128: the monolithic test source exceeds the
# standard COFF section limit. Keep every case and enable larger objects.
win32:contains(QMAKE_COMPILER, msvc) {
    QMAKE_CXXFLAGS += /bigobj
}

INCLUDEPATH += ../src
SOURCES += \
    tst_fomawrite.cpp \
    ../src/backend.cpp \
    ../src/spellcheck.cpp \
    ../src/harperscheme.cpp \
    ../src/harperengine.cpp \
    ../src/harperupdater.cpp \
    ../src/markdownhighlighter.cpp \
    ../src/outputcss.cpp
HEADERS += \
    ../src/backend.h \
    ../src/spellcheck.h \
    ../src/harperscheme.h \
    ../src/harperengine.h \
    ../src/harperupdater.h \
    ../src/markdownhighlighter.h \
    ../src/outputcss.h

QT += widgets printsupport quickcontrols2 quickdialogs2
CONFIG -= app_bundle

SOURCES += ../src/filelibrary.cpp
HEADERS += ../src/filelibrary.h

QT += concurrent

SOURCES += ../src/markdownextensions.cpp
HEADERS += ../src/markdownextensions.h

macx {
    OBJECTIVE_SOURCES += ../src/windowchrome_mac.mm
    LIBS += -framework AppKit
}

QT += network
SOURCES += ../src/workspace.cpp
HEADERS += ../src/workspace.h
RESOURCES += ../src/resources.qrc

QT += pdf pdfquick webenginequick

QT += webenginecore pdf
HEADERS += ../src/typorabase.h ../src/publishingthemes.h ../src/publishinghtml.h ../src/publishingpdf.h ../src/publisher.h ../src/macbridge.h ../src/editorbridge.h
QT += webchannel
SOURCES += ../src/typorabase.cpp ../src/publishingthemes.cpp ../src/publishinghtml.cpp ../src/publishingpdf.cpp ../src/publisher.cpp ../src/editorbridge.cpp ../src/vendor/md4c/md4c.c

# Exercise the same appearance implementation selected by the application.
HEADERS += ../src/systemtheme.h
macx {
    SOURCES += ../src/systemtheme_mac.cpp
} else:win32 {
    SOURCES += ../src/systemtheme_win.cpp
} else:linux {
    QT += dbus
    SOURCES += ../src/systemtheme.cpp
}
# PDF reading (Cycle 160)
SOURCES += ../src/pdfsession.cpp ../src/pdfpageitem.cpp ../src/pdfnotes.cpp ../src/pdfmarks.cpp ../src/pdffiles.cpp
HEADERS += ../src/pdfsession.h ../src/pdfpageitem.h ../src/pdfnotes.h ../src/pdfmarks.h ../src/pdffiles.h
HEADERS += pdffixtures.h
win32: LIBS += -lpsapi

# PDF page operations over qpdf (Cycle 165). qpdf is found through pkg-config
# (Homebrew on macOS; vcpkg's lib/pkgconfig on Windows, with PKG_CONFIG_PATH
# set), or set QPDF_PREFIX=<prefix with include/ and lib/> to link directly.
SOURCES += ../src/pdfeditor.cpp
HEADERS += ../src/pdfeditor.h
isEmpty(QPDF_PREFIX): QPDF_PREFIX = $$(QPDF_PREFIX)
!isEmpty(QPDF_PREFIX) {
    INCLUDEPATH += $$QPDF_PREFIX/include
    LIBS += -L$$QPDF_PREFIX/lib -lqpdf
} else {
    CONFIG += link_pkgconfig
    PKGCONFIG += libqpdf
}
