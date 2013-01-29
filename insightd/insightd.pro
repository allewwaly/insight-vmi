TEMPLATE = app

# Global configuration file
include(../config.pri)

# Path for target
target.path += $$PREFIX/bin

# Extra target for scripts
scripts.path += $$PREFIX/share/insight/examples
scripts.files = scripts/*.js

# Extra target for tools
tools.path += $$PREFIX/share/insight/tools
tools.files = ../tools/gcc_pp \
	../tools/make-debug-kpkg \
	../tools/mount-img \
	../tools/umount-img

# What to install
INSTALLS += target scripts tools

CONFIG += console \
    debug_and_release

QT += script \
    network \
    xml
QT -= gui \
    webkit

# Required libraries for building
LIBS += \
    -L../libdebug$$BUILD_DIR \
    -l$$DEBUG_LIB \
    -L../libinsight$$BUILD_DIR \
    -l$$INSIGHT_LIB \
    -L../libcparser$$BUILD_DIR \
    -l$$CPARSER_LIB \
    -L../libantlr3c$$BUILD_DIR \
    -l$$ANTLR_LIB

# Inter-project include paths
INCLUDEPATH += ../libinsight/include \
    ../libdebug/include \
    ../libantlr3c/include \
    ../libcparser/include

SOURCES += \
    altreftyperulewriter.cpp \
    kernelsourceparser.cpp \
    main.cpp \
    programoptions.cpp \
    shell.cpp \
    shell_readline.cpp \

HEADERS += altreftype.h \
    altreftyperulewriter.h \
    kernelsourceparser.h \
    programoptions.h \
    shell.h

# Things to do when the memory map builder and widget is to be built. Enabling
# this feature requires InSight to run on an X server.
CONFIG(mem_map_vis) {
    unix:warning(Enabled memory map visualization. The resulting binary will must be run on an X windows system!)

    DEFINES += CONFIG_WITH_X_SUPPORT

    FORMS += memorymapwindow.ui

    SOURCES += memorymapwindow.cpp \
        memorymapwidget.cpp

    HEADERS += memorymapwindow.h \
        memorymapwidget.h

    QT += gui
}

# Enable or disable libreadline support
CONFIG(with_readline) {
    DEFINES += CONFIG_READLINE
    LIBS += -lreadline
}

OTHER_FILES += \
    scripts/samplerule.js
