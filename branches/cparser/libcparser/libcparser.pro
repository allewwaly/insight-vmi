TEMPLATE = lib
TARGET = cparser
VERSION = 1.0.0
CONFIG += debug_and_release \
    staticlib \
    create_prl
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3
QT += core
HEADERS += include/ast_interface.h \
    include/astscopemanager.h \
    include/astnodecounter.h \
    include/genericexception.h \
    include/astsymbol.h \
    include/realtypes.h \
    include/astwalker.h \
    include/astsymboltypes.h \
    include/debug.h \
    include/typeevaluatorexception.h \
    include/asttypeevaluator.h \
    include/astdotgraph.h \
    include/abstractsyntaxtree.h \
    include/astnode.h \
    include/astbuilder.h \
    src/C.h \
    antlr_generated/CLexer.h \
    antlr_generated/CParser.h \
    include/astsourceprinter.h
SOURCES += src/astscopemanager.cpp \
    src/astsymbol.cpp \
    src/realtypes.cpp \
    src/abstractsyntaxtree.cpp \
    src/ast_interface.cpp \
    src/astbuilder.cpp \
    src/astdotgraph.cpp \
    src/astnode.cpp \
    src/astnodecounter.cpp \
    src/asttypeevaluator.cpp \
    src/astwalker.cpp \
    src/debug.cpp \
    src/genericexception.cpp \
    antlr_generated/CLexer.c \
    antlr_generated/CParser.c \
    src/astsourceprinter.cpp
LIBS += -L../libantlr3c \
    -lantlr3c
INCLUDEPATH += ./antlr_generated \
    ./src \
    ./include \
    ../libantlr3c/include




