#DAQCommon Makefile

CC		= g++
CFLAGS		= -Wall -g -DLINUX -fPIC
LIBDIR          = ../common
MONGODIR        = ../../mongo/mongo-cxx-driver-v2.4/
LDFLAGS         = -L$(LIBDIR) -L$(MONGODIR) -lXeDAQ -lCAENVME -lmongoclient -lboost_thread-mt -lboost_filesystem -lboost_program_options -lboost_system -lsnappy
SOURCES         = $(shell echo ./*cc)
OBJECTS         = $(SOURCES: .cc=.o)
CPP             = koMaster

CFLAGS          += -I../common

all: $(SOURCES) $(CPP)

$(CPP) : $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(CPP)
#install:

clean:	
	rm $(CPP)


