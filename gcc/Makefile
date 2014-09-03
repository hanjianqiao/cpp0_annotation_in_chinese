LINK = $(LD) -s -Bdynamic
LIBPATH = ../tools/win32/
LINK_ADDLIB = $(LIBPATH)libmingw.lib $(LIBPATH)libiberty.lib $(LIBPATH)go_lib.lib
INTLLIBS = $(LIBPATH)libintl.lib

MAKE = ../tools/make.exe
GAS2NASK = ../tools/gas2nask.exe
NASK = ../tools/nask.exe
MAKER = $(MAKE) -r
MAKEC = $(MAKE) -C
LD = ../tools/ld.exe
DEL = del
CC1 = cc1.exe

GODRV = ../drv_w32/drv_w32.obj
GODRVDIR = ../drv_w32
LINKOPT_CPP0 = --stack 0x1a00000

%.s : %.c
	$(CC1) -include ../include/gccdef.h -I../include -Os -quiet -o $*.s $*.c

%.nas : %.s
	$(GAS2NASK) -a $*.s $*.nas

%.o : %.nas
	$(NASK) $*.nas $*.o

LIBS = $(INTLLIBS)

all :
	$(MAKER) cpp0.exe

clean :
	$(MAKER) -C ../drv_w32 clean
	-$(DEL) *.exe
	-$(DEL) *.o

cpp0.exe : cppmain.o intl.o hashtable.o version.o
	$(MAKEC) $(GODRVDIR)
	$(LINK) -o $@ $(LINKOPT_CPP0) $(GODRVDIR)/cpp0main.obj cppmain.o intl.o \
		$(LIBPATH)libcpp.lib hashtable.o version.o $(LIBS) $(GODRV) $(LINK_ADDLIB)
