QT += core gui qml quick pdf pdfquick
CONFIG += c++17 console
CONFIG -= app_bundle
TARGET = pdfspike
SOURCES += pdfspike.cpp
macx {
    OBJECTIVE_SOURCES += pdfspike_mac.mm
    LIBS += -framework AppKit
    CONFIG += link_pkgconfig
    PKGCONFIG += libqpdf
}
win32 {
    INCLUDEPATH += $$(QPDF_ROOT)/include
    LIBS += $$(QPDF_LIB) -lpsapi
    SOURCES += pdfspike_win_menus.cpp
}
