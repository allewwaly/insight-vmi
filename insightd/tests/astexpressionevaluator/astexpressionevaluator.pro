# Global configuration file
include(../../../config.pri)

TEMPLATE = app
TARGET = astexpressionevaluatortester
HEADERS += astexpressionevaluatortester.h \
    ../../altreftype.h \
    ../../altreftyperulewriter.h \
    ../../array.h \
    ../../astexpressionevaluator.h \
    ../../astexpression.h \
    ../../basetype.h \
    ../../colorpalette.h \
    ../../compileunit.h \
    ../../consttype.h \
    ../../enum.h \
    ../../expressionresult.h \
    ../../funcparam.h \
    ../../funcpointer.h \
    ../../function.h \
    ../../instanceclass.h \
    ../../instancedata.h \
    ../../instance.h \
    ../../instanceprototype.h \
    ../../ioexception.h \
    ../../kernelsourceparser.h \
    ../../kernelsourcetypeevaluator.h \
    ../../kernelsymbolparser.h \
    ../../kernelsymbolreader.h \
    ../../kernelsymbolsclass.h \
    ../../kernelsymbols.h \
    ../../kernelsymbolstream.h \
    ../../kernelsymbolwriter.h \
    ../../longoperation.h \
    ../../memorydifftree.h \
    ../../memorydump.h \
    ../../memorydumpsclass.h \
    ../../memorymapbuildercs.h \
    ../../memorymapbuilder.h \
    ../../memorymapbuildersv.h \
    ../../memorymap.h \
    ../../memorymapheuristics.h \
    ../../memorymapnode.h \
    ../../memorymapnodesv.h \
    ../../memorymaprangetree.h \
    ../../memorymapverifier.h \
    ../../memoryrangetree.h \
    ../../memspecparser.h \
    ../../memspecs.h \
    ../../numeric.h \
    ../../parserexception.h \
    ../../pointer.h \
    ../../refbasetype.h \
    ../../referencingtype.h \
    ../../scriptengine.h \
    ../../shellutil.h \
    ../../slubobjects.h \
    ../../sourceref.h \
    ../../structured.h \
    ../../structuredmember.h \
    ../../symbol.h \
    ../../symfactory.h \
    ../../typedef.h \
    ../../typefilter.h \
    ../../typeinfo.h \
    ../../variable.h \
    ../../varsetter.h \
    ../../virtualmemoryexception.h \
    ../../virtualmemory.h \
    ../../volatiletype.h

SOURCES += astexpressionevaluatortester.cpp \

QT += core \
    script \
    network \
    xml \
    testlib
QT -= gui \
    webkit

# Disable warnings for this project
CONFIG -= warn_on

CONFIG += qtestlib debug_and_release

INCLUDEPATH += ../../src \
	../../../libdebug/include \
	../../../libcparser/include \
	../../../libcparser/antlr_generated \
    ../../../libantlr3c/include \
    ../../../libinsight/include

LIBS += -L../../../libcparser$$BUILD_DIR -l$$CPARSER_LIB \
        -L../../../libdebug$$BUILD_DIR -l$$DEBUG_LIB \
        -L../../../libantlr3c$$BUILD_DIR -l$$ANTLR_LIB \
        -L../../../libinsight$$BUILD_DIR -l$$INSIGHT_LIB
