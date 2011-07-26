TEMPLATE = lib
TARGET = insight
VERSION = 1.0.0
DEFINES += LIBINSIGHT_LIBRARY
CONFIG += debug
QT += core \
    network
QT -= gui
HEADERS += eventloopthread.h \
    include/insight/devicemuxer.h \
    include/insight/insight.h \
    sockethelper.h \
    include/insight/insightexception.h \
    include/insight/constdefs.h \
    debug.h
SOURCES += eventloopthread.cpp \
    devicemuxer.cpp \
    sockethelper.cpp \
    insightexception.cpp \
    insight.cpp \
    constdefs.cpp \
    debug.cpp
INCLUDEPATH += ./include
SUBDIRS = tests
