TEMPLATE = lib
TARGET = qopensearch
CONFIG += dll

include(../build.pri)
DESTDIR = $$BUILDDIR

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

QT += network script

HEADERS += \
    opensearchengine.h \
    opensearchenginedelegate.h \
    opensearchreader.h \
    opensearchwriter.h

SOURCES += \
    opensearchengine.cpp \
    opensearchenginedelegate.cpp \
    opensearchreader.cpp \
    opensearchwriter.cpp
