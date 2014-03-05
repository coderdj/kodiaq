bin_PROGRAMS = koMaster
koMaster_SOURCES =  koMaster.cc MasterMongodbConnection.cc MasterMongodbConnection.hh
koMaster_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
koMaster_LDADD = -L$(top_srcdir)/src/common/.libs -lXeDAQ -lmongoclient -lboost_thread-mt -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system -lrt
koMaster_LDFLAGS = -static