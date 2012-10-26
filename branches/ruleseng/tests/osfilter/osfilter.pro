#-------------------------------------------------
#
# Project created by QtCreator 2012-10-26T16:43:28
#
#-------------------------------------------------

QT       += core testlib

QT       -= gui

TARGET = tst_osfiltertest
CONFIG   += console debug
CONFIG   -= app_bundle

TEMPLATE = app

DEFINES += SRCDIR=\\\"$$PWD/\\\"
ROOT_DIR = ../..

INCLUDEPATH += $$ROOT_DIR/insightd \
    $$ROOT_DIR/libcparser/include
SOURCES += tst_osfiltertest.cpp \
    $$ROOT_DIR/insightd/osfilter.cpp \
    $$ROOT_DIR/libcparser/src/genericexception.cpp
HEADERS += $$ROOT_DIR/insightd/osfilter.h \
    $$ROOT_DIR/libcparser/include/genericexception.h

