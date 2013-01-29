# Root directory of project
ROOT_DIR = ../..

# Global configuration file
include($$ROOT_DIR/config.pri)

TEMPLATE = app

TARGET = test_devicemuxer

QT += core \
    network \
    testlib
CONFIG += qtestlib
HEADERS = devicemuxertest.h
SOURCES += main.cpp \
    devicemuxertest.cpp

INCLUDEPATH += \
    $$ROOT_DIR/libdebug/include \
    $$ROOT_DIR/libinsight/include

LIBS += -L$$ROOT_DIR/libinsight$$BUILD_DIR -l$$INSIGHT_LIB
