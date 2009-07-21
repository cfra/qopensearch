win32: CONFIG += console
mac:CONFIG -= app_bundle

CONFIG += qtestlib

HEADERS += qtry.h

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

RCC_DIR     = $$PWD/.rcc
UI_DIR      = $$PWD/.ui
MOC_DIR     = $$PWD/.moc
OBJECTS_DIR = $$PWD/.obj
