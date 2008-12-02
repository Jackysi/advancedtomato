###############################################################################
#
# General mumbojumbo
#
###############################################################################

Name: dnsmasq
Version:	1.10
Release:	1
Copyright:	GPL
Provides:	dns_daemon
Group:		System Environment/Daemons
Vendor:		N/A
Packager:	Jörg Mayer
Distribution:	SuSE Linux 7.3
URL:		http://www.thekelleys.org.uk/dnsmasq
Source0:	%{name}-%{version}.tar.gz
BuildRoot:	/var/tmp/%{name}-%{version}
Summary:	A lightweight caching nameserver

%description
Dnsmasq is lightweight, easy to configure DNS forwarder designed to provide
DNS (domain name) services to a small network where using BIND would be
overkill. It can be have its DNS servers automatically configured by PPP
or DHCP, and it can serve the names of local machines which are not in the
global DNS. It is ideal for networks behind NAT routers and connected via
modem, ISDN, ADSL, or cable-modem connections. 


###############################################################################
#
# Build
#
###############################################################################

%prep
%setup -q
%build
make


###############################################################################
#
# Install
#
###############################################################################

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p -m 755 $RPM_BUILD_ROOT%{_sbindir}
mkdir -p -m 755 $RPM_BUILD_ROOT/etc/init.d
mkdir -p -m 755 $RPM_BUILD_ROOT%{_mandir}/man8
mkdir -p -m 755 $RPM_BUILD_ROOT/var/adm/fillup-templates

cp rc.config.dnsmasq $RPM_BUILD_ROOT/var/adm/fillup-templates/rc.config.dnsmasq
cp dnsmasq.suse $RPM_BUILD_ROOT/etc/init.d/dnsmasq
strip dnsmasq
cp dnsmasq $RPM_BUILD_ROOT%{_sbindir}
cp dnsmasq.8 $RPM_BUILD_ROOT%{_mandir}/man8
gzip -9 $RPM_BUILD_ROOT%{_mandir}/man8/dnsmasq.8
ln -s /etc/init.d/dnsmasq $RPM_BUILD_ROOT%{_sbindir}/rcdnsmasq

###############################################################################
#
# Clean up
#
###############################################################################

%clean
rm -rf $RPM_BUILD_ROOT


###############################################################################
#
# Post-install scriptlet
#
###############################################################################

%post
echo "Updating etc/rc.config..."
if [ -x bin/fillup ] ; then
  bin/fillup -q -d = etc/rc.config var/adm/fillup-templates/rc.config.dnsmasq
else
  echo "ERROR: fillup not found. This should not happen. Please compare"
  echo "etc/rc.config and var/adm/fillup-templates/rc.config.dnsmasq and"
  echo "update by hand."
fi
sbin/insserv dnsmasq


###############################################################################
#
# Pre-uninstall scriptlet
#
# If there's a time when your package needs to have one last look around before
# the user erases it, the place to do it is in the %preun script. Anything that
# a package needs to do immediately prior to RPM taking any action to erase the
# package, can be done here.
#
###############################################################################

%preun
if [ $1 = 0 ]; then     # execute this only if we are NOT doing an upgrade
    %{_sbindir}rcdnsmasq stop >/dev/null 2>&1
    sbin/insserv -d dnsmasq
fi


###############################################################################
#
# Post-uninstall scriptlet
#
# The %postun script executes after the package has been removed. It is the
# last chance for a package to clean up after itself.
#
###############################################################################

#%postun
#if [ "$1" -ge "1" ]; then
#    rcdnsmasq restart >/dev/null 2>&1
#fi


###############################################################################
#
# File list
#
###############################################################################

%files
%defattr(-,root,root)
%doc CHANGELOG COPYING doc.html setup.html
%config(noreplace) %attr(0755,root,root) /etc/init.d/dnsmasq
%attr(0755,root,root) %{_sbindir}/dnsmasq
%attr(0755,root,root) %{_sbindir}/rcdnsmasq
%attr(0644,root,root) %{_mandir}/man8/dnsmasq.8.gz
%attr(0644,root,root) /var/adm/fillup-templates/rc.config.dnsmasq

%changelog
* Mon Nov 11 2001 - jmayer@loplof.de
- Initial rpm for use with SuSE (7.3)

