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

# Names of the libs
INSIGHT_LIB = insight
CPARSER_LIB = cparser
ANTLR_LIB = antlr3c
DEBUG_LIB = debug

win32 {
    # Dynamically linked libs have a version suffix
    INSIGHT_LIB = insight1
    ANTLR_LIB = antlr3c3
}
