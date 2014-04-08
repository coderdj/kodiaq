ACLOCAL_AMFLAGS = -I build/m4

SUBDIRS = 
if WITH_DDC10
SUBDIRS += src/ddc10
endif

#else
#SUBDIRS = 
#endif
SUBDIRS += src/common 

if WITH_USER
SUBDIRS += src/user
endif
if WITH_MASTER
SUBDIRS += src/master
endif
if WITH_SLAVE
SUBDIRS += src/slave
endif

EXTRA_DIST = autogen.sh