SOURCEDIR ?= .
DESTDIR ?= .

SUBDIRS = libmemtool memtoold memtool

# Where to put binary on 'make install'?
BIN     = $(DESTDIR)/usr/bin
LIB     = $(DESTDIR)/usr/lib
INCLUDE = $(DESTDIR)/usr/include
SHARE   = $(DESTDIR)/usr/share

all: build

distclean: 
	@echo clean
	@for dir in $(SUBDIRS) ; do cd $$dir ; make clean ; rm -rf Makefile ; cd $(PWD) ;done
	

build:
	@for dir in $(SUBDIRS) ; do cd $$dir ; qmake -makefile ; make ; cd $(PWD) ;done


install:
	@##create install directory
	@install -d $(DESTDIR)
	@## libmemtool
	@install -Dp libmemtool/libmemtool.so.1.0.0 $(LIB)/libmemtool.so.1.0.0
	@ln -s ./libmemtool.so.1.0.0   $(LIB)/libmemtool.so
	@ln -s ./libmemtool.so.1.0.0   $(LIB)/libmemtool.so.1
	@ln -s ./libmemtool.so.1.0.0   $(LIB)/libmemtool.so.1.0
	@install -d $(INCLUDE)/
	@svn export --force libmemtool/include/memtool     $(INCLUDE)/memtool > /dev/null
	@##memtoold
	@install -Dp memtoold/memtoold  $(BIN)/memtoold
	@install -d $(SHARE)/memtool
	@svn export --force memtoold/scripts   $(SHARE)/memtool/scripts > /dev/null
	@##memtool
	@install -Dp memtool/memtool    $(BIN)/memtool

%:
	@echo $@
	@echo "$@ Todo create this target"
	@#for i in `ls -d mem*/ -1` ; do cd $i ; make clean && make ; cd - ; done
