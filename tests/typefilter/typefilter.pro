#-------------------------------------------------
#
# Project created by QtCreator 2012-10-26T19:10:55
#
#-------------------------------------------------

# Global configuration file
include(../../config.pri)

QT       += core testlib script network

QT       -= gui webkit

TARGET = tst_typefiltertest
CONFIG   += console debug_and_release
CONFIG   -= app_bundle

TEMPLATE = app


DEFINES += SRCDIR=\\\"$$PWD/\\\"
ROOT_DIR = ../..

INCLUDEPATH += $$ROOT_DIR/insightd \
    $$ROOT_DIR/libdebug/include \
    $$ROOT_DIR/libcparser/include \
    $$ROOT_DIR/libcparser/antlr_generated \
        $$ROOT_DIR/libantlr3c/include \
        $$ROOT_DIR/libinsight/include

LIBS += -L$$ROOT_DIR/libcparser$$BUILD_DIR -l$$CPARSER_LIB \
        -L$$ROOT_DIR/libdebug$$BUILD_DIR -l$$DEBUG_LIB \
        -L$$ROOT_DIR/libantlr3c$$BUILD_DIR -l$$ANTLR_LIB \
        -L$$ROOT_DIR/libinsight$$BUILD_DIR -l$$INSIGHT_LIB

# Enable or disable libreadline support
CONFIG(with_readline) {
    DEFINES += CONFIG_READLINE
    LIBS += -lreadline
}

SOURCES += tst_typefiltertest.cpp \
    $$ROOT_DIR/insightd/array.cpp \
    $$ROOT_DIR/insightd/astexpression.cpp \
    $$ROOT_DIR/insightd/astexpressionevaluator.cpp \
    $$ROOT_DIR/insightd/basetype.cpp \
    $$ROOT_DIR/insightd/colorpalette.cpp \
    $$ROOT_DIR/insightd/compileunit.cpp \
    $$ROOT_DIR/insightd/consttype.cpp \
    $$ROOT_DIR/insightd/enum.cpp \
    $$ROOT_DIR/insightd/expressionresult.cpp \
    $$ROOT_DIR/insightd/funcparam.cpp \
    $$ROOT_DIR/insightd/funcpointer.cpp \
    $$ROOT_DIR/insightd/function.cpp \
    $$ROOT_DIR/insightd/instanceclass.cpp \
    $$ROOT_DIR/insightd/instance.cpp \
    $$ROOT_DIR/insightd/instancedata.cpp \
    $$ROOT_DIR/insightd/instanceprototype.cpp \
    $$ROOT_DIR/insightd/kernelsymbolparser.cpp \
    $$ROOT_DIR/insightd/kernelsourceparser.cpp \
    $$ROOT_DIR/insightd/kernelsourcetypeevaluator.cpp \
    $$ROOT_DIR/insightd/kernelsymbolreader.cpp \
    $$ROOT_DIR/insightd/kernelsymbolsclass.cpp \
    $$ROOT_DIR/insightd/kernelsymbols.cpp \
    $$ROOT_DIR/insightd/kernelsymbolstream.cpp \
    $$ROOT_DIR/insightd/kernelsymbolwriter.cpp \
    $$ROOT_DIR/insightd/longoperation.cpp \
    $$ROOT_DIR/insightd/memorydump.cpp \
    $$ROOT_DIR/insightd/memorydumpsclass.cpp \
    $$ROOT_DIR/insightd/memspecparser.cpp \
    $$ROOT_DIR/insightd/memspecs.cpp \
    $$ROOT_DIR/insightd/numeric.cpp \
    $$ROOT_DIR/insightd/pointer.cpp \
    $$ROOT_DIR/insightd/programoptions.cpp \
    $$ROOT_DIR/insightd/refbasetype.cpp \
    $$ROOT_DIR/insightd/referencingtype.cpp \
    $$ROOT_DIR/insightd/scriptengine.cpp \
    $$ROOT_DIR/insightd/shell.cpp \
    $$ROOT_DIR/insightd/shell_readline.cpp \
    $$ROOT_DIR/insightd/sourceref.cpp \
    $$ROOT_DIR/insightd/structured.cpp \
    $$ROOT_DIR/insightd/structuredmember.cpp \
    $$ROOT_DIR/insightd/symbol.cpp \
    $$ROOT_DIR/insightd/symfactory.cpp \
    $$ROOT_DIR/insightd/typedef.cpp \
    $$ROOT_DIR/insightd/typefilter.cpp \
    $$ROOT_DIR/insightd/typeinfo.cpp \
    $$ROOT_DIR/insightd/variable.cpp \
    $$ROOT_DIR/insightd/virtualmemory.cpp \
    $$ROOT_DIR/insightd/volatiletype.cpp \
    $$ROOT_DIR/libcparser/src/genericexception.cpp

HEADERS += $$ROOT_DIR/insightd/typefilter.h \
    $$ROOT_DIR/insightd/instanceprototype.h \
    $$ROOT_DIR/insightd/kernelsymbolsclass.h \
    $$ROOT_DIR/insightd/memorydumpsclass.h \
    $$ROOT_DIR/insightd/shell.h \
    $$ROOT_DIR/libcparser/include/genericexception.h


#INCLUDEPATH += ../../src \
#    ../../include \
#    ../../../libdebug/include \
#    ../../antlr_generated \
#    ../../../libantlr3c/include

#LIBS += -L../..$$BUILD_DIR -l$$CPARSER_LIB \
#        -L../../../libdebug$$BUILD_DIR -l$$DEBUG_LIB \
#        -L../../../libantlr3c$$BUILD_DIR -l$$ANTLR_LIB
