###############################################################################
#
# General mumbojumbo
#
###############################################################################

Name: dnsmasq
Version: 1.10
Release: 1
Copyright: GPL
Group: System Environment/Daemons
Vendor: Simon Kelley
Packager: Simon Kelley
Distribution: Red Hat Linux
URL: http://www.thekelleys.org.uk/dnsmasq
Source0: %{name}-%{version}.tar.gz
Requires: chkconfig
BuildRoot: /var/tmp/%{name}-%{version}
Summary: A lightweight caching nameserver

%description
Dnsmasq is lightweight, easy to configure DNS forwarder designed to provide DNS (domain name) services to a small network where using BIND would be overkill. It can be have its DNS servers automatically configured by PPP or DHCP, and it can serve the names of local machines which are not in the global DNS. It is ideal for networks behind NAT routers and connected via modem, ISDN, ADSL, or cable-modem connections. 


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

mkdir -p -m 755 $RPM_BUILD_ROOT/usr/sbin
mkdir -p -m 755 $RPM_BUILD_ROOT/etc/rc.d/init.d
mkdir -p -m 755 $RPM_BUILD_ROOT/usr/share/man/man8

cp dnsmasq.rh $RPM_BUILD_ROOT/etc/rc.d/init.d/dnsmasq
strip dnsmasq
cp dnsmasq $RPM_BUILD_ROOT/usr/sbin
cp dnsmasq.8 $RPM_BUILD_ROOT/usr/share/man/man8
gzip $RPM_BUILD_ROOT/usr/share/man/man8/dnsmasq.8

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
/sbin/chkconfig --add dnsmasq


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
    service dnsmasq stop >/dev/null 2>&1
    /sbin/chkconfig --del dnsmasq
fi


###############################################################################
#
# Post-uninstall scriptlet
#
# The %postun script executes after the package has been removed. It is the
# last chance for a package to clean up after itself.
#
###############################################################################

%postun
if [ "$1" -ge "1" ]; then
    service dnsmasq restart >/dev/null 2>&1
fi


###############################################################################
#
# File list
#
###############################################################################

%files
%defattr(-,root,root)
%doc CHANGELOG COPYING doc.html setup.html
%attr(0755,root,root) /etc/rc.d/init.d/dnsmasq
%attr(0755,root,root) /usr/sbin/dnsmasq
%attr(0644,root,root) /usr/share/man/man8/dnsmasq.8.gz


