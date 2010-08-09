TEMPLATE = app
QMAKE_CXXFLAGS += -g -O0
TARGET = libmemtooltests
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
    -lmemtool
