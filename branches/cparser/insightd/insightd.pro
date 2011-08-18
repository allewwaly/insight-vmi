SOURCES += memorydumpsclass.cpp \
    scriptengine.cpp \
    kernelsymbolsclass.cpp \
    memorydifftree.cpp \
    memorymaprangetree.cpp \
    memorymapbuilder.cpp \
    memorymapwindow.cpp \
    memorymapwidget.cpp \
    memorymapnode.cpp \
    memorymap.cpp \
    instancedata.cpp \
    instance.cpp \
    instanceprototype.cpp \
    instanceclass.cpp \
    memspecs.cpp \
    memspecparser.cpp \
    virtualmemory.cpp \
    memorydump.cpp \
    programoptions.cpp \
    longoperation.cpp \
    kernelsymbolparser.cpp \
    kernelsymbolwriter.cpp \
    kernelsymbolreader.cpp \
    volatiletype.cpp \
    funcpointer.cpp \
    refbasetype.cpp \
    typedef.cpp \
    consttype.cpp \
    symbol.cpp \
    array.cpp \
    compileunit.cpp \
    sourceref.cpp \
    shell.cpp \
    referencingtype.cpp \
    genericexception.cpp \
    debug.cpp \
    structuredmember.cpp \
    structured.cpp \
    typeinfo.cpp \
    symfactory.cpp \
    kernelsymbols.cpp \
    variable.cpp \
    pointer.cpp \
    numeric.cpp \
    main.cpp \
    basetype.cpp \
    enum.cpp
HEADERS += memorydumpsclass.h \
    scriptengine.h \
    kernelsymbolsclass.h \
    memorydifftree.h \
    memorymaprangetree.h \
    memoryrangetree.h \
    instance_def.h \
    memorymapbuilder.h \
    priorityqueue.h \
    memorymapwindow.h \
    memorymapwidget.h \
    varsetter.h \
    memorymapnode.h \
    memorymap.h \
    instancedata.h \
    instance.h \
    instanceprototype.h \
    instanceclass.h \
    virtualmemoryexception.h \
    memspecs.h \
    memspecparser.h \
    virtualmemory.h \
    ioexception.h \
    filenotfoundexception.h \
    memorydump.h \
    programoptions.h \
    longoperation.h \
    parserexception.h \
    readerwriterexception.h \
    kernelsymbolconsts.h \
    kernelsymbolparser.h \
    kernelsymbolwriter.h \
    kernelsymbolreader.h \
    volatiletype.h \
    funcpointer.h \
    refbasetype.h \
    typedef.h \
    consttype.h \
    symbol.h \
    array.h \
    compileunit.h \
    sourceref.h \
    shell.h \
    referencingtype.h \
    genericexception.h \
    debug.h \
    structuredmember.h \
    structured.h \
    typeinfo.h \
    symfactory.h \
    kernelsymbols.h \
    variable.h \
    pointer.h \
    numeric.h \
    basetype.h \
    enum.h
CONFIG += console \
    debug_and_release
QT += script \
    network \
    gui
LIBS += -lreadline \
    -L \
    ../libinsight \
    -linsight
INCLUDEPATH += ../libinsight/include
FORMS = memorymapwindow.ui
