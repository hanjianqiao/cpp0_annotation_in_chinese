TARGETLIB = libcpp.lib
LIBRARIAN = ../tools/golib00.exe

MAKER = ../tools/make.exe -r
DEL = del
NASK = ../tools/nask.exe
CC1 = ../tools/cc1.exe
GAS2NAS = ../tools/gas2nas.exe

OBJS = cpplex.o cpplib.o cppmacro.o cppexp.o cppfiles.o cpphash.o \
	cpperror.o cppinit.o cppdefault.o line-map.o mkdeps.o prefix.o mbchar.o

all :
	$(MAKER) $(TARGETLIB)

clean :
	-$(DEL) *.o
	-$(DEL) $(TARGETLIB)

$(TARGETLIB) : $(OBJS)
	$(LIBRARIAN) out:$@ $(OBJS)

%.s : %.c
	$(CC1) -include ../include/gccdef.h -I../include -Os -quiet -o $*.s $*.c

%.nas : %.s
	$(GAS2NAS) -a $*.s $*.nas

%.o : %.nas
	$(NASK) $*.nas $*.o

