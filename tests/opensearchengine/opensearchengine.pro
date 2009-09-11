TEMPLATE = app
TARGET = tst_opensearchengine

QT += network

include(../tests.pri)
include(../../src/opensearch.pri)

SOURCES += \
    tst_opensearchengine.cpp

RESOURCES += \
    opensearchengine.qrc
