#-------------------------------------------------
#
# Project created by QtCreator 2012-10-26T19:10:55
#
#-------------------------------------------------

# Global configuration file
include(../../config.pri)

QT       += core testlib script xml network

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
    $$ROOT_DIR/insightd/altreftype.cpp \
    $$ROOT_DIR/insightd/altreftyperulewriter.cpp \
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
    $$ROOT_DIR/insightd/kernelsourceparser.cpp \
    $$ROOT_DIR/insightd/kernelsourcetypeevaluator.cpp \
    $$ROOT_DIR/insightd/kernelsymbolparser.cpp \
    $$ROOT_DIR/insightd/kernelsymbolreader.cpp \
    $$ROOT_DIR/insightd/kernelsymbolsclass.cpp \
    $$ROOT_DIR/insightd/kernelsymbols.cpp \
    $$ROOT_DIR/insightd/kernelsymbolstream.cpp \
    $$ROOT_DIR/insightd/kernelsymbolwriter.cpp \
    $$ROOT_DIR/insightd/longoperation.cpp \
    $$ROOT_DIR/insightd/memorydifftree.cpp \
    $$ROOT_DIR/insightd/memorydump.cpp \
    $$ROOT_DIR/insightd/memorydumpsclass.cpp \
    $$ROOT_DIR/insightd/memorymapbuilder.cpp \
    $$ROOT_DIR/insightd/memorymapbuildercs.cpp \
    $$ROOT_DIR/insightd/memorymapbuildersv.cpp \
    $$ROOT_DIR/insightd/memorymap.cpp \
    $$ROOT_DIR/insightd/memorymapheuristics.cpp \
    $$ROOT_DIR/insightd/memorymapnode.cpp \
    $$ROOT_DIR/insightd/memorymapnodesv.cpp \
    $$ROOT_DIR/insightd/memorymaprangetree.cpp \
    $$ROOT_DIR/insightd/memorymapverifier.cpp \
    $$ROOT_DIR/insightd/memspecparser.cpp \
    $$ROOT_DIR/insightd/memspecs.cpp \
    $$ROOT_DIR/insightd/numeric.cpp \
    $$ROOT_DIR/insightd/osfilter.cpp \
    $$ROOT_DIR/insightd/pointer.cpp \
    $$ROOT_DIR/insightd/programoptions.cpp \
    $$ROOT_DIR/insightd/refbasetype.cpp \
    $$ROOT_DIR/insightd/referencingtype.cpp \
    $$ROOT_DIR/insightd/scriptengine.cpp \
    $$ROOT_DIR/insightd/shell.cpp \
    $$ROOT_DIR/insightd/shellutil.cpp \
    $$ROOT_DIR/insightd/shell_readline.cpp \
    $$ROOT_DIR/insightd/slubobjects.cpp \
    $$ROOT_DIR/insightd/sourceref.cpp \
    $$ROOT_DIR/insightd/structured.cpp \
    $$ROOT_DIR/insightd/structuredmember.cpp \
    $$ROOT_DIR/insightd/symbol.cpp \
    $$ROOT_DIR/insightd/symfactory.cpp \
    $$ROOT_DIR/insightd/typedef.cpp \
    $$ROOT_DIR/insightd/typefilter.cpp \
    $$ROOT_DIR/insightd/typeinfo.cpp \
    $$ROOT_DIR/insightd/typerule.cpp \
    $$ROOT_DIR/insightd/typeruleengine.cpp \
    $$ROOT_DIR/insightd/typeruleparser.cpp \
    $$ROOT_DIR/insightd/typerulereader.cpp \
    $$ROOT_DIR/insightd/variable.cpp \
    $$ROOT_DIR/insightd/virtualmemory.cpp \
    $$ROOT_DIR/insightd/volatiletype.cpp \
    $$ROOT_DIR/insightd/xmlschema.cpp \
    $$ROOT_DIR/libcparser/src/genericexception.cpp

