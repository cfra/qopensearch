INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

LIBS += -L$$BUILDDIR \
    -Wl,-rpath,$$BUILDDIR \
    -lqopensearch
