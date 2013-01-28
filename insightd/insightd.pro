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

SOURCES += altreftype.cpp \
    altreftyperulewriter.cpp \
    array.cpp \
    astexpression.cpp \
    astexpressionevaluator.cpp \
    basetype.cpp \
    colorpalette.cpp \
    compileunit.cpp \
    consttype.cpp \
    enum.cpp \
    expressionresult.cpp \
    funcparam.cpp \
    funcpointer.cpp \
    function.cpp \
    instanceclass.cpp \
    instance.cpp \
    instancedata.cpp \
    instanceprototype.cpp \
    kernelsourceparser.cpp \
    kernelsourcetypeevaluator.cpp \
    kernelsymbolparser.cpp \
    kernelsymbolreader.cpp \
    kernelsymbolsclass.cpp \
    kernelsymbols.cpp \
    kernelsymbolstream.cpp \
    kernelsymbolwriter.cpp \
    longoperation.cpp \
    main.cpp \
    memorydifftree.cpp \
    memorydump.cpp \
    memorydumpsclass.cpp \
    memorymapbuilder.cpp \
    memorymapbuildercs.cpp \
    memorymapbuildersv.cpp \
    memorymap.cpp \
    memorymapheuristics.cpp \
    memorymapnode.cpp \
    memorymapnodesv.cpp \
    memorymaprangetree.cpp \
    memorymapverifier.cpp \
    memspecparser.cpp \
    memspecs.cpp \
    numeric.cpp \
    osfilter.cpp \
    pointer.cpp \
    programoptions.cpp \
    refbasetype.cpp \
    referencingtype.cpp \
    scriptengine.cpp \
    shell.cpp \
    shell_readline.cpp \
    shellutil.cpp \
    slubobjects.cpp \
    sourceref.cpp \
    structured.cpp \
    structuredmember.cpp \
    symbol.cpp \
    symfactory.cpp \
    typedef.cpp \
    typefilter.cpp \
    typeinfo.cpp \
    typerule.cpp \
    typeruleengine.cpp \
    typeruleparser.cpp \
    typerulereader.cpp \
    variable.cpp \
    virtualmemory.cpp \
    volatiletype.cpp \
    xmlschema.cpp \
    typeruleenginecontextprovider.cpp \
    console.cpp


HEADERS += altreftype.h \
    altreftyperulewriter.h \
    array.h \
    astexpressionevaluator.h \
    astexpression.h \
    basetype.h \
    colorpalette.h \
    compileunit.h \
    consttype.h \
    enum.h \
    expressionresult.h \
    filenotfoundexception.h \
    filterexception.h \
    funcparam.h \
    funcpointer.h \
    function.h \
    instanceclass.h \
    instancedata.h \
    instance_def.h \
    instance.h \
    instanceprototype.h \
    ioexception.h \
    kernelsourceparser.h \
    kernelsourcetypeevaluator.h \
    kernelsymbolconsts.h \
    kernelsymbolparser.h \
    kernelsymbolreader.h \
    kernelsymbolsclass.h \
    kernelsymbols.h \
    kernelsymbolstream.h \
    kernelsymbolwriter.h \
    keyvaluestore.h \
    longoperation.h \
    memberlist.h \
    memorydifftree.h \
    memorydump.h \
    memorydumpsclass.h \
    memorymapbuildercs.h \
    memorymapbuilder.h \
    memorymapbuildersv.h \
    memorymap.h \
    memorymapheuristics.h \
    memorymapnode.h \
    memorymapnodesv.h \
    memorymaprangetree.h \
    memorymapverifier.h \
    memoryrangetree.h \
    memspecparser.h \
    memspecs.h \
    numeric.h \
    osfilter.h \
    parserexception.h \
    pointer.h \
    priorityqueue.h \
    programoptions.h \
    readerwriterexception.h \
    refbasetype.h \
    referencingtype.h \
    scriptengine.h \
    shell.h \
    shellutil.h \
    slubobjects.h \
    sourceref.h \
    structured.h \
    structuredmember.h \
    symbol.h \
    symfactory.h \
    typedef.h \
    typefilter.h \
    typeinfo.h \
    typeruleengine.h \
    typeruleexception.h \
    typerule.h \
    typeruleparser.h \
    typerulereader.h \
    variable.h \
    varsetter.h \
    virtualmemoryexception.h \
    virtualmemory.h \
    volatiletype.h \
    xmlschema.h \
    embedresult.h \
    typeruleenginecontextprovider.h \
    console.h \
    errorcodes.h

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
