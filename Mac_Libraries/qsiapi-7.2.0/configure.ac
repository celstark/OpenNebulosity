AC_INIT([qsiapi],[7.2.0],[support@qsimaging.com],[qsiapi], www.qsimaging.com)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([-Wall -Werror])
AC_COPYRIGHT(qsicopyright.txt)
AC_PROG_CXX
AC_LANG([C++])
AM_PROG_AR

dnl check whether the TIFF library is installed, this adds the -ltiff linker 
dnl flag. Furthermore, the presence of the tiffio.h header triggers
dnl inclusion of the tiff code in the programs
AC_CHECK_HEADERS([tiff.h tiffio.h])
AC_CHECK_LIB([tiff],[TIFFOpen])

dnl check whether we should build the demo program. If we do so, then we
dnl also have to check for the presence of the tiff headers and libraries
AC_ARG_ENABLE(demo,
AS_HELP_STRING([--enable-demo],[Compile demo program]),,)
AM_CONDITIONAL(ENABLE_DEMO, test x$enable_demo = xyes)

LT_PREREQ([2.2])
LT_INIT
LT_LANG([C++])

dnl Check for usb stack selection
dnl Default is libftdi (open source).
dnl Use the configure option --enable-libftd2xx for the ftd2xx stack from FTDI corp.
dnl --------------------------------------------------------------------------------
LIBFTD2XX_REQUIRED=1.1.12
LIBFTDI_REQUIRED=0.19

AC_ARG_ENABLE(libftd2xx,
        AS_HELP_STRING([--enable-libftd2xx],[Enable libftd2xx support]),
        [ if test "$enableval" = "yes"; then
                user_enabled_ftd2xx=1
          fi ],
        [ user_enabled_ftd2xx=0 ])

AC_MSG_CHECKING(if libftd2xx support is enabled)
if test "$user_enabled_ftd2xx" = 1; then
        AC_MSG_RESULT(yes)
        enable_ftd2xx="yes"
        FTD2XX_CFLAGS="-I/usr/local/include"
        FTD2XX_LIBS="-L/usr/local/lib -lftd2xx"
        AC_SUBST(FTD2XX_CFLAGS) 
        AC_SUBST(FTD2XX_LIBS)        
else
        AC_MSG_RESULT(no)
        enable_ftd2xx="no"               
fi

dnl---------------------------------------------------------------------------------
AC_ARG_ENABLE(libftdi,
        AS_HELP_STRING([--enable-libftdi],[Enable libftdi support]),
        [ if test "$enableval" = "yes"; then
                user_enabled_ftdi=1
          fi ],
        [ user_enabled_ftdi=0 ])

AC_MSG_CHECKING(if libftdi support is enabled)
if test "$user_enabled_ftdi" = 1; then
        AC_MSG_RESULT(yes)
        enable_ftdi="yes"
else
		if test $enable_ftd2xx = "yes"; then
			AC_MSG_RESULT(no)
			enable_ftdi="no"
		else
			AC_MSG_RESULT(yes)
			enable_ftdi="yes"
		fi    
fi

if test x$enable_ftdi = xyes; then
        FTDI_CFLAGS="-I/usr/local/include"
        FTDI_LIBS="-L/usr/local/lib -lftdi"
        AC_SUBST(FTDI_CFLAGS) 
        AC_SUBST(FTDI_LIBS) 
fi

AM_CONDITIONAL(ENABLE_LIBFTDI, [test x$enable_ftdi = xyes])
AM_CONDITIONAL(ENABLE_LIBFTD2XX, [test x$enable_ftd2xx = xyes])

dnl --------------------------------------------------------------------------------
 
AC_CONFIG_FILES([libqsiapi-config])
AC_CONFIG_COMMANDS([default],[chmod a+x libqsiapi-config],[])
AC_CONFIG_FILES(Makefile doc/Makefile lib/Makefile src/Makefile demo_src/Makefile)

AC_OUTPUT
