prefix = /home/arun/Desktop/CVES/2010_4259/fontforge-20100501/install
localedir = $(prefix)/share/locale
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

MOFILES = de.mo el.mo en_GB.mo es.mo fr.mo it.mo ja.mo mal.mo pl.mo ru.mo uk.mo vi.mo zh_CN.mo zh_TW.mo

.SUFFIXES:
.SUFFIXES: .po .mo

all: $(MOFILES)

.po.mo:
	msgfmt -o $@ $<

install:
	mkdir -p $(DESTDIR)$(localedir)/de/LC_MESSAGES
	$(INSTALL_DATA) de.mo $(DESTDIR)$(localedir)/de/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/el/LC_MESSAGES
	$(INSTALL_DATA) el.mo $(DESTDIR)$(localedir)/el/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/en_GB/LC_MESSAGES
	$(INSTALL_DATA) en_GB.mo $(DESTDIR)$(localedir)/en_GB/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/es/LC_MESSAGES
	$(INSTALL_DATA) es.mo $(DESTDIR)$(localedir)/es/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/fr/LC_MESSAGES
	$(INSTALL_DATA) fr.mo $(DESTDIR)$(localedir)/fr/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/it/LC_MESSAGES
	$(INSTALL_DATA) it.mo $(DESTDIR)$(localedir)/it/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/ja/LC_MESSAGES
	$(INSTALL_DATA) ja.mo $(DESTDIR)$(localedir)/ja/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/mal/LC_MESSAGES
	$(INSTALL_DATA) mal.mo $(DESTDIR)$(localedir)/mal/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/pl/LC_MESSAGES
	$(INSTALL_DATA) pl.mo $(DESTDIR)$(localedir)/pl/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/ru/LC_MESSAGES
	$(INSTALL_DATA) ru.mo $(DESTDIR)$(localedir)/ru/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/uk/LC_MESSAGES
	$(INSTALL_DATA) uk.mo $(DESTDIR)$(localedir)/uk/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/vi/LC_MESSAGES
	$(INSTALL_DATA) vi.mo $(DESTDIR)$(localedir)/vi/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/zh_CN/LC_MESSAGES
	$(INSTALL_DATA) zh_CN.mo $(DESTDIR)$(localedir)/zh_CN/LC_MESSAGES/FontForge.mo
	mkdir -p $(DESTDIR)$(localedir)/zh_TW/LC_MESSAGES
	$(INSTALL_DATA) zh_TW.mo $(DESTDIR)$(localedir)/zh_TW/LC_MESSAGES/FontForge.mo
