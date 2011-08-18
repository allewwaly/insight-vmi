
EXTRA_CFLAGS += -I/usr/include
KDIR := /lib/modules/$(shell uname -r)/build

obj-m += memconsts.o

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
	@rm -fv memconsts

prog: memconsts.o
#	Just try both 64 bit and 32 bit cross-compilation
	gcc -m64 -o memconsts memconsts.o || gcc -m32 -o memconsts memconsts.o
