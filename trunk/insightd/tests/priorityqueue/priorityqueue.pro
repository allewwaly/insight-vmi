TEMPLATE = app
TARGET = priorityqueuetester
HEADERS = priorityqueuetester.h ../../priorityqueue.h
SOURCES = priorityqueuetester.cpp
QT += core \
    testlib
QT -= gui webkit
CONFIG += qtestlib debug_and_release
