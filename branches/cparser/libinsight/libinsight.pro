TEMPLATE = lib
TARGET = insight

# Set PREFIX, if not set
isEmpty(PREFIX):PREFIX = /usr/local

# Path for target
target.path += $$PREFIX/lib

# Extra target for headers
headers.files = include/insight/*.h
headers.path += $$PREFIX/include/insight

INSTALLS += target headers
VERSION = 1.0.0
DEFINES += LIBINSIGHT_LIBRARY
CONFIG += debug_and_release
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3
QT += core \
    network
QT -= gui
HEADERS += eventloopthread.h \
    include/insight/devicemuxer.h \
    include/insight/insight.h \
    sockethelper.h \
    include/insight/insightexception.h \
    include/insight/constdefs.h
SOURCES += eventloopthread.cpp \
    devicemuxer.cpp \
    sockethelper.cpp \
    insightexception.cpp \
    insight.cpp \
    constdefs.cpp
INCLUDEPATH += ./include ../libdebug/include
SUBDIRS = tests
