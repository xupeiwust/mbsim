dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.57])
AC_INIT([nurbs++],[3.0.13],[martin.o.foerg@googlemail.com],[nurbs++])
AC_CONFIG_HEADER(include/plib_config.h)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([nurbs/nurbs.h])
LT_INIT([win32-dll])
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_LANG([C++])
AC_C_CONST

AC_CONFIG_MACRO_DIR([m4])

PLIB_INSIDE_MINDSEYE

dnl Add support for the `--enable-shared' and `--disable-shared' configure flags. Invoke ltconfig with the correct arguments to configure the package
AM_PROG_LIBTOOL

dnl the user can use --enable-shared to enable this
dnl AM_DISABLE_SHARED

AC_PROG_MAKE_SET

dnl 
dnl Adding exception handling support
dnl 
AC_ARG_ENABLE(exception,
  AS_HELP_STRING([--enable-exception],
    [use exception handling instead of the Error class [default=yes]]),
if test "$enable_exception" != no; then
  AC_DEFINE([USE_EXCEPTION],[1],[Use exeptions])
fi, 
AC_DEFINE([USE_EXCEPTION],[1],[Use exeptions])
)

dnl 
dnl Adding exception handling support
dnl 
AC_ARG_ENABLE(verbose_exception,
  AS_HELP_STRING([--enable-verbose-exception],
    [will print an error message when an exception is thrown [default=no]]),
  if test "$enable_verbose_exception" != no; then
  AC_DEFINE([USE_EXCEPTION],[1],[Use execptions])
  AC_DEFINE([VERBOSE_EXCEPTION],[1],[Verbose exeptions])
  fi
)

dnl
dnl Adding debug information support
dnl

AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug],
     [adds range checking and other debugging information to the library [default=yes]]),
  if test "$enable_debug" != no; then
  AC_DEFINE([DEBUG_PLIB],[1],[Debugging functions like range validity checking and others])
  fi,AC_DEFINE([DEBUG_PLIB],[1],[Debugging functions like range validity checking and others])
)

dnl 
dnl Create a library or not ?
dnl

include_source=no

AC_ARG_ENABLE(library,
  AS_HELP_STRING([--enable-library],
    [it generates explicitely instantiate the templates. This creates a library. If disabled, it will include the source files with the header files and the compiler will instantiate the templates as it needs them [default=yes]]),
  if test "$enable_library" != no; then
    include_source=no
  else
    include_source=yes
  fi,

  include_source=no
)

dnl
dnl Setting the flags for different compilers to the proper settings
dnl

dnl clean_list="" 

if test "$CXX" = CC; then
case "$target_os" in 
solaris*) 
    CXXFLAGS=$CXXFLAGS" -pta"
    complex_lib="-lcomplex"
    clean_list="-r Templates.DB"
;;
irix*) 
    CXXFLAGS=$CXXFLAGS" -pta -n32 -64"
    complex_lib="-lcomplex"
    clean_list="-r Templates.DB ii_files"
;;
esac
else
if test "$GXX" = yes; then
 if test "$include_source" = yes; then
 AC_DEFINE([INCLUDE_TEMPLATE_SOURCE],[1],[Include template source])
 CXXFLAGS=$CXXFLAGS" -frepo"
 else
 AC_DEFINE([NO_IMPLICIT_TEMPLATES],[1],[Do not use implicit templates])
 CXXFLAGS=$CXXFLAGS" -fno-implicit-templates"
 fi
 case "$target_os" in
 solaris*)
    AC_DEFINE([USING_GNU_SOLARIS],[1],[Use GNU Solaris])
 ;;
 linux*)
    AC_DEFINE([USING_LINUX],[1],[Use Linux])
 ;;
 alpha*)
    AC_DEFINE([USING_GNU_DECALPHA],[1],[Use GNU DEC-ALPHA])
 ;; 
 esac   
fi
fi

dnl 
dnl Optional library support
dnl

