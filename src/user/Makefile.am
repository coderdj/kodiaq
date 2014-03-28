#empty
bin_PROGRAMS = koUser
koUser_SOURCES =  koUser.cc NCursesUI.cc NCursesUI.hh
koUser_CPPFLAGS = -I$(top_srcdir)/src/common -Wall -g -DLINUX -fPIC
koUser_LDFLAGS = -L$(top_srcdir)/src/common/.libs -lkodiaq -lmenu -lform -lncurses -ltinfo