HEADERS += \
    $$ROOT_DIR/insightd/altreftype.h \
    $$ROOT_DIR/insightd/altreftyperulewriter.h \
    $$ROOT_DIR/insightd/array.h \
    $$ROOT_DIR/insightd/astexpression.h \
    $$ROOT_DIR/insightd/astexpressionevaluator.h \
    $$ROOT_DIR/insightd/basetype.h \
    $$ROOT_DIR/insightd/colorpalette.h \
    $$ROOT_DIR/insightd/compileunit.h \
    $$ROOT_DIR/insightd/consttype.h \
    $$ROOT_DIR/insightd/enum.h \
    $$ROOT_DIR/insightd/expressionresult.h \
    $$ROOT_DIR/insightd/funcparam.h \
    $$ROOT_DIR/insightd/funcpointer.h \
    $$ROOT_DIR/insightd/function.h \
    $$ROOT_DIR/insightd/instanceclass.h \
    $$ROOT_DIR/insightd/instance.h \
    $$ROOT_DIR/insightd/instancedata.h \
    $$ROOT_DIR/insightd/instanceprototype.h \
    $$ROOT_DIR/insightd/kernelsourceparser.h \
    $$ROOT_DIR/insightd/kernelsourcetypeevaluator.h \
    $$ROOT_DIR/insightd/kernelsymbolparser.h \
    $$ROOT_DIR/insightd/kernelsymbolreader.h \
    $$ROOT_DIR/insightd/kernelsymbolsclass.h \
    $$ROOT_DIR/insightd/kernelsymbols.h \
    $$ROOT_DIR/insightd/kernelsymbolstream.h \
    $$ROOT_DIR/insightd/kernelsymbolwriter.h \
    $$ROOT_DIR/insightd/longoperation.h \
    $$ROOT_DIR/insightd/memorydifftree.h \
    $$ROOT_DIR/insightd/memorydump.h \
    $$ROOT_DIR/insightd/memorydumpsclass.h \
    $$ROOT_DIR/insightd/memorymapbuildercs.h \
    $$ROOT_DIR/insightd/memorymapbuilder.h \
    $$ROOT_DIR/insightd/memorymapbuildersv.h \
    $$ROOT_DIR/insightd/memorymap.h \
    $$ROOT_DIR/insightd/memorymapheuristics.h \
    $$ROOT_DIR/insightd/memorymapnode.h \
    $$ROOT_DIR/insightd/memorymapnodesv.h \
    $$ROOT_DIR/insightd/memorymaprangetree.h \
    $$ROOT_DIR/insightd/memorymapverifier.h \
    $$ROOT_DIR/insightd/memoryrangetree.h \
    $$ROOT_DIR/insightd/memspecparser.h \
    $$ROOT_DIR/insightd/memspecs.h \
    $$ROOT_DIR/insightd/numeric.h \
    $$ROOT_DIR/insightd/osfilter.h \
    $$ROOT_DIR/insightd/pointer.h \
    $$ROOT_DIR/insightd/programoptions.h \
    $$ROOT_DIR/insightd/refbasetype.h \
    $$ROOT_DIR/insightd/referencingtype.h \
    $$ROOT_DIR/insightd/scriptengine.h \
    $$ROOT_DIR/insightd/shell.h \
    $$ROOT_DIR/insightd/shellutil.h \
    $$ROOT_DIR/insightd/sourceref.h \
    $$ROOT_DIR/insightd/structured.h \
    $$ROOT_DIR/insightd/structuredmember.h \
    $$ROOT_DIR/insightd/symbol.h \
    $$ROOT_DIR/insightd/symfactory.h \
    $$ROOT_DIR/insightd/typedef.h \
    $$ROOT_DIR/insightd/typefilter.h \
    $$ROOT_DIR/insightd/typeinfo.h \
    $$ROOT_DIR/insightd/typerule.h \
    $$ROOT_DIR/insightd/typeruleengine.h \
    $$ROOT_DIR/insightd/typeruleparser.h \
    $$ROOT_DIR/insightd/typerulereader.h \
    $$ROOT_DIR/insightd/variable.h \
    $$ROOT_DIR/insightd/virtualmemory.h \
    $$ROOT_DIR/insightd/volatiletype.h \
    $$ROOT_DIR/insightd/xmlschema.h \
    $$ROOT_DIR/libcparser/include/genericexception.h
