TEMPLATE = app
TARGET = tst_opensearchwriter

include(../tests.pri)
include(../../src/opensearch.pri)

SOURCES += \
    tst_opensearchwriter.cpp

RESOURCES += \
    opensearchwriter.qrc
