[![Build Status](https://travis-ci.org/jedisct1/dnscrypt-proxy.png?branch=master)](https://travis-ci.org/jedisct1/dnscrypt-proxy?branch=master)

[DNSCrypt](http://dnscrypt.org)
===============================

A tool for securing communications between a client and a DNS resolver.

Description
-----------

dnscrypt-proxy provides local service which can be used directly as
your local resolver or as a DNS forwarder, authenticating requests
using the DNSCrypt protocol and passing them to an upstream server, by
default OpenDNS.

The DNSCrypt protocol uses high-speed high-security elliptic-curve
cryptography and is very similar to [DNSCurve](http://dnscurve.org/),
but focuses on securing communications between a client and its first-level
resolver.

While not providing end-to-end security, it protects the local
network, which is often the weakest point of the chain, against
man-in-the-middle attacks. It also provides some confidentiality to
DNS queries.

Current list of free, DNSCrypt-enabled resolvers
------------------------------------------------

* [OpenDNS](http://www.opendns.com)
  - Server address: 208.67.220.220:443
  - Provider name: 2.dnscrypt-cert.opendns.com
  - Public key: B735:1140:206F:225D:3E2B:D822:D7FD:691E:A1C3:3CC8:D666:8D0C:BE04:BFAB:CA43:FB79

* [CloudNS](https://cloudns.com.au/) - No logs, DNSSEC
  * Canberra, Australia
    - Server address: 113.20.6.2:443 or gc2tzw6lbmeagrp3.onion:443
    - Provider name: 2.dnscrypt-cert.cloudns.com.au
    - Public key: 1971:7C1A:C550:6C09:F09B:ACB1:1AF7:C349:6425:2676:247F:B738:1C5A:243A:C1CC:89F4
  * Sydney, Australia
    - Server address: 113.20.8.17:443 or l65q62lf7wnfme7m.onion:443
    - Provider name: 2.dnscrypt-cert-2.cloudns.com.au
    - Public key: 67A4:323E:581F:79B9:BC54:825F:54FE:1025:8B4F:37EB:0D07:0BCE:4010:6195:D94F:E330

* [OpenNIC](http://www.opennicproject.org/) - No logs
  * Japan
    - Server address: 106.186.17.181:2053
    - Provider name: 2.dnscrypt-cert.ns2.jp.dns.opennic.glue
    - Public key: 8768:C3DB:F70A:FBC6:3B64:8630:8167:2FD4:EE6F:E175:ECFD:46C9:22FC:7674:A1AC:2E2A
  * UK
    * NovaKing (ns8)
      - Server address: 185.19.104.45:443
      - Provider name: 2.dnscrypt-cert.ns8.uk.dns.opennic.glue
      - Public key: A17C:06FC:BA21:F2AC:F4CD:9374:016A:684F:4F56:564A:EB30:A422:3D9D:1580:A461:B6A6
    * NovaKing (ns9)
      - Server address: 185.19.105.6:443
      - Provider name: 2.dnscrypt-cert.ns9.uk.dns.opennic.glue
      - Public key: E864:80D9:DFBD:9DB4:58EA:8063:292F:EC41:9126:8394:BC44:FAB8:4B6E:B104:8C3B:E0B4
    * NovaKing (ns10)
      - Server address: 185.19.105.14:443
      - Provider name: 2.dnscrypt-cert.ns10.uk.dns.opennic.glue
      - Public key: B1AB:7025:1119:9AEE:E42E:1B12:F2EF:12D4:53D9:CD92:E07B:9AF4:4794:F6EB:E5A4:F725

* [DNSCrypt.eu](http://dnscrypt.eu/) - No logs, DNSSEC
  * Holland
    - Server address: 176.56.237.171:443
    - Provider name: 2.dnscrypt-cert.dnscrypt.eu
    - Public key: 67C0:0F2C:21C5:5481:45DD:7CB4:6A27:1AF2:EB96:9931:40A3:09B6:2B8D:1653:1185:9C66

* [Soltysiak.com](http://dc1.soltysiak.com/) - No logs, DNSSEC
  * Poznan, Poland
    - Server address: 178.216.201.222:2053
    - Provider name: 2.dnscrypt-cert.soltysiak.com
    - Public key: 25C4:E188:2915:4697:8F9C:2BBD:B6A7:AFA4:01ED:A051:0508:5D53:03E7:1928:C066:8F21


Download and integrity check
----------------------------

DNSCrypt can be downloaded here: [dnscrypt download](http://dnscrypt.org)

After having downloaded a file, compute its SHA256 digest. For example:

    $ openssl dgst -sha256 dnscrypt-proxy-1.3.3.tar.bz2

Verify this digest against the expected one, that can be retrieved
using a simple DNS query:

    $ drill -D TXT dnscrypt-proxy-1.3.3.tar.bz2.download.dnscrypt.org

or

    $ dig +dnssec TXT dnscrypt-proxy-1.3.3.tar.bz2.download.dnscrypt.org

If the content of the TXT record doesn't match the SHA256 digest you
computed, please file a bug report on Github as soon as possible and
don't go any further.

Installation
------------

The daemon is known to work on recent versions of OSX, OpenBSD,
Bitrig, NetBSD, Dragonfly BSD, FreeBSD, Linux, iOS (requires a
jailbroken device), Android (requires a rooted device), Solaris
(SmartOS) and Windows (requires MingW).

Install [libsodium](https://github.com/jedisct1/libsodium).
On Linux, don't forget to run `ldconfig` if you installed it from
source.

On Fedora, RHEL and CentOS, you may need to add `/usr/local/lib` to
the paths the dynamic linker is going to look at. Before issuing
`ldconfig`, type:

    # echo /usr/local/lib > /etc/ld.so.conf.d/usr_local_lib.conf

Now, download the latest dnscrypt-proxy version and extract it:

    $ bunzip2 -cd dnscrypt-proxy-*.tar.bz2 | tar xvf -
    $ cd dnscrypt-proxy-*

Compile and install it using the standard procedure:

    $ ./configure && make -j2
    # make install

Replace `-j2` with whatever number of CPU cores you want to use for the
compilation process.

The proxy will be installed as `/usr/local/sbin/dnscrypt-proxy` by default.

Command-line switches are documented in the `dnscrypt-proxy(8)` man page.

*Note:* gcc 3.4.6 (and probably other similar versions) is known to
produce broken code on Mips targets with the -Os optimization level.
Use a different level (-O and -O2 are fine) or upgrade the compiler.
Thanks to Adrian Kotelba for reporting this.

GUI for dnscrypt-proxy
----------------------

If you need a simple graphical user interface in order to start/stop
the proxy and change your DNS settings, check out the following
project:

- [DNSCrypt WinClient](https://github.com/FivfBx2dOQTC3gc8YS4yMNo0el/dnscrypt-winclient):
Easily enable/disable DNSCrypt on multiple adapters. Supports
different ports and protocols, IPv6, parental controls and the proxy
can act as a gateway service. Windows only, written in .NET. Designed
for OpenDNS and CloudNS.

Server-side proxy
-----------------

[DNSCrypt-Wrapper](https://github.com/Cofyc/dnscrypt-wrapper) is a
server-side dnscrypt proxy that works with any name resolver.

Usage
-----

Having a dedicated system user, with no privileges and with an empty
home directory, is highly recommended. For extra security, DNSCrypt
will chroot() to this user's home directory and drop root privileges
for this user's uid as soon as possible.

The easiest way to start the daemon is:

    # dnscrypt-proxy --daemonize

The proxy will accept incoming requests on 127.0.0.1, tag them with an
authentication code, forward them to OpenDNS resolvers, and validate
each answer before passing it to the client.

Given such a setup, in order to actually start using DNSCrypt, you
need to update your `/etc/resolv.conf` file and replace your current
set of resolvers with:

    nameserver 127.0.0.1

Other common command-line switches include:

* `--daemonize` in order to run the server as a background process.
* `--local-address=<ip>[:port]` in order to locally bind a different IP
address than 127.0.0.1
* `--logfile=<file>` in order to write log data to a dedicated file. By
  default, logs are sent to stdout if the server is running in foreground,
  and to syslog if it is running in background.
* `--loglevel=<level>` if you need less verbosity in log files.
* `--max-active-requests=<count>` to set the maximum number of active
  requests. The default value is 250.
* `--pid-file=<file>` in order to store the PID number to a file.
* `--user=<user name>` in order to chroot()/drop privileges.
* `--test` in order to check that the server-side proxy is properly
configured and that a valid certificate can be used. This is useful
for monitoring your own dnscrypt proxy. See the man page for more
information.

DNSCrypt comes pre-configured for OpenDNS, although the
`--resolver-address=<ip>[:port]`,
`--provider-name=<certificate provider FQDN>`
and `--provider-key=<provider public key>` can be specified in
order to change the default settings.

Installation as a service (Windows only)
----------------------------------------

The proxy can be installed as a Windows service.

Copy the `dnscrypt-proxy.exe` file to any location, as well as the
`libsodium-4.dll` file. Both should be in the same location. If you
are using plugins depending on ldns, copy the ldns DLL as well. Then open a
terminal and type (eventually with the full path to `dnscrypt-proxy.exe`):

    dnscrypt-proxy.exe --install

It will install a new service named `dnscrypt-proxy`.

After being stopped, the service can be removed with:

    dnscrypt-proxy.exe --uninstall

Using DNSCrypt in combination with a DNS cache
----------------------------------------------

The DNSCrypt proxy is **not** a DNS cache. This means that incoming
queries will **not** be cached and every single query will require a
round-trip to the upstream resolver.

For optimal performance, the recommended way of running DNSCrypt is to
run it as a forwarder for a local DNS cache, like `unbound` or
`powerdns-recursor`.

Both can safely run on the same machine as long as they are listening
to different IP addresses (preferred) or different ports.

If your DNS cache is `unbound`, all you need is to edit the
`unbound.conf` file and add the following lines at the end of the `server`
section:

    do-not-query-localhost: no

    forward-zone:
      name: "."
      forward-addr: 127.0.0.1@40

The first line is not required if you are using different IP addresses
instead of different ports.

Then start `dnscrypt-proxy`, telling it to use a specific port (`40`, in
this example):

    # dnscrypt-proxy --local-address=127.0.0.1:40 --daemonize

IPv6 support
------------

IPv6 is fully supported. IPv6 addresses with a port number should be
specified as [ip]:port

    # dnscrypt-proxy --local-address='[::1]:40' --daemonize

Queries using nonstandard ports / over TCP
------------------------------------------

Some routers and firewalls can block outgoing DNS queries or
transparently redirect them to their own resolver. This especially
happens on public Wifi hotspots, such as coffee shops.

As a workaround, the port number can be changed using
the `--resolver-port=<port>` option. For example, OpenDNS servers
reply to queries sent to ports 53, 443 and 5353.

By default, dnscrypt-proxy sends outgoing queries to UDP port 443.

In addition, the DNSCrypt proxy can force outgoing queries to be
sent over TCP. For example, TCP port 443, which is commonly used for
communication over HTTPS, may not be filtered.

The `--tcp-only` command-line switch forces this behavior. When
an incoming query is received, the daemon immediately replies with a
"response truncated" message, forcing the client to retry over TCP.
The daemon then authenticates the query and forwards it over TCP
to the resolver.

`--tcp-only` is slower than UDP because multiple queries over a single
TCP connections aren't supported yet, and this workaround should
never be used except when bypassing a filter is actually required.

EDNS payload size
-----------------

DNS packets sent over UDP have been historically limited to 512 bytes,
which is usually fine for queries, but sometimes a bit short for
replies.

Most modern authoritative servers, resolvers and stub resolvers
support the Extension Mechanism for DNS (EDNS) that, among other
things, allows a client to specify how large a reply over UDP can be.

Unfortunately, this feature is disabled by default on a lot of
operating systems. It has to be explicitly enabled, for example by
adding `options edns0` to the `/etc/resolv.conf` file on most
Unix-like operating systems.

`dnscrypt-proxy` can transparently rewrite outgoing packets before
authenticating them, in order to add the EDNS0 mechanism. By
default, a conservative payload size of 1252 bytes is advertised.

This size can be made larger by starting the proxy with the
`--edns-payload-size=<bytes>` command-line switch. Values up to 4096
are usually safe.

A value below or equal to 512 will disable this mechanism, unless a
client sends a packet with an OPT section providing a payload size.

The `hostip` utility
--------------------

The DNSCrypt proxy ships with a simple tool named `hostip` that
resolves a name to IPv4 or IPv6 addresses.

This tool can be useful for starting some services before
`dnscrypt-proxy`.

Queries made by `hostip` are not authenticated.
