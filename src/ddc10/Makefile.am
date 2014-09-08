#makefile.am for ddc10
ACLOCAL_AMFLAGS   = -I m4
lib_LTLIBRARIES = libddc.la
libddc_la_SOURCES = ddc_10.cc ddc_10.hh
libddc_la_CPPFLAGS = -I$(top_srcdir)/src/ddc10 -Wall -g -DLINUX -fPIC 
libddc_la_LDFLAGS = -shared -lexpect #-ltcl8.5
