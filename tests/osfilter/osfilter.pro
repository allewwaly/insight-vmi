#-------------------------------------------------
#
# Project created by QtCreator 2012-10-26T16:43:28
#
#-------------------------------------------------

# Global configuration file
include(../../config.pri)

QT       += core testlib

QT       -= gui

TARGET = tst_osfiltertest
CONFIG   += console debug_and_release
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += SRCDIR=\\\"$$PWD/\\\"
ROOT_DIR = ../..

INCLUDEPATH += $$ROOT_DIR/insightd \
    $$ROOT_DIR/libdebug/include \
    $$ROOT_DIR/libcparser/include

LIBS += -L$$ROOT_DIR/libdebug$$BUILD_DIR -l$$DEBUG_LIB

SOURCES += tst_osfiltertest.cpp \
    $$ROOT_DIR/insightd/osfilter.cpp \
    $$ROOT_DIR/insightd/shellutil.cpp \
    $$ROOT_DIR/libcparser/src/genericexception.cpp
HEADERS += $$ROOT_DIR/insightd/osfilter.h \
    $$ROOT_DIR/insightd/shellutil.h \
    $$ROOT_DIR/libcparser/include/genericexception.h

