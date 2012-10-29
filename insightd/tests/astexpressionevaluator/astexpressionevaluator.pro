# Global configuration file
include(../../../config.pri)

TEMPLATE = app
TARGET = astexpressionevaluatortester
HEADERS += astexpressionevaluatortester.h \
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
    ../../memorydump.h \
    ../../memorydumpsclass.h \
    ../../memorymapbuildercs.h \
    ../../memorymapbuildersv.h \
    ../../memorymapheuristics.h \
    ../../memorymapnodesv.h \
    ../../memorymapverifier.h \
    ../../memspecparser.h \
    ../../memspecs.h \
    ../../numeric.h \
    ../../parserexception.h \
    ../../pointer.h \
    ../../programoptions.h \
    ../../refbasetype.h \
    ../../referencingtype.h \
    ../../scriptengine.h \
    ../../shell.h \
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
    ../../array.cpp \
    ../../astexpression.cpp \
    ../../astexpressionevaluator.cpp \
    ../../basetype.cpp \
    ../../colorpalette.cpp \
    ../../compileunit.cpp \
    ../../consttype.cpp \
    ../../enum.cpp \
    ../../expressionresult.cpp \
    ../../funcparam.cpp \
    ../../funcpointer.cpp \
    ../../function.cpp \
    ../../instanceclass.cpp \
    ../../instance.cpp \
    ../../instancedata.cpp \
    ../../instanceprototype.cpp \
    ../../kernelsourceparser.cpp \
    ../../kernelsourcetypeevaluator.cpp \
    ../../kernelsymbolparser.cpp \
    ../../kernelsymbolreader.cpp \
    ../../kernelsymbolsclass.cpp \
    ../../kernelsymbols.cpp \
    ../../kernelsymbolstream.cpp \
    ../../kernelsymbolwriter.cpp \
    ../../longoperation.cpp \
    ../../memorydump.cpp \
    ../../memorydumpsclass.cpp \
    ../../memorymapbuildercs.cpp \
    ../../memorymapbuildersv.cpp \
    ../../memorymapheuristics.cpp \
    ../../memorymapnodesv.cpp \
    ../../memorymapverifier.cpp \
    ../../memspecparser.cpp \
    ../../memspecs.cpp \
    ../../numeric.cpp \
    ../../osfilter.cpp \
    ../../pointer.cpp \
    ../../programoptions.cpp \
    ../../refbasetype.cpp \
    ../../referencingtype.cpp \
    ../../scriptengine.cpp \
    ../../shell.cpp \
    ../../shell_readline.cpp \
    ../../slubobjects.cpp \
    ../../sourceref.cpp \
    ../../structured.cpp \
    ../../structuredmember.cpp \
    ../../symbol.cpp \
    ../../symfactory.cpp \
    ../../typedef.cpp \
    ../../typefilter.cpp \
    ../../typeinfo.cpp \
    ../../typerule.cpp \
    ../../typeruleengine.cpp \
    ../../typeruleparser.cpp \
    ../../typerulereader.cpp \
    ../../variable.cpp \
    ../../virtualmemory.cpp \
    ../../volatiletype.cpp \
    ../../xmlschema.cpp

QT += core \
    script \
    network \
    xml \
    testlib
QT -= webkit
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

QMAKE_CXXFLAGS_DEBUG += -w
QMAKE_CXXFLAGS_RELEASE += -w

# Things to do when the memory map builder and widget is to be built. Enabling
# this feature requires InSight to run on an X server.
CONFIG += memory_map
CONFIG(memory_map) {
    unix:warning(Enabled compilation of the memory_map features. The resulting binary will must be run on an X windows system!)

    DEFINES += CONFIG_MEMORY_MAP
    QT += gui

    FORMS += ../../memorymapwindow.ui

    SOURCES += ../../memorydifftree.cpp \
        ../../memorymaprangetree.cpp \
        ../../memorymapbuilder.cpp \
        ../../memorymapwindow.cpp \
        ../../memorymapwidget.cpp \
        ../../memorymapnode.cpp \
        ../../memorymap.cpp

    HEADERS += ../../memorydifftree.h \
        ../../memorymaprangetree.h \
        ../../memoryrangetree.h \
        ../../memorymapbuilder.h \
        ../../memorymapwindow.h \
        ../../memorymapwidget.h \
        ../../memorymapnode.h \
        ../../memorymap.h
}
!CONFIG(memory_map) {
    QT -= gui
}
# Enable or disable libreadline support
CONFIG(with_readline) {
    DEFINES += CONFIG_READLINE
    LIBS += -lreadline
}
