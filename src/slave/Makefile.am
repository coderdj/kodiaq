bin_PROGRAMS = koSlave
koSlave_SOURCES =  koSlave.cc CBV1724.cc CBV1724.hh CBV2718.cc CBV2718.hh DigiInterface.cc DigiInterface.hh VMEBoard.cc VMEBoard.hh VMECrate.cc VMECrate.hh DAQRecorder.cc DAQRecorder.hh DataProcessor.cc DataProcessor.hh
koSlave_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
if WITH_MONGODB
koSlave_LDADD = -L$(top_srcdir)/src/common/.libs -lkodiaq -lCAENVME -lmongoclient -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system  -lsnappy -lrt
else
koSlave_LDADD = -L$(top_srcdir)/src/common/.libs -lkodiaq -lCAENVME
endif
koSlave_LDFLAGS = -static