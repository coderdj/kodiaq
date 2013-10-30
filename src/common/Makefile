#DAQCommon Makefile

CC		= g++
CFLAGS		= -Wall -g -DLINUX -fPIC
LDFLAGS         = -shared
SOURCES         = $(shell echo ./*cc)
OBJECTS         = $(SOURCES: .cc=.o)
SO              = libXeDAQ.so

#CFLAGS          += -I/usr/include/ncurses

all: $(SO)

$(SO) : $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(CFLAGS) $(LIBS) -o $(SO)
#install:

clean:	
	rm $(SO)


