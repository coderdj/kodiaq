bin_PROGRAMS = koSlave
koSlave_SOURCES =  koSlave.cc CBV1724.cc CBV1724.hh CBV2718.cc CBV2718.hh DigiInterface.cc DigiInterface.hh VMEBoard.cc VMEBoard.hh VMECrate.cc VMECrate.hh XeDAQRecorder.cc XeDAQRecorder.hh XeMongoRecorder.cc XeMongoRecorder.hh XeProcessor.cc XeProcessor.hh 
koSlave_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
koSlave_LDADD = -L$(top_srcdir)/src/common/.libs -lXeDAQ -lCAENVME -lmongoclient -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system  -lsnappy -lrt
#koSlave_LDADD = -L$(top_srcdir)/src/common/.libs -lXeDAQ
koSlave_LDFLAGS = -static