TEMPLATE = app
TARGET = memtool
QT += core network
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
