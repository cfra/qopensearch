INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

include(../build.pri)

LIBS += -L$$BUILDDIR \
    -Wl,-rpath,$$BUILDDIR \
    -lqopensearch
