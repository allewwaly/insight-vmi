# Root directory of project
ROOT_DIR = ../..

# Global configuration file
include($$ROOT_DIR/config.pri)

TEMPLATE = app
TARGET = asttypeevaluator
QT += core \
    testlib
QT -= gui webkit
CONFIG += qtestlib debug_and_release
HEADERS += asttypeevaluatortest.h
SOURCES += asttypeevaluatortest.cpp

INCLUDEPATH += \
    $$ROOT_DIR/libdebug/include \
    $$ROOT_DIR/libcparser/include \
    $$ROOT_DIR/libantlr3c/include \
    $$ROOT_DIR/libinsight/include

LIBS += -L$$ROOT_DIR/libinsight$$BUILD_DIR -l$$INSIGHT_LIB
