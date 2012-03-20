TEMPLATE = lib
TARGET = antlr3c
VERSION = 3.0.1
CONFIG += debug_and_release staticlib create_prl warn_off
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3
SOURCES = src/antlr3baserecognizer.c \
    src/antlr3basetree.c \
    src/antlr3basetreeadaptor.c \
    src/antlr3bitset.c \
    src/antlr3collections.c \
    src/antlr3commontoken.c \
    src/antlr3commontree.c \
    src/antlr3commontreeadaptor.c \
    src/antlr3commontreenodestream.c \
    src/antlr3cyclicdfa.c \
    src/antlr3encodings.c \
    src/antlr3exception.c \
    src/antlr3filestream.c \
    src/antlr3inputstream.c \
    src/antlr3intstream.c \
    src/antlr3lexer.c \
    src/antlr3parser.c \
    src/antlr3rewritestreams.c \
    src/antlr3string.c \
    src/antlr3stringstream.c \
    src/antlr3tokenstream.c \
    src/antlr3treeparser.c \
    src/antlr3ucs2inputstream.c
HEADERS = include/antlr3.h \
    include/antlr3baserecognizer.h \
    include/antlr3basetree.h \
    include/antlr3basetreeadaptor.h \
    include/antlr3bitset.h \
    include/antlr3collections.h \
    include/antlr3commontoken.h \
    include/antlr3commontree.h \
    include/antlr3commontreeadaptor.h \
    include/antlr3commontreenodestream.h \
    include/antlr3config.h \
    include/antlr3cyclicdfa.h \
    include/antlr3defs.h \
    include/antlr3encodings.h \
    include/antlr3errors.h \
    include/antlr3exception.h \
    include/antlr3filestream.h \
    include/antlr3input.h \
    include/antlr3interfaces.h \
    include/antlr3intstream.h \
    include/antlr3lexer.h \
    include/antlr3memory.h \
    include/antlr3parser.h \
    include/antlr3parsetree.h \
    include/antlr3rewritestreams.h \
    include/antlr3string.h \
    include/antlr3stringstream.h \
    include/antlr3tokenstream.h \
    include/antlr3treeparser.h
INCLUDEPATH += ./include

