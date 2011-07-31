SOURCEDIR ?= .
DESTDIR ?= .
PREFIX ?= /usr 

SUBDIRS = libinsight insightd insight

# Where to put binary on 'make install'?
BIN     = $(DESTDIR)$(PREFIX)/bin
LIB     = $(DESTDIR)$(PREFIX)/lib
INCLUDE = $(DESTDIR)$(PREFIX)/include
SHARE   = $(DESTDIR)$(PREFIX)/share

all: build

clean:
	@echo clean
	@for dir in $(SUBDIRS) ; do cd $$dir ; make clean ; cd $(PWD) ; done
	
distclean:
	@echo distclean
	@for dir in $(SUBDIRS) ; do cd $$dir ; make distclean ; rm -rf Makefile ; cd $(PWD) ; done
	

build:
	@for dir in $(SUBDIRS) ; do cd $$dir ; qmake -makefile && make release ; cd $(PWD) ; done


install:
	@##create install directory
	@install -v -d $(DESTDIR)
	@## libinsight
	@install -v -Dp libinsight/libinsight.so.1.0.0 $(LIB)/libinsight.so.1.0.0
	@ln -sv ./libinsight.so.1.0.0   $(LIB)/libinsight.so
	@ln -sv ./libinsight.so.1.0.0   $(LIB)/libinsight.so.1
	@ln -sv ./libinsight.so.1.0.0   $(LIB)/libinsight.so.1.0
	@install -v -d $(INCLUDE)/
	@svn export --force libinsight/include/insight     $(INCLUDE)/insight > /dev/null
	@##insightd
	@install -v -Dp insightd/insightd  $(BIN)/insightd
	@install -v -d $(SHARE)/insight
	@svn export --force insightd/scripts   $(SHARE)/insight/scripts > /dev/null
	@##insight
	@install -v -Dp insight/insight    $(BIN)/insight

#%:
#	@echo $@
#	@echo "$@ Todo create this target"
#	@#for i in `ls -d mem*/ -1` ; do cd $i ; make clean && make ; cd - ; done
