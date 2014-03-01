Summary:     mdadm is used for controlling Linux md devices (aka RAID arrays)
Name:        mdadm
Version:     2.6
Release:     1
Source:      http://www.cse.unsw.edu.au/~neilb/source/mdadm/mdadm-%{version}.tgz
URL:         http://www.cse.unsw.edu.au/~neilb/source/mdadm/
License:     GPL
Group:       Utilities/System
BuildRoot:   %{_tmppath}/%{name}-root
Obsoletes:   mdctl

%description 
mdadm is a program that can be used to create, manage, and monitor
Linux MD (Software RAID) devices.
As such is provides similar functionality to the raidtools packages.
The particular differences to raidtools is that mdadm is a single
program, and it can perform (almost) all functions without a
configuration file (that a config file can be used to help with
some common tasks).

%prep
%setup -q
# we want to install in /sbin, not /usr/sbin...
%define _exec_prefix %{nil}

%build
# This is a debatable issue. The author of this RPM spec file feels that
# people who install RPMs (especially given that the default RPM options
# will strip the binary) are not going to be running gdb against the
# program.
make CXFLAGS="$RPM_OPT_FLAGS" SYSCONFDIR="%{_sysconfdir}"

%install
make DESTDIR=$RPM_BUILD_ROOT MANDIR=%{_mandir} BINDIR=%{_sbindir} install
install -D -m644 mdadm.conf-example $RPM_BUILD_ROOT/%{_sysconfdir}/mdadm.conf

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc TODO ChangeLog mdadm.conf-example COPYING
%{_sbindir}/mdadm
%config(noreplace,missingok)/%{_sysconfdir}/mdadm.conf
%{_mandir}/man*/md*

%changelog
* Fri May 10 2002  <neilb@cse.unsw.edu.au>
- update to 1.0.0
- Set CXFLAGS instead of CFLAGS

* Sat Apr  6 2002  <neilb@cse.unsw.edu.au>
- change install to use "make install"

* Fri Mar 15 2002  <gleblanc@localhost.localdomain>
- beautification
- made mdadm.conf non-replaceable config
- renamed Copyright to License in the header
- added missing license file
- used macros for file paths

* Fri Mar 15 2002 Luca Berra <bluca@comedia.it>
- Added Obsoletes: mdctl
- missingok for configfile

* Wed Mar 12 2002 NeilBrown <neilb@cse.unsw.edu.au>
- Add md.4 and mdadm.conf.5 man pages

* Fri Mar 08 2002		Chris Siebenmann <cks@cquest.utoronto.ca>
- builds properly as non-root.

* Fri Mar 08 2002 Derek Vadala <derek@cynicism.com>
- updated for 0.7, fixed /usr/share/doc and added manpage

* Tue Aug 07 2001 Danilo Godec <danci@agenda.si>
- initial RPM build
