TEMPLATE = app
TARGET = insight
QT += core network
CONFIG += debug_and_release
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
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3

