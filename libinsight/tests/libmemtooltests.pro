TEMPLATE = app
QMAKE_CXXFLAGS += -g -O0
TARGET = libinsighttests
QT += core \
    network \
    testlib
CONFIG += qtestlib
HEADERS = devicemuxertest.h
SOURCES += main.cpp \
    devicemuxertest.cpp
INCLUDEPATH += ../include
LIBS += -L \
    ../ \
    -linsight
