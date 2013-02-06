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

# libinsight headers
HEADERS += ../libinsight/include/insight/altreftype.h \
    ../libinsight/include/insight/array.h \
    ../libinsight/include/insight/astexpressionevaluator.h \
    ../libinsight/include/insight/astexpression.h \
    ../libinsight/include/insight/basetype.h \
    ../libinsight/include/insight/colorpalette.h \
    ../libinsight/include/insight/compiler.h \
    ../libinsight/include/insight/compileunit.h \
    ../libinsight/include/insight/console.h \
    ../libinsight/include/insight/constdefs.h \
    ../libinsight/include/insight/consttype.h \
    ../libinsight/include/insight/devicemuxer.h \
    ../libinsight/include/insight/embedresult.h \
    ../libinsight/include/insight/enum.h \
    ../libinsight/include/insight/errorcodes.h \
    ../libinsight/include/insight/expressionresult.h \
    ../libinsight/include/insight/filenotfoundexception.h \
    ../libinsight/include/insight/filterexception.h \
    ../libinsight/include/insight/funcparam.h \
    ../libinsight/include/insight/funcpointer.h \
    ../libinsight/include/insight/function.h \
    ../libinsight/include/insight/insightexception.h \
    ../libinsight/include/insight/insight.h \
    ../libinsight/include/insight/instanceclass.h \
    ../libinsight/include/insight/instancedata.h \
    ../libinsight/include/insight/instance_def.h \
    ../libinsight/include/insight/instance.h \
    ../libinsight/include/insight/instanceprototype.h \
    ../libinsight/include/insight/ioexception.h \
    ../libinsight/include/insight/kernelsourcetypeevaluator.h \
    ../libinsight/include/insight/kernelsymbolconsts.h \
    ../libinsight/include/insight/kernelsymbolparser.h \
    ../libinsight/include/insight/kernelsymbolreader.h \
    ../libinsight/include/insight/kernelsymbolsclass.h \
    ../libinsight/include/insight/kernelsymbols.h \
    ../libinsight/include/insight/kernelsymbolstream.h \
    ../libinsight/include/insight/kernelsymbolwriter.h \
    ../libinsight/include/insight/keyvaluestore.h \
    ../libinsight/include/insight/longoperation.h \
    ../libinsight/include/insight/memberlist.h \
    ../libinsight/include/insight/memorydifftree.h \
    ../libinsight/include/insight/memorydump.h \
    ../libinsight/include/insight/memorydumpsclass.h \
    ../libinsight/include/insight/memorymapbuildercs.h \
    ../libinsight/include/insight/memorymapbuilder.h \
    ../libinsight/include/insight/memorymapbuildersv.h \
    ../libinsight/include/insight/memorymap.h \
    ../libinsight/include/insight/memorymapheuristics.h \
    ../libinsight/include/insight/memorymapnode.h \
    ../libinsight/include/insight/memorymapnodesv.h \
    ../libinsight/include/insight/memorymaprangetree.h \
    ../libinsight/include/insight/memorymapverifier.h \
    ../libinsight/include/insight/memoryrangetree.h \
    ../libinsight/include/insight/memspecparser.h \
    ../libinsight/include/insight/memspecs.h \
    ../libinsight/include/insight/multithreading.h \
    ../libinsight/include/insight/numeric.h \
    ../libinsight/include/insight/osfilter.h \
    ../libinsight/include/insight/parserexception.h \
    ../libinsight/include/insight/pointer.h \
    ../libinsight/include/insight/priorityqueue.h \
    ../libinsight/include/insight/readerwriterexception.h \
    ../libinsight/include/insight/refbasetype.h \
    ../libinsight/include/insight/referencingtype.h \
    ../libinsight/include/insight/scriptengine.h \
    ../libinsight/include/insight/shellutil.h \
    ../libinsight/include/insight/slubobjects.h \
    ../libinsight/include/insight/sourceref.h \
    ../libinsight/include/insight/structured.h \
    ../libinsight/include/insight/structuredmember.h \
    ../libinsight/include/insight/symbol.h \
    ../libinsight/include/insight/symfactory.h \
    ../libinsight/include/insight/typedef.h \
    ../libinsight/include/insight/typefilter.h \
    ../libinsight/include/insight/typeinfo.h \
    ../libinsight/include/insight/typeruleenginecontextprovider.h \
    ../libinsight/include/insight/typeruleengine.h \
    ../libinsight/include/insight/typeruleexception.h \
    ../libinsight/include/insight/typerule.h \
    ../libinsight/include/insight/typeruleparser.h \
    ../libinsight/include/insight/typerulereader.h \
    ../libinsight/include/insight/variable.h \
    ../libinsight/include/insight/varsetter.h \
    ../libinsight/include/insight/virtualmemoryexception.h \
    ../libinsight/include/insight/virtualmemory.h \
    ../libinsight/include/insight/volatiletype.h \
    ../libinsight/include/insight/xmlschema.h

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
