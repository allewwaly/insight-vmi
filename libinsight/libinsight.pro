# Global configuration file
include(../config.pri)

TEMPLATE = lib
TARGET = insight

# Set PREFIX, if not set
isEmpty(PREFIX):PREFIX = /usr/local

# Path for target
target.path += $$PREFIX/lib

# Extra target for headers
headers.files = include/insight/*.h
headers.path += $$PREFIX/include/insight

INSTALLS += target headers
VERSION = 1.0.0
DEFINES += LIBINSIGHT_LIBRARY
CONFIG += debug_and_release
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3
QT += core \
    network \
    script \
    xml
QT -= gui webkit

HEADERS += \
    eventloopthread.h \
    sockethelper.h \
    include/insight/altreftype.h \
    include/insight/array.h \
    include/insight/astexpressionevaluator.h \
    include/insight/astexpression.h \
    include/insight/basetype.h \
    include/insight/colorpalette.h \
    include/insight/compileunit.h \
    include/insight/console.h \
    include/insight/constdefs.h \
    include/insight/consttype.h \
    include/insight/devicemuxer.h \
    include/insight/enum.h \
    include/insight/expressionresult.h \
    include/insight/funcparam.h \
    include/insight/funcpointer.h \
    include/insight/function.h \
    include/insight/insight.h \
    include/insight/insightexception.h \
    include/insight/instanceclass.h \
    include/insight/instancedata.h \
    include/insight/instance.h \
    include/insight/instanceprototype.h \
    include/insight/kernelsourcetypeevaluator.h \
    include/insight/kernelsymbolparser.h \
    include/insight/kernelsymbolreader.h \
    include/insight/kernelsymbolsclass.h \
    include/insight/kernelsymbols.h \
    include/insight/kernelsymbolstream.h \
    include/insight/kernelsymbolwriter.h \
    include/insight/longoperation.h \
    include/insight/memorydifftree.h \
    include/insight/memorydump.h \
    include/insight/memorydumpsclass.h \
    include/insight/memorymapbuildercs.h \
    include/insight/memorymapbuilder.h \
    include/insight/memorymapbuildersv.h \
    include/insight/memorymap.h \
    include/insight/memorymapheuristics.h \
    include/insight/memorymapnode.h \
    include/insight/memorymapnodesv.h \
    include/insight/memorymaprangetree.h \
    include/insight/memorymapverifier.h \
    include/insight/memspecparser.h \
    include/insight/memspecs.h \
    include/insight/multithreading.h \
    include/insight/numeric.h \
    include/insight/osfilter.h \
    include/insight/pointer.h \
    include/insight/refbasetype.h \
    include/insight/referencingtype.h \
    include/insight/scriptengine.h \
    include/insight/shellutil.h \
    include/insight/slubobjects.h \
    include/insight/sourceref.h \
    include/insight/structured.h \
    include/insight/structuredmember.h \
    include/insight/symbol.h \
    include/insight/symfactory.h \
    include/insight/typedef.h \
    include/insight/typefilter.h \
    include/insight/typeinfo.h \
    include/insight/typeruleenginecontextprovider.h \
    include/insight/typeruleengine.h \
    include/insight/typerule.h \
    include/insight/typeruleparser.h \
    include/insight/typerulereader.h \
    include/insight/variable.h \
    include/insight/virtualmemory.h \
    include/insight/volatiletype.h \
    include/insight/xmlschema.h \
    include/insight/compiler.h

SOURCES += \
    altreftype.cpp \
    array.cpp \
    astexpression.cpp \
    astexpressionevaluator.cpp \
    basetype.cpp \
    colorpalette.cpp \
    compileunit.cpp \
    console.cpp \
    constdefs.cpp \
    consttype.cpp \
    devicemuxer.cpp \
    enum.cpp \
    eventloopthread.cpp \
    expressionresult.cpp \
    funcparam.cpp \
    funcpointer.cpp \
    function.cpp \
    insightexception.cpp \
    insight.cpp \
    instanceclass.cpp \
    instance.cpp \
    instancedata.cpp \
    instanceprototype.cpp \
    kernelsourcetypeevaluator.cpp \
    kernelsymbolparser.cpp \
    kernelsymbolreader.cpp \
    kernelsymbolsclass.cpp \
    kernelsymbols.cpp \
    kernelsymbolstream.cpp \
    kernelsymbolwriter.cpp \
    longoperation.cpp \
    memorydifftree.cpp \
    memorydump.cpp \
    memorydumpsclass.cpp \
    memorymapbuilder.cpp \
    memorymapbuildercs.cpp \
    memorymapbuildersv.cpp \
    memorymap.cpp \
    memorymapheuristics.cpp \
    memorymapnode.cpp \
    memorymapnodesv.cpp \
    memorymaprangetree.cpp \
    memorymapverifier.cpp \
    memspecparser.cpp \
    memspecs.cpp \
    multithreading.cpp \
    numeric.cpp \
    osfilter.cpp \
    pointer.cpp \
    refbasetype.cpp \
    referencingtype.cpp \
    scriptengine.cpp \
    shellutil.cpp \
    slubobjects.cpp \
    sockethelper.cpp \
    sourceref.cpp \
    structured.cpp \
    structuredmember.cpp \
    symbol.cpp \
    symfactory.cpp \
    typedef.cpp \
    typefilter.cpp \
    typeinfo.cpp \
    typerule.cpp \
    typeruleenginecontextprovider.cpp \
    typeruleengine.cpp \
    typeruleparser.cpp \
    typerulereader.cpp \
    variable.cpp \
    virtualmemory.cpp \
    volatiletype.cpp \
    xmlschema.cpp

# Required libraries for building
LIBS += \
    -L../libdebug$$BUILD_DIR \
    -l$$DEBUG_LIB \
    -L../libcparser$$BUILD_DIR \
    -l$$CPARSER_LIB \
    -L../libantlr3c$$BUILD_DIR \
    -l$$ANTLR_LIB

# Inter-project include paths
INCLUDEPATH += ./include \
    ../libdebug/include \
    ../libantlr3c/include \
    ../libcparser/include

SUBDIRS = tests
