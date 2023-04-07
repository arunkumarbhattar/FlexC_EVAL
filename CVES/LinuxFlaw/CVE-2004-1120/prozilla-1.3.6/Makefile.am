SUBDIRS         = src man  
DIST_SUBDIRS = $(SUBDIRS) 
DIST_OTHER      = COPYING

# All the rest of the distributed files
EXTRA_DIST =            \
	ANNOUNCE	\
	CREDITS		\
	FAQ		\
	prozilla.spec	\
	prozrc.sample	
	


install:
	 $(INSTALL) -d $(sysconfdir)
	if [ ! -e $(sysconfdir)/prozilla.conf ]; then cp prozrc.sample $(sysconfdir)/prozilla.conf; fi; cp prozrc.sample $(sysconfdir)/prozilla.conf-sample

#Rule to build tar-gzipped distribution package
$(PACKAGE)-$(VERSION).tar.gz: dist

# Rule to build RPM distribution package
rpm: $(PACKAGE)-$(VERSION).tar.gz
	cp $(PACKAGE)-$(VERSION).tar.gz /usr/src/redhat/SOURCES
	rpm -ba prozilla.spec
