prefix = /home/arun/Desktop/CVES/2010_4259/fontforge-20100501/install
exec_prefix = ${prefix}

sharedir = /home/arun/Desktop/CVES/2010_4259/fontforge-20100501/install/share/fontforge
srcdir = .
top_srcdir = ..
top_builddir = ..

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
plugindir = $(sharedir)/plugins

LIBTOOL = $(SHELL) $(top_builddir)/libtool
CC = gcc
O = lo

X_CFLAGS = 
X_LIBS = 
X_11LIB = -lXi -lX11
X_EXTRA_LIBS = 
X_PRE_LIBS =  -lSM -lICE
STATIC_LIBS = 

FF_VERSION=1
FF_REVISION=0
FF_AGE=0

fontforge_LIBOBJECTS = asmfpst.$O autohint.$O autosave.$O autotrace.$O autowidth.$O \
 bezctx_ff.$O bitmapchar.$O bitmapcontrol.$O bvedit.$O clipnoui.$O crctab.$O \
 cvexport.$O cvimages.$O cvundoes.$O dumpbdf.$O dumppfa.$O effects.$O encoding.$O \
 featurefile.$O fontviewbase.$O freetype.$O fvcomposit.$O fvfonts.$O fvimportbdf.$O \
 fvmetrics.$O glyphcomp.$O http.$O ikarus.$O lookups.$O macbinary.$O \
 macenc.$O mathconstants.$O mm.$O namelist.$O nonlineartrans.$O noprefs.$O nouiutil.$O \
 nowakowskittfinstr.$O ofl.$O othersubrs.$O palmfonts.$O parsepdf.$O parsepfa.$O \
 parsettfatt.$O parsettfbmf.$O parsettf.$O parsettfvar.$O plugins.$O print.$O \
 psread.$O pua.$O python.$O savefont.$O scripting.$O scstyles.$O search.$O \
 sfd1.$O sfd.$O sflayout.$O spiro.$O splinechar.$O splinefill.$O \
 splinefont.$O splineorder2.$O splineoverlap.$O splinerefigure.$O \
 splinesaveafm.$O splinesave.$O splinestroke.$O splineutil2.$O splineutil.$O \
 start.$O stemdb.$O svg.$O tottfaat.$O tottfgpos.$O tottf.$O \
 tottfvar.$O ttfinstrs.$O ttfspecial.$O ufo.$O unicoderange.$O utils.$O \
 winfonts.$O zapfnomen.$O groups.$O langfreq.$O ftdelta.$O autowidth2.$O \
 woff.$O
fontforge_UIOBJECTS = alignment.o anchorsaway.o autowidth2dlg.o basedlg.o \
 bdfinfo.o bitmapdlg.o bitmapview.o charinfo.o charview.o clipui.o \
 combinations.o contextchain.o cursors.o cvaddpoints.o cvdebug.o cvdgloss.o \
 cvexportdlg.o cvfreehand.o cvgetinfo.o cvgridfit.o cvhand.o cvhints.o \
 cvimportdlg.o cvknife.o cvpalettes.o cvpointer.o cvruler.o cvshapes.o \
 cvstroke.o cvtranstools.o displayfonts.o effectsui.o encodingui.o \
 fontinfo.o fontview.o freetypeui.o fvfontsdlg.o fvmetricsdlg.o \
 gotodlg.o \
 groupsdlg.o histograms.o images.o kernclass.o layer2layer.o lookupui.o \
 macencui.o math.o metricsview.o mmdlg.o nonlineartransui.o oflib.o \
 openfontdlg.o prefs.o problems.o pythonui.o savefontdlg.o scriptingdlg.o \
 scstylesui.o searchview.o sftextfield.o showatt.o simplifydlg.o splashimage.o \
 stamp.o startui.o statemachine.o tilepath.o transform.o ttfinstrsui.o \
 uiutil.o windowmenu.o justifydlg.o deltaui.o
fontforge_SCRIPTOBJ = startnoui.o stamp.o

DIFFOBJS = sfddiff.o stamp.o
ACORNOBJS = acorn2sfd.o stamp.o

