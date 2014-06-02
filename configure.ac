AC_INIT(kodiaq, [0.9], [daniel.coderre@lhep.unibe.ch])
AC_MSG_NOTICE([Thanks for your interest in installing kodiaq. Let's get started.])

AC_CONFIG_SRCDIR(src/common)
AC_CONFIG_AUX_DIR(build/build-aux)
AC_CONFIG_MACRO_DIR(build/m4)
AC_CONFIG_HEADERS([build/config.h])

AM_INIT_AUTOMAKE([1.10 -Wall -Wno-portability foreign])

AC_PROG_LIBTOOL

AC_PROG_MAKE_SET
AC_PROG_CXX
AC_LANG(C++)

AC_ENABLE_SHARED
AC_DISABLE_STATIC
LT_INIT

############################
# Compile UI
############################
AC_ARG_ENABLE( [user],
    AS_HELP_STRING([--disable-user], [Disable compilation of UI]))
AS_IF([test "x$enable_user" != "xno"], [
    AM_CONDITIONAL(WITH_USER, true)
    AC_DEFINE(WITH_USER,[],[Compile user interface])
    AC_CHECK_LIB([ncurses],initscr,[],AC_MSG_ERROR)
    AC_CHECK_LIB([menu],menu_init,[],AC_MSG_ERROR)
    AC_CHECK_LIB([form],main,[],AC_MSG_ERROR)
    AC_CHECK_LIB([tinfo],setupterm,[],AC_MSG_ERROR)
    ], [
    AM_CONDITIONAL(WITH_USER,false)
    ])

###########################
# Compile Master Module
###########################
AC_ARG_ENABLE( [master],
    AS_HELP_STRING([--enable-master], [Enable compilation of master executable]))
AS_IF([test "x$enable_master" = "xyes"], [
    AM_CONDITIONAL(WITH_MASTER,true)
    AC_DEFINE(WITH_MASTER,[],[Compile master module])    
    AC_CHECK_LIB(pthread,pthread_mutex_init,[],AC_MSG_ERROR([libpthread missing.]))
    AC_CHECK_LIB(boost_system,main,[],AC_MSG_ERROR([libboost_system missing.]))
    AC_CHECK_LIB(boost_program_options,main,[],AC_MSG_ERROR([libboost_program_options missing.]))
    AC_CHECK_LIB(boost_filesystem,main,[],AC_MSG_ERROR([libboost_filesystem missing.]))
    AC_CHECK_LIB(boost_thread,main,[],AC_MSG_ERROR([libboost_thread missing.]))
    AC_CHECK_LIB(mongoclient,main,[],AC_MSG_ERROR([libmongoclient missing.]))
    ], [
    AM_CONDITIONAL(WITH_MASTER,false)
    ])    
    
###########################
# Compile Slave Module
###########################
AC_ARG_ENABLE( [slave],
    AS_HELP_STRING([--enable-slave], [Enable slave module]))
AS_IF([test "x$enable_slave" = "xyes"], [
    AM_CONDITIONAL(WITH_SLAVE,true)
    AC_DEFINE(WITH_SLAVE,[],[Compile slave module])
    
    AC_CHECK_LIB(pthread,pthread_mutex_init,[],AC_MSG_ERROR([libpthread missing.]))
    AC_CHECK_LIB(CAENVME,CAENVME_Init,[],AC_MSG_ERROR([libCAENVME missing.]))
    AC_CHECK_LIB(snappy,_ZN6snappy11RawCompressEPKcmPcPm,[],AC_MSG_ERROR([libsnappy missing.]))
    AC_CHECK_LIB(boost_system,main,[],AC_MSG_ERROR([libboost_system missing.]))
    AC_CHECK_LIB(boost_program_options,main,[],AC_MSG_ERROR([libboost_program_options missing.]))
    AC_CHECK_LIB(boost_filesystem,main,[],AC_MSG_ERROR([libboost_filesystem missing.]))
    AC_CHECK_LIB(boost_thread,main,[],AC_MSG_ERROR([libboost_thread missing.]))

    ], [
    AM_CONDITIONAL(WITH_SLAVE,false)
    ])

###########################
# Compile lite module
###########################
AC_ARG_ENABLE( [lite],
    AS_HELP_STRING([--enable-lite], [Compile standalone slave]))
AS_IF([test "x$enable_lite" = "xyes"], [
    AM_CONDITIONAL(KODIAQ_LITE,true)
    AM_CONDITIONAL(WITH_SLAVE,true)
    AC_DEFINE([KLITE],[1],[Compile standalone slave])    
    AC_CHECK_LIB(pthread,pthread_mutex_init,[],AC_MSG_ERROR([libpthread missing.]))
    AC_CHECK_LIB(CAENVME,CAENVME_Init,[],AC_MSG_ERROR([libCAENVME missing.]))
    AC_CHECK_LIB(snappy,_ZN6snappy11RawCompressEPKcmPcPm,[],AC_MSG_ERROR([libsnappymissing.]))
    AC_CHECK_LIB([ncurses],initscr,[],AC_MSG_ERROR)   
#    AC_CHECK_LIB(protobuf,main,[],AC_MSG_ERROR([libprotobuf missing]))
    ], [
    AM_CONDITIONAL(KODIAQ_LITE,false)
    ])

###################################
# Compile DDC10 Module
###################################
#Now some more specific options. For example, should we compile with DDC10 support?
AC_ARG_ENABLE( [DDC10],
    AS_HELP_STRING([--enable-ddc10], [Enable support for DDC10 HE veto module]))
AS_IF([test "x$enable_ddc10" = "xyes"], [
    AM_CONDITIONAL(WITH_DDC10, true)
    AC_DEFINE(WITH_DDC10,[],[Compile with DDC HE veto support])    
    AC_CHECK_LIB([tcl8.5],main,[],AC_MSG_ERROR)
    AC_CHECK_LIB([expect],exp_popen,[],AC_MSG_ERROR)
    ], [
    AM_CONDITIONAL(WITH_DDC10,false)
    ])

###################################
# Compile all modules
###################################
AC_ARG_ENABLE( [all],
    AS_HELP_STRING([--enable-all], [Enable compilation of all modules]))
AS_IF([test "x$enable_all" = "xyes"], [
    AM_CONDITIONAL(WITH_SLAVE,true)
    AM_CONDITIONAL(WITH_MASTER,true)
    AM_CONDITIONAL(WITH_USER,true)
    AM_CONDITIONAL(WITH_DDC10,true)
    AC_DEFINE(WITH_MASTER,[],[Compile master module])
    AC_DEFINE(WITH_SLAVE,[],[Compile slave module])
    AC_DEFINE(WITH_DDC10,[],[Compile with DDC HE veto support])
    AC_DEFINE(WITH_USER,[],[Compile user interface])   
    AC_CHECK_LIB([tcl8.5],main,[],AC_MSG_ERROR([libtcl8.5 not found]))
    AC_CHECK_LIB([expect],exp_popen,[],AC_MSG_ERROR([libexpect not found]))
    AC_CHECK_LIB(pthread,pthread_mutex_init,[],AC_MSG_ERROR([libpthread missing.]))
    AC_CHECK_LIB(CAENVME,CAENVME_Init,[],AC_MSG_ERROR([libCAENVME missing.]))
    AC_CHECK_LIB(snappy,_ZN6snappy11RawCompressEPKcmPcPm,[],AC_MSG_ERROR([libsnappy missing.]))
    AC_CHECK_LIB(boost_system,main,[],AC_MSG_ERROR([libboost_system missing.]))
    AC_CHECK_LIB(boost_program_options,main,[],AC_MSG_ERROR([libboost_program_options missing.]))
    AC_CHECK_LIB(boost_filesystem,main,[],AC_MSG_ERROR([libboost_filesystem missing.]))
    AC_CHECK_LIB(boost_thread,main,[],AC_MSG_ERROR([libboost_thread missing.]))
    AC_CHECK_LIB(mongoclient,main,[],AC_MSG_ERROR([libmongoclient missing.]))

#    AC_CHECK_LIB(protobuf,main,[],AC_MSG_ERROR([libprotobuf missing]))    
    ], [
    ])

#Check for mongodb support and compile with it if we have it
AC_CHECK_LIB(boost_system,main,[],[])
AC_CHECK_LIB(boost_program_options,main,[],[])
AC_CHECK_LIB(boost_filesystem,main,[],[])
AC_CHECK_LIB(boost_thread,main,[],[])

AC_CHECK_LIB(mongoclient,main)
AS_IF([test "$ac_cv_lib_mongoclient_main" = yes], [HAVE_LIBMONGOCLIENT=1],[HAVE_LIBMONGOCLIENT=0]) 
AM_CONDITIONAL([WITH_MONGODB],[test "$ac_cv_lib_mongoclient_main" = yes])

#Check for libpbf (output file format) support and compile with it if it's there
AC_CHECK_LIB(protobuf,main)
AS_IF([test "$ac_cv_lib_protobuf_main" = yes], [
     AC_CHECK_LIB(pbf,main)
     AS_IF([test "$ac_cv_lib_pbf_main" = yes], [HAVE_LIBPBF=1],[HAVE_LIBPBF=0])
     ], [
     ])
AM_CONDITIONAL([WITH_LIBPBF],[test "$ac_cv_lib_pbf_main" = yes])

#AC_CONFIG_FILES([Makefile])
AC_OUTPUT(Makefile src/ddc10/Makefile src/common/Makefile src/master/Makefile src/slave/Makefile src/user/Makefile)

AS_IF([test "$ac_cv_lib_mongoclient_main" = yes], [AC_MSG_NOTICE([Compiling WITH mongodb support])],[AC_MSG_NOTICE([Compiling WITHOUT mongodb support])])
AS_IF([test "$ac_cv_lib_pbf_main" = yes], [AC_MSG_NOTICE([Compiling WITH file output support])],[AC_MSG_NOTICE([Compiling WITHOUT file output support])])

