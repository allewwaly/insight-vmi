TEMPLATE = app
TARGET = insight
isEmpty(PREFIX):PREFIX = /usr/local
target.path += $$PREFIX/bin
INSTALLS += target
QT += core network
CONFIG += debug_and_release
HEADERS += \
    shell.h \
    programoptions.h
SOURCES += \
    shell.cpp \
    programoptions.cpp \
    main.cpp
LIBS += -lreadline \
    -L../libinsight \
    -linsight \
    -L../libdebug \
    -ldebug
INCLUDEPATH += ../libdebug/include \
    ../libinsight/include
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3