_CFLAGS = -I$(top_srcdir)/inc -I../inc -I$(srcdir) -I. -Wmissing-prototypes -Wunused -Wimplicit -Wreturn-type -Wparentheses -Wformat -Wchar-subscripts $(X_CFLAGS) \
    -DHAVE_CONFIG_H '-DSHAREDIR="$(sharedir)"' -DLIBDIR='"$(libdir)"' \
     '-DPREFIX="/home/arun/Desktop/CVES/2010_4259/fontforge-20100501/install"'
CFLAGS = -g -O2 -I/usr/include/freetype2/ -I/usr/include/libxml2/ -I/usr/include/cairo -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16 -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/harfbuzz -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16  $(_CFLAGS)

#I used to include -L/usr/lib explicitly after static_libs. Not sure why. Should be implied & breaks 64bit compiles
LDFLAGS = $(X_LIBS)  -L/usr/local/lib
LIBS = -rpath $(libdir) ../libgdraw.la ../libgutils.la \
    ../libgunicode.la \
    $(X_PRE_LIBS) $(X_11LIB) $(X_EXTRA_LIBS)  -lpthread $(STATIC_LIBS) -lm
LIBLIBS = -rpath $(libdir) ../libgutils.la ../libgunicode.la \
     -lpthread $(STATIC_LIBS) -lm
DLIBS = -rpath $(libdir)  ../libfontforge.la ../libgutils.la ../libgunicode.la  -lpthread $(STATIC_LIBS) -lm

all: ../libfontforge.la  sfddiff fontimage fontlint

fontforge: $(fontforge_UIOBJECTS) $(fontforge_LIBOBJECTS) exelibstamp.pre libstamp.pre
	sed -e '1,$$ s/REPLACE_ME_WITH_MAJOR_VERSION/$(FF_VERSION)/' -e '1,$$ s/REPLACE_ME_WITH_MINOR_VERSION/$(FF_REVISION)/' < exelibstamp.pre > exelibstamp.c
	$(CC) -c $(CFLAGS) exelibstamp.c
	sed -e '1,$$ s/REPLACE_ME_WITH_MAJOR_VERSION/$(FF_VERSION)/' -e '1,$$ s/REPLACE_ME_WITH_MINOR_VERSION/$(FF_REVISION)/' < libstamp.pre > libstamp.c
	$(CC) -c $(CFLAGS) libstamp.c
	$(LIBTOOL) --mode=link $(CC) -export-dynamic $(LDFLAGS) -o fontforge \
	    $(fontforge_UIOBJECTS) $(fontforge_LIBOBJECTS) exelibstamp.o libstamp.o $(LIBS)

../libfontforge.la: $(fontforge_UIOBJECTS)  $(fontforge_LIBOBJECTS) exelibstamp.pre libstamp.pre
	sed -e '1,$$ s/REPLACE_ME_WITH_MAJOR_VERSION/$(FF_VERSION)/' -e '1,$$ s/REPLACE_ME_WITH_MINOR_VERSION/$(FF_REVISION)/' < libstamp.pre > libstamp.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c libstamp.c
	$(LIBTOOL) --mode=link $(CC) -no-undefined $(CFLAGS) $(LDFLAGS) \
	    -o ../libfontforge.la $(fontforge_LIBOBJECTS) libstamp.lo $(LIBLIBS) \
	    -rpath $(libdir) -version-info $(FF_VERSION):$(FF_REVISION):$(FF_AGE)
	sed -e '1,$$ s/REPLACE_ME_WITH_MAJOR_VERSION/$(FF_VERSION)/' -e '1,$$ s/REPLACE_ME_WITH_MINOR_VERSION/$(FF_REVISION)/' < exelibstamp.pre > exelibstamp.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c exelibstamp.c
	$(LIBTOOL) --mode=link $(CC) $(LDFLAGS) -o fontforge \
	    $(fontforge_UIOBJECTS) exelibstamp.lo ../libfontforge.la $(LIBS)

FontForge.app/Contents/Info.plist: MacFontForgeApp.zip stamp.c
	-rm -rf FontForge.app __MACOSX
	unzip MacFontForgeApp.zip
	cp FontForge.app/Contents/Info.plist /tmp
	( VERSION=`grep source_version_str stamp.c |sed 's/const char \*source_version_str = "//' | sed 's/";//'` ; sed s/20080725/$$VERSION/ < /tmp/Info.plist >FontForge.app/Contents/Info.plist )
	rm /tmp/Info.plist

