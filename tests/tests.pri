win32: CONFIG += console
mac:CONFIG -= app_bundle

CONFIG += qtestlib

HEADERS += qtry.h

include(../build.pri)
DESTDIR = $$BUILDDIR

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
