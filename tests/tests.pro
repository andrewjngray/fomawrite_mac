QT += core gui quick testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_fomawrite

INCLUDEPATH += ../src
SOURCES += \
    tst_fomawrite.cpp \
    ../src/backend.cpp \
    ../src/markdownhighlighter.cpp \
    ../src/sourcevisualmapping.cpp \
    ../src/visualtexthighlighter.cpp \
    ../src/outputcss.cpp
HEADERS += \
    ../src/backend.h \
    ../src/markdownhighlighter.h \
    ../src/sourcevisualmapping.h \
    ../src/visualtexthighlighter.h \
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
HEADERS += ../src/publishingthemes.h ../src/publishinghtml.h ../src/publishingpdf.h ../src/publisher.h ../src/macbridge.h
SOURCES += ../src/publishingthemes.cpp ../src/publishinghtml.cpp ../src/publishingpdf.cpp ../src/publisher.cpp ../src/vendor/md4c/md4c.c