sfddiff: sfddiff.pe
	echo "#!$(bindir)/fontforge -lang=ff" > $@
	cat $< >> $@
	chmod +x $@

fontimage: fontimage.pe
	echo "#!$(bindir)/fontforge -lang=ff" > $@
	cat $< >> $@
	chmod +x $@

fontlint: fontlint.pe
	echo "#!$(bindir)/fontforge -lang=ff" >$@
	cat $< >>$@
	chmod +x $@

sfddiff.old: $(DIFFOBJS)
	$(LIBTOOL) --mode=link $(CC) -o sfddiff.old $(DIFFOBJS) $(DLIBS)

acorn2sfd: $(ACORNOBJS)
	$(LIBTOOL) --mode=link $(CC) -o acorn2sfd $(ACORNOBJS) $(DLIBS)

# The slight error introduced occasionally by optimization turns out to have
# disasterous effects. This file may not be compiled with the optimizer.
# On the other hand we do need CPPFLAGS to get the right includes...
splinerefigure.o: $(srcdir)/splinerefigure.c $(srcdir)/splinefont.h
	$(CC) -g -c -o splinerefigure.o -g  -I/usr/include/freetype2/ -I/usr/include/libxml2/ -I/usr/include/cairo -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16 -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/harfbuzz -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16 $(_CFLAGS) $(srcdir)/splinerefigure.c

splinerefigure.lo: $(srcdir)/splinerefigure.c $(srcdir)/splinefont.h
	$(LIBTOOL) --mode=compile $(CC) -g -c -o splinerefigure.lo -g  -I/usr/include/freetype2/ -I/usr/include/libxml2/ -I/usr/include/cairo -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16 -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/harfbuzz -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16 $(_CFLAGS) $(srcdir)/splinerefigure.c

%.o: %.c

%.o: %.c $(srcdir)/splinefont.h $(srcdir)/views.h $(srcdir)/stemdb.h $(srcdir)/ttf.h $(srcdir)/unicoderange.h $(srcdir)/psfont.h $(srcdir)/groups.h $(srcdir)/edgelist.h $(srcdir)/edgelist2.h $(srcdir)/uiinterface.h $(srcdir)/fontforgevw.h
	$(CC) $(CFLAGS) -c $<

%.lo: %.c

%.lo: %.c $(srcdir)/splinefont.h $(srcdir)/views.h $(srcdir)/stemdb.h $(srcdir)/ttf.h $(srcdir)/unicoderange.h $(srcdir)/psfont.h $(srcdir)/groups.h $(srcdir)/edgelist.h $(srcdir)/edgelist2.h $(srcdir)/uiinterface.h $(srcdir)/fontforgevw.h
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $<

clean:
	-rm -f fontforge sfddiff fontimage acorn2sfd fontimage fontlint \
		../libfontforge.la exelibstamp.c libstamp.c
	-rm -rf FontForge.app __MACOSX
	-rm -f *.o
	-rm -f *.lo
	-rm -rf .libs

distclean cleaner: clean
	-rm -f Makefile

install:
	( cd .. ; make install )

FontForge-MenuShortCuts.pot: *.c
	xgettext -kH_ -oFontForge-MenuShortCuts.pot *.c ../gdraw/*.c
	-patch < potmstitle.patch

FontForge.pot: *.c utf8.pot
	echo "If this doesn't work try make fontforge-old.pot instead"
	xgettext --from-code=UTF-8 -k_ -kN_ -kS_ -kU_ -kNU_ -kP_:1,2 --add-comments=GT: -oFontForge.pot *.c ../gdraw/*.c
	-patch < pottitle.patch

# xgettext has caniptions and strips any non-ASCII characters from my strings
# but I do occasionally want to use unicode characters (TeX, arrows, Increment)
# so I have put these messages in utf8.pot which I manage by hand. non-ASCII
# strings work just fine as keys. It does mean that my pot files should be
# viewed in utf8 though.

# I am told that new versions of xgettext support an option --from-code=UTF-8
# unfortunately this works on none of the systems I use, and I haven't found
# it documented. If it worked I would not need utf8.pot.
FontForge-old.pot: *.c utf8.pot
	xgettext -k_ -kN_ -kS_ -kP_:1,2 --add-comments=GT: -oFontForge.pot *.c ../gdraw/*.c
	cat utf8.pot >> FontForge.pot
	-patch < pottitle.patch
