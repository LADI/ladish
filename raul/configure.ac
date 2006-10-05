AC_PREREQ(2.59)
AC_INIT(raul, 0.0.0, [drobilla@connect.carleton.ca])
#AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([raul/Mutex.h])
AM_INIT_AUTOMAKE

AC_LANG([C++])

#AC_PROG_CXX
AC_TYPE_SIZE_T
#AC_PROG_LIBTOOL

AC_HEADER_STDC
AC_TYPE_INT32_T
AC_TYPE_SIZE_T

AC_FUNC_MALLOC
AC_CHECK_FUNCS([strdup strerror])

AC_CHECK_HEADER([pthread.h], [],
	AC_MSG_ERROR([Raul requires POSIX threads.]))

build_smart_pointers="yes"
AC_ARG_ENABLE(smart_pointers,
	[AS_HELP_STRING(--enable-smart_pointers, [Include smart pointers - requires boost (yes)])],
	[ if test x$enable_smart_pointers = xno ; then build_smart_pointers=no ; fi ])
if test "$build_smart_pointers" = "yes"; then
	AC_CHECK_HEADER([boost/shared_ptr.hpp], [],  
		AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))

	AC_CHECK_HEADER([boost/weak_ptr.hpp], [],  
		AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))

	AC_SUBST(BOOST_CFLAGS)
	AC_SUBST(BOOST_LIBS)
fi
AM_CONDITIONAL(WITH_BOOST, [test "$build_smart_pointers" = "yes"])


build_raptor="yes"
AC_ARG_ENABLE(raptor,
	[AS_HELP_STRING(--enable-raptor, [Include Raptor (RDF) serialization support (yes)])],
	[ if test x$enable_raptor = xno ; then build_raptor=no ; fi ])
if test "$build_raptor" = "yes"; then
	PKG_CHECK_MODULES(RAPTOR, raptor >= 1.4.0, [],
		AC_MSG_ERROR([RDF support requires raptor (>= 1.4.0)]))
fi
AM_CONDITIONAL(WITH_RAPTOR, [test "$build_raptor" = "yes"])


build_liblo="yes"
AC_ARG_ENABLE(liblo,
	[AS_HELP_STRING(--enable-liblo, [Include liblo (OSC) support (yes)])],
	[ if test x$enable_liblo = xno ; then build_liblo=no ; fi ])
if test "$build_liblo" = "yes"; then
	PKG_CHECK_MODULES(LIBLO, liblo >= 0.22, [],
		AC_MSG_ERROR([OSC support requires liblo (>= 0.22)]))
fi
AM_CONDITIONAL(WITH_LIBLO, [test "$build_liblo" = "yes"])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([raul/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/reference.doxygen])
AC_CONFIG_FILES([raul.pc])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Raul build configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([OSC supprt:     $build_liblo])
AC_MSG_RESULT([RDF supprt:     $build_raptor])
AC_MSG_RESULT([Smart Pointers: $build_smart_pointers])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

