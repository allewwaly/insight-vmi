# Global configuration file
include(../../../config.pri)

TEMPLATE = app
TARGET = asttypeevaluator
QT += core \
    testlib
QT -= gui webkit
CONFIG += qtestlib debug_and_release
HEADERS += asttypeevaluatortest.h
SOURCES += asttypeevaluatortest.cpp
INCLUDEPATH += ../../src \
	../../include \
	../../../libdebug/include \
	../../antlr_generated \
	../../../libantlr3c/include 

LIBS += -L../..$$BUILD_DIR -l$$CPARSER_LIB \
        -L../../../libdebug$$BUILD_DIR -l$$DEBUG_LIB \
        -L../../../libantlr3c$$BUILD_DIR -l$$ANTLR_LIB
