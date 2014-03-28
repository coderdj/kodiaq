#makefile.am for common
ACLOCAL_AMFLAGS   = -I m4
lib_LTLIBRARIES = libkodiaq.la
libkodiaq_la_SOURCES = kbhit.cc kbhit.hh koHelper.hh koHelper.cc koLogger.hh koLogger.cc koOptions.hh koOptions.cc koNet.hh koNet.cc koNetClient.hh koNetClient.cc koNetServer.hh koNetServer.cc
libkodiaq_la_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
libkodiaq_la_LDFLAGS = -shared
