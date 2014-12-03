bin_PROGRAMS = koMaster
koMaster_SOURCES =  koMaster.cc MasterMongodbConnection.cc MasterMongodbConnection.hh DAQMonitor.hh DAQMonitor.cc
koMaster_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC -std=c++11
koMaster_LDADD = -L$(top_srcdir)/src/common/.libs -lkodiaq

if WITH_DDC10
koMaster_LDADD += -L$(top_srcdir)/src/ddc10/.libs -lddc
koMaster_CPPFLAGS += -I$(top_srcdir)/src/ddc10
endif
koMaster_LDFLAGS = -static
