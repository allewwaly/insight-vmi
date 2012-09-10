# Global configuration file
include(../config.pri)

TEMPLATE = app
TARGET = insight
target.path += $$PREFIX/bin
INSTALLS += target
QT += core network
QT -= gui webkit
CONFIG += debug_and_release
HEADERS += \
    shell.h \
    programoptions.h
SOURCES += \
    shell.cpp \
    programoptions.cpp \
    main.cpp
LIBS += \
    -L../libinsight$$BUILD_DIR \
    -l$$INSIGHT_LIB \
    -L../libdebug$$BUILD_DIR \
    -l$$DEBUG_LIB
INCLUDEPATH += ../libdebug/include \
    ../libinsight/include
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3

# Enable or disable libreadline support
CONFIG(with_readline) {
    DEFINES += CONFIG_READLINE
    LIBS += -lreadline
}
