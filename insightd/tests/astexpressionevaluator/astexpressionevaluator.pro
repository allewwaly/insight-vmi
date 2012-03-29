TEMPLATE = app
TARGET = astexpressionevaluatortester
HEADERS += astexpressionevaluatortester.h \
    ../../array.h \
    ../../volatiletype.h \
    ../../virtualmemoryexception.h \
    ../../virtualmemory.h \
    ../../varsetter.h \
    ../../variable.h \
    ../../typedef.h \
    ../../symfactory.h \
    ../../symbol.h \
    ../../structuredmember.h \
    ../../structured.h \
    ../../sourceref.h \
    ../../shell.h \
    ../../referencingtype.h \
    ../../refbasetype.h \
    ../../pointer.h \
    ../../parserexception.h \
    ../../numeric.h \
    ../../memspecs.h \
    ../../kernelsymbols.h \
    ../../kernelsourcetypeevaluator.h \
    ../../kernelsourceparser.h \
    ../../ioexception.h \
    ../../function.h \
    ../../funcpointer.h \
    ../../funcparam.h \
    ../../enum.h \
    ../../consttype.h \
    ../../compileunit.h \
    ../../basetype.h \
    ../../astexpressionevaluator.h \
    ../../astexpression.h \
    ../../programoptions.h \
    ../../scriptengine.h \
    ../../instance.h \
    ../../instanceclass.h \
    ../../instancedata.h \
    ../../instanceprototype.h \
    ../../typeinfo.h \
    ../../longoperation.h \
    ../../kernelsymbolsclass.h \
    ../../kernelsymbolparser.h \
    ../../kernelsymbolstream.h \
    ../../kernelsymbolreader.h \
    ../../kernelsymbolwriter.h \
    ../../memspecparser.h \
    ../../memorydump.h \
    ../../memorydumpsclass.h \
    ../../colorpalette.h \
    ../../expressionresult.h
SOURCES += astexpressionevaluatortester.cpp \
    ../../array.cpp \
    ../../volatiletype.cpp \
    ../../virtualmemory.cpp \
    ../../variable.cpp \
    ../../typedef.cpp \
    ../../symfactory.cpp \
    ../../symbol.cpp \
    ../../structuredmember.cpp \
    ../../structured.cpp \
    ../../sourceref.cpp \
    ../../shell_readline.cpp \
    ../../shell.cpp \
    ../../referencingtype.cpp \
    ../../refbasetype.cpp \
    ../../pointer.cpp \
    ../../numeric.cpp \
    ../../memspecs.cpp \
    ../../kernelsymbols.cpp \
    ../../kernelsourcetypeevaluator.cpp \
    ../../kernelsourceparser.cpp \
    ../../function.cpp \
    ../../funcpointer.cpp \
    ../../funcparam.cpp \
    ../../enum.cpp \
    ../../consttype.cpp \
    ../../compileunit.cpp \
    ../../basetype.cpp \
    ../../astexpressionevaluator.cpp \
    ../../astexpression.cpp \
    ../../programoptions.cpp \
    ../../scriptengine.cpp \
    ../../instance.cpp \
    ../../instanceclass.cpp \
    ../../instancedata.cpp \
    ../../instanceprototype.cpp \
    ../../typeinfo.cpp \
    ../../longoperation.cpp \
    ../../kernelsymbolsclass.cpp \
    ../../kernelsymbolparser.cpp \
    ../../kernelsymbolstream.cpp \
    ../../kernelsymbolreader.cpp \
    ../../kernelsymbolwriter.cpp \
    ../../memspecparser.cpp \
    ../../memorydump.cpp \
    ../../memorydumpsclass.cpp \
    ../../colorpalette.cpp \
    ../../expressionresult.cpp
QT += core \
    script \
    network \
    testlib
QT -= gui webkit
CONFIG += qtestlib debug_and_release
INCLUDEPATH += ../../src \
	../../../libdebug/include \
	../../../libcparser/include \
	../../../libcparser/antlr_generated \
        ../../../libantlr3c/include \
        ../../../libinsight/include
LIBS += -L../../../libcparser -lcparser \
	-L../../../libdebug -ldebug \
	-L../../../libantlr3c -lantlr3c \
	-L../../../libinsight -linsight \
	-lreadline
QMAKE_CXXFLAGS_DEBUG += -w
QMAKE_CXXFLAGS_RELEASE += -w
