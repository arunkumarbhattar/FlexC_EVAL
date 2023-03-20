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
INSTALL = /usr/bin/install -c
CC = gcc

_CFLAGS = -I$(top_srcdir)/inc -I../inc -I$(srcdir) -I. -I$(top_srcdir)/fontforge \
    -Wmissing-prototypes -Wunused -Wimplicit -Wreturn-type -Wparentheses -Wformat -Wchar-subscripts $(X_CFLAGS) \
    -DHAVE_CONFIG_H '-DSHAREDIR="$(sharedir)"' -DLIBDIR='"$(libdir)"' \
     '-DPREFIX="/home/arun/Desktop/CVES/2010_4259/fontforge-20100501/install"'
CFLAGS = -g -O2 -I/usr/include/freetype2/ -I/usr/include/libxml2/ -I/usr/include/cairo -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/harfbuzz -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16 -pthread -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/fribidi -I/usr/include/harfbuzz -I/usr/include/cairo -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16 $(_CFLAGS)

LIBS = -rpath $(libdir)  -L/usr/lib -lm

PLUGINS = gb12345.la

all: $(PLUGINS)

clean:
	-rm -rf .libs *.la *.lo

distclean: clean
	-rm -rf Makefile

%.lo: %.c
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $<

%.la: %.lo
	$(LIBTOOL) --mode=link $(CC) -export-dynamic -module -o $@ $< $(LIBS)

install: all
	mkdir -p $(DESTDIR)$(plugindir)
	-mv .libs/*.{so,dll,dylib,lai}* $(DESTDIR)$(plugindir)
	mv *.la $(DESTDIR)$(plugindir)


