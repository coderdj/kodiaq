#DAQSlave Makefile

CC              = g++
CFLAGS          = -Wall -g -DLINUX -fPIC -fopenmp
MONGODIR        = ../../mongo/mongo-cxx-driver-v2.4/
COMMONDIR       = ../common
LDFLAGS         = -L$(COMMONDIR) -L$(MONGODIR) -L/usr/lib -lXeDAQ -lCAENVME -lmongoclient -lboost_thread-mt -lboost_filesystem -lboost_program_options -lboost_system  -lsnappy
SOURCES         = $(shell echo ./*cc)
OBJECTS         = $(SOURCES: .cc=.o)
CPP             = koSlave

CFLAGS         += -I../common -I$(MONGODIR)/src

all: $(SOURCES) $(CPP)

$(CPP) : $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(CPP) 

#install:
clean:
	rm $(CPP)
