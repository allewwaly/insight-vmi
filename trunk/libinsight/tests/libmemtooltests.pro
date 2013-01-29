TEMPLATE = app
QMAKE_CXXFLAGS += -g -O0
nclude/insight/astexpression.h/d
nclude/insight/funcparam.h/d
nclude/insight/array.h/d
nclude/insight/pointer.h/d
nclude/insight/volatiletype.h/d
nclude/insight/memorymapbuildersv.h/d
nclude/insight/typefilter.h/d
nclude/insight/osfilter.h/d
nclude/insight/scriptengine.h/d
nclude/insight/structuredmember.h/d
nclude/insight/colorpalette.h/d
nclude/insight/memorydump.h/d
nclude/insight/typeruleengine.h/d
nclude/insight/symfactory.h/d
nclude/insight/kernelsymbolwriter.h/d
nclude/insight/memorymap.h/d
nclude/insight/typerule.h/d
nclude/insight/basetype.h/d
nclude/insight/virtualmemory.h/d
nclude/insight/kernelsymbolsclass.h/d
nclude/insight/longoperation.h/d
nclude/insight/multithreading.h/d
nclude/insight/xmlschema.h/d
nclude/insight/memorydifftree.h/d
nclude/insight/function.h/d
nclude/insight/consttype.h/d
nclude/insight/instanceclass.h/d
nclude/insight/instanceprototype.h/d
nclude/insight/astexpressionevaluator.h/d
nclude/insight/sourceref.h/d
nclude/insight/typeinfo.h/d
nclude/insight/expressionresult.h/d
nclude/insight/console.h/d
nclude/insight/memorymapbuildercs.h/d
nclude/insight/memorymaprangetree.h/d
nclude/insight/memspecparser.h/d
nclude/insight/memorymapheuristics.h/d
nclude/insight/memorymapnode.h/d
nclude/insight/memorymapbuilder.h/d
nclude/insight/slubobjects.h/d
nclude/insight/kernelsymbolparser.h/d
nclude/insight/memorymapnodesv.h/d
nclude/insight/variable.h/d
nclude/insight/kernelsourcetypeevaluator.h/d
nclude/insight/typeruleparser.h/d
nclude/insight/compileunit.h/d
nclude/insight/referencingtype.h/d
nclude/insight/kernelsymbols.h/d
nclude/insight/kernelsymbolreader.h/d
nclude/insight/typerulereader.h/d
nclude/insight/instancedata.h/d
nclude/insight/typedef.h/d
nclude/insight/numeric.h/d
nclude/insight/memorymapverifier.h/d
nclude/insight/enum.h/d
nclude/insight/symbol.h/d
nclude/insight/shellutil.h/d
nclude/insight/memorydumpsclass.h/d
nclude/insight/refbasetype.h/d
nclude/insight/memspecs.h/d
nclude/insight/typeruleenginecontextprovider.h/d
nclude/insight/altreftype.h/d
nclude/insight/kernelsymbolstream.h/d
nclude/insight/funcpointer.h/d
nclude/insight/instance.h/d
nclude/insight/structured.h/d
TARGET = libinsighttests
QT += core \
    network \
    testlib
CONFIG += qtestlib
HEADERS = devicemuxertest.h
SOURCES += main.cpp \
    devicemuxertest.cpp
INCLUDEPATH += ../include
LIBS += -L \
    ../ \
    -linsight
