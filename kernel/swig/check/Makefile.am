dummydir = $(abs_builddir)
dummy_LTLIBRARIES = _mymod.la

BUILT_SOURCES = mymod_swig_python.cc

CLEANFILES = mymod_swig_python.cc mymod.py mymod_swig_python.d
# create a empty dummy mymod_swig_python.cc file if MBSIM_SWIG is not set (to avoid a long build time)
.DELETE_ON_ERROR: # swig -Werror requires this
mymod_swig_python.cc: $(srcdir)/mymod.i
	rm -f mymod_swig_python.cc
	test -z "$(MBSIM_SWIG)" && echo "" > $@ || $(swig) -python -py3 -MP -MD -Werror -c++ -o mymod_swig_python.cc -I$(srcdir)/.. $(DEPS_CFLAGS) $(srcdir)/mymod.i
-include $(builddir)/mymod_swig_python.d
mymod.py: mymod_swig_python.cc
	test ! -f $@ && (rm -f mymod_swig_python.cc && $(MAKE) $(AM_MAKEFLAGS) mymod_swig_python.cc) || :

_mymod_la_SOURCES = mymod_swig_python.cc
_mymod_la_LDFLAGS = -module -shrext $(PYTHON_SHEXT) -avoid-version
_mymod_la_CPPFLAGS = $(PYCPPWRAPPER_CFLAGS) -I$(PYTHON_LIBDIR)/site-packages/numpy/core/include $(DEPS_CFLAGS) -D_hypot=hypot # -D_hypot is a MSVS/python bug workaround
_mymod_la_LIBADD = $(PYCPPWRAPPER_LIBS) $(DEPS_LIBS)



TESTS = fmatvec_main.sh
