%define prefix   /usr
%define name prozilla
%define version 1.3.3.3

Summary: An advanced Linux download manager
Name: %{name}
Version: %{version}
Release: 1
Copyright: GNU
Group: Networking/File transfer
Source0: %{name}-%{version}.tar.gz 
Url: http://prozilla.delrom.ro
Packager: Ralph Slooten <axllent@axllent.cjb.net>
BuildRoot: /var/tmp/%{name}-%{version}-root

%description
ProZilla is a new download accellerator program written for
Linux to speed up the normal file download process. It often
gives speed increases of around 200% to 300%. It supports
both FTP and HTTP protocols, and the theory behind it is
very simple. The program opens multiple connections to a
server, and each of the connections downloads a part of the
file, thus defeating existing internet congestion prevention
methods which slow down a single connection based download.

ProZilla also supports file download resuming, and ftpsearch
for fastest ping times.

%prep
rm -rf $RPM_BUILD_ROOT

%setup -n %{name}-%{version}

CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix} --sysconfdir=/etc

%build
make
mkdir $RPM_BUILD_ROOT
mkdir $RPM_BUILD_ROOT/etc

%install
make install \
     prefix="${RPM_BUILD_ROOT}/%{prefix}" \
     sysconfdir="${RPM_BUILD_ROOT}/etc"

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/*

%files
%{prefix}/bin/proz
%config(noreplace) %verify(not size mtime md5) /etc/prozilla.conf
%config(noreplace) %verify(not size mtime md5) /etc/prozilla.conf-sample
%{prefix}/man/man1/proz*
%doc ANNOUNCE
%doc AUTHORS
%doc ChangeLog
%doc COPYING
%doc CREDITS
%doc FAQ
%doc INSTALL
%doc NEWS
%doc README
%doc TODO

%changelog
* Tue Mar 06 2001 Kalum / Grendel<kalum@lintux.cx>
- Prozilla 1.3.3.2 released
- Added several backup ftpsearch servers to fall back, if the main one
  fails
- Added prozilla.spec and modified the Makefile.am, to make rpms if
  necessary.

* Sun Mar 04 2001 Ralph Slooten <axllent@axllent.cjb.net>
- Prozilla 1.3.3.1 released
- ncurses display fixed to show no blank spaces

* Fri Mar 02 2001 Ralph Slooten <axllent@axllent.cjb.net>
- Development ProZilla 1.3.3 released
- Fixed ftp search URL
- Fixed Makefile.in problem fixed
- Spec file changed to suite /etc dir
- Spec file updated using definitions

* Sun Feb 18 2001 Ralph Slooten <axllent@axllent.cjb.net>
- ProZilla version 1.3.2 released
- Global preferences files added in /etc file
- Spec file changed to suite config file needs
- Makefile.in changed to suite RPM's needs... Needs to be fixed!

* Tue Feb 6 2001 Ralph Slooten <axllent@axllent.cjb.net>
- ProZilla version 1.3.1 released
- New FTP search function implemented into Prozilla
- proz manpage symlinked to prozilla man page
- gproz falls out due to drastic changes in Prozilla
- CHANGES file dropped due to existing Changelog
- Rpm spec file changes to suite Prozilla

* Fri Jan 25 2001 Ralph Slooten <axllent@axllent.cjb.net>
- Tweaked up the RPM spec file

* Mon Jan 22 2001 Ralph Slooten <axllent@axllent.cjb.net>
- man proz added, instead of just prozilla
- Server tweaking and changes (Read CHANGES)

* Wed Jan 3 2001 Calum Selkirk <cselkirk@sophix.uklinux.net>
- added RPM_BUILD_ROOT and install to that dir
- added RPM_OPT_FLAGS
- changed Source0: to use %version