option_libs=
option_include=

dnl
dnl Addin CppUnit Support
dnl

WITH_CPPUNIT

AM_CONDITIONAL([BUILD_CPPUNIT_TESTS],[test x"$has_cppunit" = xyes])

dnl
dnl Adding Image Magick support
dnl

test_magick=yes

AC_ARG_WITH(magick,[  --with-magick[=/magickdir]     
                          supports a limited set of the Image Magick library 
                          functionnality with the IM_ImageT class [default=yes]],
if test "$with_magick" != no; then
	test_magick=yes
else
	test_magick=no
fi)



dnl has_image_magick=no
dnl if test "$test_magick" = yes; then
	dnl WITH_IMAGE_MAGICK
dnl fi


dnl if test "$has_image_magick" = yes; then
dnl 	option_libs=' $(MAGICK_LDFLAGS) $(MAGICK_LIBS)'
dnl 	option_include=' $(MAGICK_CPPFLAGS)'
dnl	AC_DEFINE(WITH_IMAGE_MAGICK)
dnl 
	
dnl
dnl setting the order to be column order
dnl

user_defined_column_order=no

AC_ARG_ENABLE(column-order,
 AS_HELP_STRING([--enable-column-order], 
   [defines if the matrices are in row or column ordering [default=no]. This is always true if your are also compiling for OpenGL]),
  [if test "$enable_column-order" != no; then 
  AC_DEFINE([COLUMN_ORDER],[1],[Storing matrices in column ordering])
  user_defined_column_order=yes
  fi],
  AC_DEFINE([COLUMN_ORDER],[0],[Storing matrices in column ordering])
)

dnl
dnl Adding OpenGL support
dnl

#AC_PATH_GL

if test "$use_opengl" = yes; then
AC_DEFINE([WITH_OPENGL],[1],[Compile with OpenGL])
AC_DEFINE([COLUMN_ORDER],[1],[Storing matrices in column ordering])
option_libs=$option_libs' $(GL_LIBS)'
option_include=$option_include' $(GL_CFLAGS)'
fi

