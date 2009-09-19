TEMPLATE = app

QT += network

include(../../build.pri)
include(../../src/opensearch.pri)
DESTDIR = $$BUILDDIR/examples

SOURCES += main.cpp
HEADERS += helperobject.h
