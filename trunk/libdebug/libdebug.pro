#-------------------------------------------------
#
# Project created by QtCreator 2012-03-02T14:30:24
#
#-------------------------------------------------

QT       -= gui webkit

TARGET = debug
TEMPLATE = lib
CONFIG += debug_and_release staticlib create_prl

INCLUDEPATH += ./include

SOURCES += src/debug.cpp \
    src/bugreport.cpp \
    src/log.cpp

HEADERS += include/debug.h \
    include/bugreport.h \
    include/log.h \
    include/safeflags.h

# Compiler defines for the version information
SVN_REVISION = exported
INSIGHT_RELEASE = unknown
BUILD_DATE = unknown

# Try to find out the SVN revision
unix {
    # Do we have svn available?
    system(svnversion > /dev/null 2>&1) {
        SVN_REVISION = $$system(LANG="" svnversion ../)
    }
    # Read the major version from the changelog file
    exists(../debian/changelog) {
        INSIGHT_RELEASE = $$system(awk \'{if (!s && /^insight/i) s=$2} END {print substr(s, 2, length(s)-2)}\' ../debian/changelog)
    }
    BUILD_DATE = $$system(date --utc "+%s")
}

DEFINES += SVN_REVISION=\"\\\"$$SVN_REVISION\\\"\" \
    INSIGHT_RELEASE=\"\\\"$$INSIGHT_RELEASE\\\"\" \
    BUILD_DATE=\"\\\"$$BUILD_DATE\\\"\"
