# Global configuration file
include(../config.pri)

TEMPLATE = lib
TARGET = cparser
VERSION = 1.0.0
CONFIG += debug_and_release \
    staticlib \
    create_prl \
    warn_off
QT += core
QT -= gui webkit
HEADERS += include/ast_interface.h \
    include/astscopemanager.h \
    include/genericexception.h \
    include/astsymbol.h \
    include/realtypes.h \
    include/astwalker.h \
    include/astsymboltypes.h \
    include/typeevaluatorexception.h \
    include/asttypeevaluator.h \
    include/astdotgraph.h \
    include/abstractsyntaxtree.h \
    include/astnode.h \
    include/astbuilder.h \
    src/C.h \
    antlr_generated/CLexer.h \
    antlr_generated/CParser.h \
    include/astsourceprinter.h \
    include/expressionevalexception.h \
    include/bitop.h \
    include/astnodefinder.h \
    include/typeinfooracle.h \
    include/gzip.h
SOURCES += \
    antlr_generated/CLexer.c \
    antlr_generated/CParser.c \
    src/abstractsyntaxtree.cpp \
    src/ast_interface.cpp \
    src/astbuilder.cpp \
    src/astdotgraph.cpp \
    src/astnode.cpp \
    src/astnodefinder.cpp \
    src/astscopemanager.cpp \
    src/astsourceprinter.cpp \
    src/asttypeevaluator.cpp \
    src/astsymbol.cpp \
    src/astwalker.cpp \
    src/genericexception.cpp \
    src/realtypes.cpp \
    src/gzip.cpp

LIBS += -L../libantlr3c$$BUILD_DIR \
    -l$$ANTLR_LIB \
    -L../libdebug$$BUILD_DIR \
    -l$$DEBUG_LIB
INCLUDEPATH += ./antlr_generated \
    ./src \
    ./include \
    ../libdebug/include \
    ../libantlr3c/include \
    ../libinsight/include












