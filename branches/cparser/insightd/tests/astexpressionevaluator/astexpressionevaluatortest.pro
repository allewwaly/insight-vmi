TEMPLATE = app
TARGET = astexpressionevaluatortester
HEADERS = astexpressionevaluatortester.h
SOURCES = astexpressionevaluatortester.cpp
QT += core \
    testlib
CONFIG += qtestlib
INCLUDEPATH += ../../src \
	../../../libcparser/include \
	../../../libcparser/antlr_generated \
	../../../libantlr3c/include
LIBS += -L../../../libcparser -lcparser \
	-L../../../libantlr3c -lantlr3c
