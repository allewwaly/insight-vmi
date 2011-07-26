TEMPLATE = app
TARGET = insight
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
    ../libinsight \
    -linsight
INCLUDEPATH += ../libinsight/include
