# Root directory of project
ROOT_DIR = ../..

# Global configuration file
include($$ROOT_DIR/config.pri)

TEMPLATE = app
TARGET = astexpressionevaluatortester
HEADERS += astexpressionevaluatortester.h

SOURCES += astexpressionevaluatortester.cpp

QT += core \
    script \
    network \
    xml \
    testlib
QT -= gui \
    webkit

# Disable warnings for this project
CONFIG -= warn_on

CONFIG += qtestlib debug_and_release

INCLUDEPATH += \
    $$ROOT_DIR/libdebug/include \
    $$ROOT_DIR/libcparser/include \
    $$ROOT_DIR/libantlr3c/include \
    $$ROOT_DIR/libinsight/include

LIBS += -L$$ROOT_DIR/libinsight$$BUILD_DIR -l$$INSIGHT_LIB
