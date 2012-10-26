# Global configuration file
include(config.pri)

SUBDIRS += libdebug libantlr3c libcparser libcparser/tests libinsight insightd insightd/tests insight
TEMPLATE = subdirs
CONFIG += debug_and_release \
          warn_on \
          ordered
# Build tests, if requested
CONFIG(memory_map): SUBDIRS += tests
