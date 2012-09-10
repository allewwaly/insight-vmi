# Global configuration file
include(../../../config.pri)

TEMPLATE = app
TARGET = memoryrangetreetester
HEADERS = memoryrangetreetester.h ../../memoryrangetree.h
SOURCES = memoryrangetreetester.cpp
QT += core \
    testlib
QT -= webkit
CONFIG += qtestlib debug_and_release

INCLUDEPATH +=  ../../../libdebug/include
LIBS += -L../../../libdebug$$BUILD_DIR -l$$DEBUG_LIB
