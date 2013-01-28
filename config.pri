# Set PREFIX for installation, if not set
isEmpty(PREFIX) {
    unix:PREFIX = /usr/local
    win32:PREFIX = quote(C:\\Program Files)
}

# Should the tests be built?
CONFIG += tests

# Disable type-safe enums in release mode
CONFIG(release): DEFINES += NO_TYPESAFE_FLAGS
CONFIG(debug): DEFINES -= NO_TYPESAFE_FLAGS

# Should the memory map visualization be built? Requires the X window system
# for Unix. Disabled by default.
CONFIG += mem_map_vis

# Build with readline support? Default yes for Unix, no for Windows
unix:CONFIG += with_readline

# Should we enabled ANSI colors for terminal output? Ignored for Windows.
CONFIG += with_colors

#---------------------------[ End of configuration ]----------------------------

# Directory where libraries are created
BUILD_DIR =
win32 {
    BUILD_DIR = /release
    CONFIG(debug): BUILD_DIR = /debug
}

# Enable high optimization
QMAKE_CFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE += -O3

# Enable C++11 support for GCC
contains(QMAKE_CXX, g++): QMAKE_CXXFLAGS += -std=c++0x

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

# If building without colorized output, define the corresponding flag
!CONFIG(with_colors) {
    DEFINES += NO_ANSI_COLORS
}
