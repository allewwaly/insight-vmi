#-------------------------------------------------
#
# Project created by QtCreator 2012-10-26T19:10:55
#
#-------------------------------------------------

# Root directory of project
ROOT_DIR = ../..

# Global configuration file
include($$ROOT_DIR/config.pri)

QT       += core testlib script xml network

QT       -= gui webkit

TARGET = test_typefilter
CONFIG   += console debug_and_release
CONFIG   -= app_bundle

TEMPLATE = app


#DEFINES += SRCDIR=\\\"$$PWD/\\\"

INCLUDEPATH += \
    $$ROOT_DIR/libdebug/include \
    $$ROOT_DIR/libcparser/include \
    $$ROOT_DIR/libcparser/antlr_generated \
    $$ROOT_DIR/libantlr3c/include \
    $$ROOT_DIR/libinsight/include

LIBS += -L$$ROOT_DIR/libcparser$$BUILD_DIR -l$$CPARSER_LIB \
        -L$$ROOT_DIR/libdebug$$BUILD_DIR -l$$DEBUG_LIB \
        -L$$ROOT_DIR/libantlr3c$$BUILD_DIR -l$$ANTLR_LIB \
        -L$$ROOT_DIR/libinsight$$BUILD_DIR -l$$INSIGHT_LIB

SOURCES += tst_typefiltertest.cpp \
    $$ROOT_DIR/insightd/altreftyperulewriter.cpp \
    $$ROOT_DIR/insightd/kernelsourceparser.cpp \
    $$ROOT_DIR/libcparser/src/genericexception.cpp

