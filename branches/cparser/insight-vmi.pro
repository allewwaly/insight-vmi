SUBDIRS += libdebug libantlr3c libcparser libcparser/tests libinsight insightd insightd/tests insight
TEMPLATE = subdirs
CONFIG += debug_and_release \
          warn_on \
          ordered
