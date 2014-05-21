#
# (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
#
# @@@ END COPYRIGHT @@@

include $(MY_SQROOT)/macros.gmk
include ./macros.gmk

SQINSTALL=$(MY_SQROOT)/export

#SM library files
LIBSRC=libsm.c
LIBOBJ=$(LIBSRC:.c=.o)
CCFLAGS += -fpic
INCSRC=sm.h
INCEXPDIR	= $(SQINSTALL)/include

LIBSM		= $(SQINSTALL)/lib$(SQ_MBTYPE)/libsm.so
INCSM		= $(INCEXPDIR)/sm.h

#Monitor startup
CCFLAGS += -Wno-long-long -fmessage-length=0
INCLUDES = -I$(INCEXPDIR)

EXE=$(LIBSM) $(INCSM)

all: $(EXE)

sq-local: $(EXE)

clean:
	rm -f *.o $(LIBSM) $(INCSM)

clean-local:
	rm -f *.o $(LIBSM) $(INCSM)

$(LIBOBJ): $(LIBSRC) $(INCSRC)

%.o: %.c
	$(CC) $(CCFLAGS) $(INCLUDES) $*.c $(LDFLAGS) -c

$(LIBSM): $(LIBOBJ)
	-mkdir -p $(SP_EXPORT_LIB)
	$(CC) $(LDFLAGS) $(CCFLAGS) -Wall -Werror -shared \
		-Wl,-soname,libsm.so -o $@ $(LIBOBJ) -lrt

$(INCSM): $(INCSRC)
	-mkdir -p $(INCEXPDIR)
	cp $(INCSRC) $(INCEXPDIR)
