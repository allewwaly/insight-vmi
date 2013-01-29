#-------------------------------------------------
#
# Project created by QtCreator 2012-10-26T16:43:28
#
#-------------------------------------------------

# Root directory of project
ROOT_DIR = ../..

# Global configuration file
include($$ROOT_DIR/config.pri)

QT       += core testlib

QT       -= gui

TARGET = tst_osfiltertest
CONFIG   += console debug_and_release
CONFIG   -= app_bundle

TEMPLATE = app

#DEFINES += SRCDIR=\\\"$$PWD/\\\"

INCLUDEPATH += $$ROOT_DIR/insightd \
    $$ROOT_DIR/libdebug/include \
    $$ROOT_DIR/libcparser/include \
    $$ROOT_DIR/libinsight/include

LIBS += -L$$ROOT_DIR/libdebug$$BUILD_DIR -l$$DEBUG_LIB \
        -L$$ROOT_DIR/libinsight$$BUILD_DIR -l$$INSIGHT_LIB

SOURCES += tst_osfiltertest.cpp \
    $$ROOT_DIR/libcparser/src/genericexception.cpp

HEADERS += $$ROOT_DIR/insightd/osfilter.h \
    $$ROOT_DIR/insightd/shellutil.h \
    $$ROOT_DIR/libcparser/include/genericexception.h

