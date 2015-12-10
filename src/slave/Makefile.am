bin_PROGRAMS = koSlave
koSlave_SOURCES =  koSlave.cc CBV1724.cc CBV1724.hh CBV2718.cc CBV2718.hh DigiInterface.cc DigiInterface.hh VMEBoard.cc VMEBoard.hh DAQRecorder.cc DAQRecorder.hh DataProcessor.cc DataProcessor.hh NCursesUI.hh NCursesUI.cc
koSlave_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC -std=c++11


koSlave_LDADD = -L$(top_srcdir)/src/common/.libs -lkodiaq -lCAENVME -lncurses -lsnappy

if WITH_LIBPBF
koSlave_LDADD += -lpbf -lprotobuf -lsnappy -lpthread
endif

#options flags
#if WITH_MONGODB
#koSlave_LDADD += -lmongoclient -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system -lsnappy
#endif
if WITH_DDC10
koSlave_LDADD += -L$(top_srcdir)/src/ddc10/.libs -lddc
koSlave_CPPFLAGS += -I$(top_srcdir)/src/ddc10
endif
koSlave_LDFLAGS = -static
