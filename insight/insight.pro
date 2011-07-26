TEMPLATE = app
TARGET = memtool
QT += core network
CONFIG += debug
HEADERS += debug.h \
    shell.h \
    programoptions.h
SOURCES += debug.cpp \
    shell.cpp \
    programoptions.cpp \
    main.cpp
LIBS += -lreadline \
    -L \
    ../libmemtool \
    -lmemtool
INCLUDEPATH += ../libmemtool/include
