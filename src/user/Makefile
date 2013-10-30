#DAQCommon Makefile

CC		= g++
CFLAGS		= -Wall -g -DLINUX -fPIC
LIBDIR          = ../common
LDFLAGS         = -L$(LIBDIR) -lXeDAQ -lmenu -lform -lncurses -ltinfo
SOURCES         = $(shell echo ./*cc)
OBJECTS         = $(SOURCES: .cc=.o)
CPP             = koUser

CFLAGS          += -I../common -I/usr/include/ncurses

all: $(SOURCES) $(CPP)

$(CPP) : $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(CPP)
#install:

clean:	
	rm $(CPP)


