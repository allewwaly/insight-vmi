# Set PREFIX for installation, if not set
isEmpty(PREFIX) {
    unix:PREFIX = /usr/local
    win32:PREFIX = quote(C:\\Program Files)
}

# Should the memory_map feature be built? Requires the X window system
# for Unix. Disabled by default.
CONFIG += memory_map

# Build with readline support? Default yes for Unix, no for Windows
unix:CONFIG += with_readline

# Directory where libraries are created
BUILD_DIR =
win32 {
    BUILD_DIR = /release
    CONFIG(debug): BUILD_DIR = /debug
}

# Enable high optimization
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3

# Names of the libs
INSIGHT_LIB = insight
CPARSER_LIB = cparser
ANTLR_LIB = antlr3c
DEBUG_LIB = debug

# Windows specific configuration
win32 {
    # Dynamically linked libs have a version suffix
    INSIGHT_LIB = insight1

    # Avoid compiler warnings from MinGW
    QMAKE_LFLAGS_DEBUG += --enable-auto-import
    QMAKE_LFLAGS_RELEASE += --enable-auto-import
}
