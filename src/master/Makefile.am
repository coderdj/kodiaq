bin_PROGRAMS = koMaster
koMaster_SOURCES =  koMaster.cc 
koMaster_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
koMaster_LDADD = -L$(top_srcdir)/src/common/.libs -lXeDAQ
koMaster_LDFLAGS = -static