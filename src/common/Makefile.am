#makefile.am for common
ACLOCAL_AMFLAGS   = -I m4
lib_LTLIBRARIES = libXeDAQ.la
libXeDAQ_la_SOURCES = kbhit.cc kbhit.hh XeDAQHelper.hh XeDAQHelper.cc XeDAQLogger.hh XeDAQLogger.cc XeDAQOptions.hh XeDAQOptions.cc XeNet.hh XeNet.cc XeNetClient.hh XeNetClient.cc XeNetServer.hh XeNetServer.cc
libXeDAQ_la_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
libXeDAQ_la_LDFLAGS = -shared
#libXeDAQ_la_LIBADD = ../src/common/libXeDAQ.la