TARGETS = drv_w32.obj cpp0main.obj
MAKE = make.exe
MAKER = $(MAKE) -r
GAS2NASK = gas2nask.exe
NASK = nask.exe
DEL = del
CC1 = cc1.exe

%.s : %.c Makefile
	$(CC1) -include ../include/gccdef.h -I../include -Os -quiet -o $*.s $*.c

%.nas : %.s Makefile
	$(GAS2NASK) -a $*.s $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj

all :
	$(MAKER) $(TARGETS)

clean :
	$(DEL) *.obj
