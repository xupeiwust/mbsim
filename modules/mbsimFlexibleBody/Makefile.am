SUBDIRS = mbsimFlexibleBody schema

DIST_SUBDIRS = mbsimFlexibleBody schema

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mbsimFlexibleBody.pc

licdir = @datadir@/mbsimFlexibleBody
lic_DATA = COPYING NOTICE

mbsimmodulesdir = $(datadir)/mbsimmodules
mbsimmodules_DATA = mbsimFlexibleBody.mbsimmodule.xml

# wrapper target for doxygen in dir doc
.PHONY: doc clean-doc install-doc uninstall-doc xmldoc clean-xmldoc intall-xmldoc uninstall-xmldoc

doc:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) all)
xmldoc:
	(cd xmldoc && $(MAKE) $(AM_MAKEFLAGS) all)

clean-doc:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) clean)
clean-xmldoc:
	(cd xmldoc && $(MAKE) $(AM_MAKEFLAGS) clean)

install-doc:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) install)
install-xmldoc:
	(cd xmldoc && $(MAKE) $(AM_MAKEFLAGS) install)

uninstall-doc:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) uninstall)
uninstall-xmldoc:
	(cd xmldoc && $(MAKE) $(AM_MAKEFLAGS) uninstall)
