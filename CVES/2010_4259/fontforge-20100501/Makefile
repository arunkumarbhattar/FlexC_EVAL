prefix = /home/arun/Desktop/CVES/2010_4259/fontforge-20100501/install
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
mandir = ${prefix}/share/man
includedir = ${prefix}/include
sharedir = $(prefix)/share/fontforge
pixmapsdir = $(sharedir)/pixmaps
docdir = $(prefix)/share/doc/fontforge
localedir = $(prefix)/share/locale
pythondir = $(sharedir)/python

srcdir = .
top_builddir = .

LIBTOOL = $(SHELL) $(top_builddir)/libtool
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

all: fontforge

libs: libgunicode libgutils libgdraw

fontforge: libgunicode libgutils libgdraw pos
	( cd fontforge ; $(MAKE) )

libgunicode:
	( cd Unicode ; $(MAKE) )

libgutils: libgunicode
	( cd gutils ; $(MAKE) )

libgdraw: libgutils libgunicode
	( cd gdraw ; $(MAKE) )

pos:
	-ln -s $(srcdir)/po/*.po po
	( cd po ; $(MAKE) )

fontforgegtk: fontforge
	( cd gtkui ; $(MAKE) )

clean:
	( cd fontforge ; $(MAKE) clean )
	( cd Unicode ; $(MAKE) clean )
	( cd gutils ; $(MAKE) clean )
	( cd gdraw ; $(MAKE) clean )
	( cd plugins ; $(MAKE) clean )
	if test -d gtkui; then cd gtkui ; $(MAKE) clean ; fi
	-( cd po ; rm *.mo )
	-( cd pyhook ; for dir in * ; do if test -d $$dir -a "$$dir" != "CVS" ; then  rm -rf $$dir ; fi; done )
	-rm -rf .libs

distclean cleaner:
	( cd fontforge ; $(MAKE) distclean )
	( cd Unicode ; $(MAKE) distclean )
	( cd gutils ; $(MAKE) distclean )
	( cd gdraw ; $(MAKE) distclean )
	( cd plugins ; $(MAKE) distclean )
	if test -d gtkui; then cd gtkui ; $(MAKE) distclean ; fi
	-( cd po ; rm *.mo Makefile* )
	-( cd pyhook ; rm setup.py ; for dir in * ; do if test -d $$dir -a "$$dir" != "CVS" ; then  rm -rf $$dir ; fi; done)
	-rm -rf .libs
	-rm Makefile config.cache config.log config.status libtool fontforge.pc systemspecific inc/config.h

install_libs: libs
# You may need to be root to do the install
	mkdir -p $(DESTDIR)$(libdir)
	#-rm $(DESTDIR)$(libdir)/libgdraw.so* $(DESTDIR)$(libdir)/libgunicode.so.*
	$(LIBTOOL) --mode=install $(INSTALL) -c libgunicode.la $(DESTDIR)$(libdir)/libgunicode.la
	$(LIBTOOL) --mode=install $(INSTALL) -c libgutils.la $(DESTDIR)$(libdir)/libgutils.la
	$(LIBTOOL) --mode=install $(INSTALL) -c libgioftp.la $(DESTDIR)$(libdir)/libgioftp.la
#We don't build gdraw if no X
	if test -r libgdraw.la ; then $(LIBTOOL) --mode=install $(INSTALL) -c libgdraw.la $(DESTDIR)$(libdir)/libgdraw.la ; fi
#We don't always build libfontforge
	if test -r libfontforge.la ; then $(LIBTOOL) --mode=install $(INSTALL) -c libfontforge.la $(DESTDIR)$(libdir)/libfontforge.la ; fi
	$(LIBTOOL) --mode=finish $(DESTDIR)$(libdir)
	mkdir -p $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL_DATA) fontforge.pc $(DESTDIR)$(libdir)/pkgconfig

install_incs:
	mkdir -p $(DESTDIR)$(includedir)/fontforge
	$(INSTALL_DATA) $(srcdir)/inc/*.h $(DESTDIR)$(includedir)/fontforge
#	$(INSTALL_DATA) `grep -L GGadget $(srcdir)/fontforge/*.h` $(DESTDIR)$(includedir)/fontforge
	( for file in $(srcdir)/fontforge/*.h ; do if grep GGadget $$file >/dev/null 2>&1 ; then echo pointless >/dev/null ; else $(INSTALL_DATA) $$file $(DESTDIR)$(includedir)/fontforge ; fi ; done )

install_docs:
	mkdir -p $(DESTDIR)$(docdir)
	mkdir -p $(DESTDIR)$(docdir)/flags $(DESTDIR)$(docdir)/ja
#explicit use of sh to make sure we don't invoke csh with different "if" syntax
	-sh -c "if test -d $(srcdir)/htdocs/ja >/dev/null ; then mkdir -p $(DESTDIR)$(docdir)/ja ; fi"
	-$(INSTALL_DATA) $(srcdir)/htdocs/*.{html,png,gif,pdf} $(DESTDIR)$(docdir)
	-$(INSTALL_DATA) $(srcdir)/htdocs/flags/* $(DESTDIR)$(docdir)/flags
	-$(INSTALL_DATA) $(srcdir)/htdocs/ja/* $(DESTDIR)$(docdir)/ja

install_po:
	( cd po ; $(MAKE) install )

install_pycontrib:
	mkdir -p $(DESTDIR)$(pythondir)
	( cd pycontrib ; $(INSTALL_DATA) *.py $(DESTDIR)$(pythondir) )

install_prog:
	mkdir -p $(DESTDIR)$(bindir)
	mkdir -p $(DESTDIR)$(mandir)/man1
	mkdir -p $(DESTDIR)$(sharedir)
	mkdir -p $(DESTDIR)$(pixmapsdir)
	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/fontforge $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/sfddiff $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/fontimage $(DESTDIR)$(bindir)
	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/fontlint $(DESTDIR)$(bindir)
#	-rm -f $(DESTDIR)$(bindir)/pfaedit
#	(cd $(DESTDIR)$(bindir) ; ln -s fontforge pfaedit)
#	$(INSTALL_DATA) fontforge/pfaedit-*.ui $(DESTDIR)$(sharedir)
#The next line will fail for most installs. If you have downloaded the cidmaps
# this will install them. If you haven't it won't. You only need the cidmaps
# if you are editing CID keyed files (and even then you can live without them
	-$(INSTALL_DATA) $(srcdir)/cidmap/*.cidmap $(DESTDIR)$(sharedir) >/dev/null 2>&1
	-$(INSTALL_DATA) $(srcdir)/fontforge/pixmaps/*.png $(DESTDIR)$(pixmapsdir)
	$(INSTALL_DATA) $(srcdir)/fontforge/fontforge.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/fontforge/sfddiff.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/fontforge/fontimage.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/fontforge/fontlint.1 $(DESTDIR)$(mandir)/man1

install: $(DESTDIR)$(sharedir) all install_libs install_incs install_po install_prog install_pycontrib
# You may need to be root to do the install
#explicit use of sh to make sure we don't invoke csh with different "if" syntax
	sh -c "if test -d $(srcdir)/htdocs >/dev/null ; then $(MAKE) install_docs ; fi"
	sh -c "if test -d fontforge/FontForge.app >/dev/null ; then rm -rf /Applications/FontForge.app ; mv fontforge/FontForge.app /Applications ; ln -s /usr/local/bin/fontforge /Applications/FontForge.app/Contents/MacOS/FontForge ; fi"
	sh < systemspecific
#we don't always create a setup.py
	-(cd pyhook ; if test -r setup.py ; then python setup.py install --prefix=$(prefix) --root=$(DESTDIR); for dir in * ; do if test -d $$dir -a "$$dir" != "CVS" ; then  rm -rf $$dir ; fi; done ; fi )

install_py: $(DESTDIR)$(sharedir) all install_libs install_incs install_po install_prog install_pycontrib
# You may need to be root to do the install
#explicit use of sh to make sure we don't invoke csh with different "if" syntax
	sh -c "if test -d $(srcdir)/htdocs >/dev/null ; then $(MAKE) install_docs ; for dir in * ; do if test -d $$dir ; then  rm -rf $$dir ; fi; done ; fi"
	sh -c "if test -d fontforge/FontForge.app >/dev/null ; then rm -rf /Applications/FontForge.app ; mv fontforge/FontForge.app /Applications ; ln -s /usr/local/bin/fontforge /Applications/FontForge.app/Contents/MacOS/FontForge ; fi"
	sh < systemspecific
	-(cd pyhook ; python setup.py bdist --formats=gztar ; cp dist/fontforge-*.tar.gz $(DESTDIR)/tmp; for dir in * ; do if test -d $$dir -a "$$dir" != "CVS" ; then  rm -rf $$dir ; fi; done )

install_gtk: fontforgegtk
	(cd gtkui ; $(MAKE) install )

uninstall:
	-rm $(DESTDIR)$(bindir)/fontforge
	-rm $(DESTDIR)$(bindir)/sfddiff
	-rm $(DESTDIR)$(bindir)/fontimage
	-rm $(DESTDIR)$(bindir)/fontlint
	-rm $(DESTDIR)$(sharedir)/pfaedit-*.ui
	-rm $(DESTDIR)$(localedir)/*/LC_MESSAGES/FontForge.mo
	-rm $(DESTDIR)$(mandir)/man1/fontimage.1
	-rm $(DESTDIR)$(mandir)/man1/fontforge.1
	-rm $(DESTDIR)$(mandir)/man1/sfddiff.1
	-rm $(DESTDIR)$(mandir)/man1/fontlint.1
	-rm $(DESTDIR)$(libdir)/libgdraw.*
	-rm $(DESTDIR)$(libdir)/libgutils.*
	-rm $(DESTDIR)$(libdir)/libgioftp.*
	-rm $(DESTDIR)$(libdir)/libgunicode.*
	-rm $(DESTDIR)$(libdir)/libfontforge.*
	-rm $(DESTDIR)$(sharedir)/*.cidmap
	-rm $(DESTDIR)$(includedir)/fontforge/*.h
	-rm -rf $(DESTDIR)$(docdir)/*
	-rm $(DESTDIR)$(libdir)/pkgconfig/fontforge.pc
	-rm $(DESTDIR)$(pythondir)/*

$(DESTDIR)$(sharedir):
	mkdir -p $(DESTDIR)$(sharedir)
