#makefile.am for common
ACLOCAL_AMFLAGS   = -I m4
lib_LTLIBRARIES = libkodiaq.la
libkodiaq_la_SOURCES = kbhit.cc kbhit.hh koHelper.hh koHelper.cc koLogger.hh koLogger.cc koOptions.hh koOptions.cc koNet.hh koNet.cc koNetClient.hh koNetClient.cc koNetServer.hh koNetServer.cc
libkodiaq_la_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC -std=c++11

libkodiaq_la_LDFLAGS = -shared -lncurses
if WITH_DDC10
libkodiaq_la_LDFLAGS += -L$(top_srcdir)/src/ddc10/.libs -lddc
libkodiaq_la_CPPFLAGS += -I$(top_srcdir)/src/ddc10
endif
