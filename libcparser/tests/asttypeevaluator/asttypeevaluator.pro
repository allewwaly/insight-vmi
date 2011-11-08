TEMPLATE = app
TARGET = asttypeevaluator
QT += core \
    testlib
CONFIG += qtestlib debug
HEADERS += asttypeevaluatortest.h
SOURCES += asttypeevaluatortest.cpp
INCLUDEPATH += ../../src \
	../../include \
	../../antlr_generated \
	../../../libantlr3c/include 
LIBS += -L ../.. -lcparser \
	-L../../../libantlr3c -lantlr3c
