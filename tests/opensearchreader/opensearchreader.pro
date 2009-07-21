TEMPLATE = app
TARGET = tst_opensearchreader

include(../tests.pri)
include(../../src/opensearch.pri)

SOURCES += \
    tst_opensearchreader.cpp

RESOURCES += \
    opensearchreader.qrc
