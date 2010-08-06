TEMPLATE = lib
TARGET = memtool
VERSION = 1.0.0
DEFINES += LIBMEMTOOL_LIBRARY
QT += core \
    network
QT -= gui
HEADERS += devicemuxer.h \
    include/memtool/memtool.h \
    sockethelper.h \
    include/memtool/memtoolexception.h \
    include/memtool/constdefs.h \
    debug.h
SOURCES += devicemuxer.cpp \
    sockethelper.cpp \
    memtoolexception.cpp \
    memtool.cpp \
    constdefs.cpp \
    debug.cpp
INCLUDEPATH += ./include
