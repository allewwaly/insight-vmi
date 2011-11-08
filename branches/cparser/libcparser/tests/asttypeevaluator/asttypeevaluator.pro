TEMPLATE = app
TARGET = asttypeevaluator
QT += core \
    testlib
CONFIG += qtestlib debug
HEADERS += asttypeevaluatortest.h \
    ../../../insightd/volatiletype.h \
    ../../../insightd/virtualmemoryexception.h \
    ../../../insightd/virtualmemory.h \
    ../../../insightd/varsetter.h \
    ../../../insightd/variable.h \
    ../../../insightd/typedef.h \
    ../../../insightd/symfactory.h \
    ../../../insightd/symbol.h \
    ../../../insightd/structuredmember.h \
    ../../../insightd/structured.h \
    ../../../insightd/sourceref.h \
    ../../../insightd/shell.h \
    ../../../insightd/referencingtype.h \
    ../../../insightd/refbasetype.h \
    ../../../insightd/pointer.h \
    ../../../insightd/parserexception.h \
    ../../../insightd/numeric.h \
    ../../../insightd/memspecs.h \
    ../../../insightd/kernelsymbols.h \
    ../../../insightd/kernelsourcetypeevaluator.h \
    ../../../insightd/kernelsourceparser.h \
    ../../../insightd/ioexception.h \
    ../../../insightd/function.h \
    ../../../insightd/funcpointer.h \
    ../../../insightd/funcparam.h \
    ../../../insightd/expressionevalexception.h \
    ../../../insightd/enum.h \
    ../../../insightd/debug.h \
    ../../../insightd/consttype.h \
    ../../../insightd/compileunit.h \
    ../../../insightd/basetype.h \
    ../../../insightd/astexpressionevaluator.h \
    ../../../insightd/astexpression.h
SOURCES += asttypeevaluatortest.cpp \
    ../../../insightd/volatiletype.cpp \
    ../../../insightd/virtualmemory.cpp \
    ../../../insightd/variable.cpp \
    ../../../insightd/typedef.cpp \
    ../../../insightd/symfactory.cpp \
    ../../../insightd/symbol.cpp \
    ../../../insightd/structuredmember.cpp \
    ../../../insightd/structured.cpp \
    ../../../insightd/sourceref.cpp \
    ../../../insightd/shell_readline.cpp \
    ../../../insightd/shell.cpp \
    ../../../insightd/referencingtype.cpp \
    ../../../insightd/refbasetype.cpp \
    ../../../insightd/pointer.cpp \
    ../../../insightd/numeric.cpp \
    ../../../insightd/memspecs.cpp \
    ../../../insightd/kernelsymbols.cpp \
    ../../../insightd/kernelsourcetypeevaluator.cpp \
    ../../../insightd/kernelsourceparser.cpp \
    ../../../insightd/function.cpp \
    ../../../insightd/funcpointer.cpp \
    ../../../insightd/funcparam.cpp \
    ../../../insightd/enum.cpp \
    ../../../insightd/debug.cpp \
    ../../../insightd/consttype.cpp \
    ../../../insightd/compileunit.cpp \
    ../../../insightd/basetype.cpp \
    ../../../insightd/astexpressionevaluator.cpp
INCLUDEPATH += ../../src \
	../../include \
	../../antlr_generated \
	../../../libantlr3c/include 
LIBS += -L ../.. -lcparser \
	-L../../../libantlr3c -lantlr3c