AC_ARG_WITH(sunopengl,
  AS_HELP_STRING([--with-sunopengl],
    [Sun's version of OpenGL has some particularities...]),
if test "$with_sunopengl" != no; then
CXXFLAGS=$CXXFLAGS" -DSUNOPENGL"
fi
)

dnl 
dnl adjusting the option information
dnl

if test "$option_libs" != ''; then
echo 'option is defined'
option_libs='$(XLIBS)'$option_libs
option_include=$option_include' $(XINCLUDE)'
fi


dnl
dnl checking for the ways of your C++ compiler...
dnl

dnl first it must have namespaces...

CXX_MUST_HAVE_NAMESPACE
CXX_HAVE_TEMPLATE_OF_TEMPLATE
CXX_HAVE_ISO_FRIEND_DECL
CXX_HAVE_COMPLEX_FCNS

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

complex_header="complex.h"
use_iso_complex=no
AC_MSG_CHECKING(if the compiler can see complex instead of complex.h)
AC_TRY_COMPILE([#include <complex>],[return 1; ],
	[use_iso_complex=yes ; ])

if test "$use_iso_complex" = yes; then
AC_MSG_RESULT(yes)
else
AC_MSG_RESULT(no)
dnl echo "\nI am sorry. This library requires an ISO C++ compiler. The reason is"
dnl echo "the difference between the new standards and old implementations"
dnl echo "are big enough to become a maintenance problem. Even more so, when"
dnl echo "you take into account that an ISO C++ compiler should be available"
dnl echo "very soon now (tm) by your vendor. Or you can use a GNU alternative"
dnl echo "such as EGCS or GCC."
dnl exit 1
echo 
echo "The ISO C++ standard requires the presence of <complex>."
echo "Your system doesn't seem to have it, you might not be able to"
echo "use NURBS++. Cross your fingers."
fi


AC_LANG_RESTORE


dnl The matrix libs and includes

packagedir=`pwd`

matrix_include='-I$(top_srcdir)/matrix -I$(top_srcdir)/image -I$(top_srcdir)/numerical'
matrix_lib_flag='-L$(top_srcdir)/matrix -L$(top_srcdir)/image -L$(top_srcdir)/numerical'
dnl matrix_lib='$(packagedir)/numerical/libmatrixN.la $(packagedir)/image/libmatrixI.la $(packagedir)/matrix/libmatrix.la '
matrix_lib='-lmatrixN -lmatrixI -lmatrix'

dnl the NURBS libs and includes

dnl
dnl creating a double floating point library
dnl


AC_ARG_ENABLE(double,
  AS_HELP_STRING([--enable-double],
    [creates a double floating point library [default=yes]]),
if test "$enable_double" != no; then 
build_double=yes
else
build_double=no
fi,build_double=yes)

dnl
dnl setting the libraries to build
dnl

nurbs_include='-I$(top_srcdir)/nurbs'
nurbs_libd='$(packagedir)/nurbs/libnurbsd.la'
nurbs_lib="$nurbs_libf $nurbs_libd"

dnl
dnl Making the changes in the proper files
dnl

AC_DEFINE(PACKAGE)
AC_DEFINE(VERSION)
AC_SUBST(VERSION)

AC_SUBST(packagedir)
AC_SUBST(nurbs_libf) 
AC_SUBST(nurbs_libd) 
AC_SUBST(libld)
AC_SUBST(libld_options)
AC_SUBST(libcxx_flags)
AC_SUBST(MAGICK_CPPFLAGS)
AC_SUBST(MAGICK_LDFLAGS)
AC_SUBST(MAGICK_LIBS)
AC_SUBST(complex_lib)
AC_SUBST(matrix_lib)
AC_SUBST(matrix_lib_flag)
AC_SUBST(ar)
AC_SUBST(clean_list)
AC_SUBST(complex_header)
AC_SUBST(matrix_include)
AC_SUBST(nurbs_libs)
AC_SUBST(nurbs_include)
AC_SUBST(nurbs_lib)
AC_SUBST(nurbs_demo_lib)
AC_SUBST(libs_inc)
AC_SUBST(option_libs)
AC_SUBST(option_include)
AC_SUBST(CPPUNIT_CPPFLAGS)
AC_SUBST(CPPUNIT_LIBS)
AC_SUBST([plib_compilerflags])

PL_PROG_PERL

LDFLAGS="$LDFLAGS $LDFLAGS_LIBTOOL -no-undefined -Wl,--no-undefined"
test "_$host_os" != "_mingw32" && LDFLAGS="$LDFLAGS -Wl,--disable-new-dtags,-rpath,\\\$\$ORIGIN/../lib"
CXXFLAGS="$CXXFLAGS -fpermissive"

dnl removed from list: nurbs++-config (and added nurbs++.pc to use pkg-config)

AC_CONFIG_FILES([Makefile
  Doxyfile
  include/Makefile
  m4/Makefile
  matrix/Makefile
  numerical/Makefile
  image/Makefile
  nurbs/Makefile
  examples/Makefile
  examples/matrix/Makefile
  examples/image/Makefile
  examples/numerical/Makefile
  examples/nurbs/Makefile
  tests/Makefile
  tests/matrix/Makefile
  tests/nurbs/Makefile
  nurbs++.pc])

hardcode_into_libs=no # do not add hardcoded libdirs to ltlibraries
hardcode_libdir_flag_spec_CXX= # do not add hardcodeed libdirs to ltbinaries
AC_OUTPUT

echo
echo 'The package has now been configured. Type '
echo '	make '
echo '	make install'
echo 
echo 'To generate and install the NURBS++ package.'
echo
echo 'The library will be installed in ' $libdir
echo 'The header files will be installed in '$includedir'/nurbs++'
echo 
echo 

