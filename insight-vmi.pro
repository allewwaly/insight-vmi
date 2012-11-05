# Global configuration file
include(config.pri)

SUBDIRS += libdebug libantlr3c libcparser libinsight insightd insight
TEMPLATE = subdirs
CONFIG += debug_and_release \
          warn_on \
          ordered

# Build tests, if requested
CONFIG(tests): SUBDIRS += tests libcparser/tests insightd/tests
