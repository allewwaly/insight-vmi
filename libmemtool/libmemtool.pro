TEMPLATE = lib
TARGET = memtool
VERSION = 1.0.0
DEFINES += LIBMEMTOOL_LIBRARY
QT += core \
    network
QT -= gui
HEADERS += include/memtool/memtool.h \
    sockethelper.h \
    include/memtool/memtoolexception.h \
    include/memtool/constdefs.h
SOURCES += sockethelper.cpp \
    memtoolexception.cpp \
    memtool.cpp \
    constdefs.cpp
INCLUDEPATH += ./include
