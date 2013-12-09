#empty
#AM_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
bin_PROGRAMS = koUser
koUser_SOURCES =  koUser.cc XeCursesInterface.cc XeCursesInterface.hh
koUser_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
koUser_LDFLAGS = -L$(top_srcdir)/src/common/.libs -lXeDAQ -lmenu -lform -lncurses -ltinfo
