TEMPLATE = app
TARGET = asttypeevaluator
QT += core \
    testlib
CONFIG += qtestlib debug_and_release
HEADERS += asttypeevaluatortest.h
SOURCES += asttypeevaluatortest.cpp
INCLUDEPATH += ../../src \
	../../include \
	../../../libdebug/include \
	../../antlr_generated \
	../../../libantlr3c/include 
LIBS += -L ../.. -lcparser \
	-L../../../libdebug -ldebug \
	-L../../../libantlr3c -lantlr3c
