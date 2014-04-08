bin_PROGRAMS = koMaster
koMaster_SOURCES =  koMaster.cc MasterMongodbConnection.cc MasterMongodbConnection.hh
koMaster_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
koMaster_LDADD = -L$(top_srcdir)/src/common/.libs -lkodiaq -lmongoclient -lboost_thread -lboost_filesystem -lboost_program_options -lboost_system -lrt
if WITH_DDC10
koMaster_LDADD += -L$(top_srcdir)/src/ddc10/.libs -lddc
koMaster_CPPFLAGS += -I$(top_srcdir)/src/ddc10
endif
koMaster_LDFLAGS = -static