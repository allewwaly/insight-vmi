# Root directory of project
ROOT_DIR = ../../..

# Global configuration file
include($$ROOT_DIR/config.pri)

TEMPLATE = app
TARGET = priorityqueuetester
HEADERS = priorityqueuetester.h ../../priorityqueue.h
SOURCES = priorityqueuetester.cpp
QT += core \
    testlib
QT -= webkit
CONFIG += qtestlib debug_and_release

INCLUDEPATH += $$ROOT_DIR/libinsight/include \
    $$ROOT_DIR/libdebug/include


LIBS += -L$$ROOT_DIR/libinsight$$BUILD_DIR -l$$INSIGHT_LIB
