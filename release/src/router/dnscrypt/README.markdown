DNSCrypt
========

A tool for securing communications between a client and a DNS resolver.

Description
-----------

DNSCrypt is a slight variation on [DNSCurve](http://www.dnscurve.org/).

DNSCurve improves the confidentiality and integrity of DNS requests using
high-speed high-security elliptic-curve cryptography. Best of all,
DNSCurve has very low overhead and adds virtually no latency to
queries.

DNSCurve aims at securing the entire chain down to authoritative
servers. However, it only works with authoritative servers that explicitly
support the protocol. And unfortunately, DNSCurve hasn't received much
adoption yet.

The DNSCrypt protocol is very similar to DNSCurve, but focuses on
securing communications between a client and its first-level resolver.
While not providing end-to-end security, it protects the local
network (which is often the weakest link in the chain) against
man-in-the-middle attacks. It also provides some confidentiality to
DNS queries.

The DNSCrypt daemon acts as a DNS proxy between a regular client, like
a DNS cache or an operating system stub resolver, and a DNSCrypt-aware
resolver, like OpenDNS.

Installation
------------

The daemon is known to work on recent versions of OSX, OpenBSD,
NetBSD, Dragonfly BSD, FreeBSD, Linux, Windows (MingW or Cygwin), and iOS
(requires a jailbroken device).

Download the
[latest version](https://github.com/opendns/dnscrypt-proxy/downloads)
and extract it:

    $ bunzip2 -cd dnscrypt-proxy-*.tar.bz2 | tar xvf -
    $ cd dnscrypt-proxy-*

Compile and install it using the standard procedure:

    $ ./configure && make -j2
    # make install

Replace `-j2` with whatever number of CPU cores you want to use for the
compilation process.

Running `make -j2 test` in the `src/libnacl` directory is also highly
recommended.

On BSD systems, _GNU Make_ should be installed prior to running the
`./configure` script.

The proxy will be installed as `/usr/local/sbin/dnscrypt-proxy` by default.

Command-line switches are documented in the `dnscrypt-proxy(8)` man page.

Usage
-----

Having a dedicated system user, with no privileges and with an empty
home directory, is highly recommended. For extra security, DNSCrypt
will chroot() to this user's home directory and drop root privileges
for this user's uid as soon as possible.

The easiest way to start the daemon is:

    # dnscrypt-proxy --daemonize

The proxy will accept incoming requests on 127.0.0.1 and
encrypt/decrypt them from/to OpenDNS resolvers.

Given such a setup, in order to actually start using DNSCrypt, you
need to update your `/etc/resolv.conf` file and replace your current
set of resolvers with:

    nameserver 127.0.0.1

Other common command-line switches include:

* `--daemonize` in order to run the server as a background process.
* `--local-address=<ip>` in order to locally bind a different IP address than
  127.0.0.1
* `--local-port=<port>` to change the local port to listen to.
* `--logfile=<file>` in order to write log data to a dedicated file. By
  default, logs are sent to stdout if the server is running in foreground,
  and to syslog if it is running in background.
* `--max-active-requests=<count>` to set the maximum number of active
  requests. The default value is 250.
* `--pid-file=<file>` in order to store the PID number to a file.
* `--user=<user name>` in order to chroot()/drop privileges.

DNSCrypt comes pre-configured for OpenDNS, although the
`--resolver-address=<ip>`, `--provider-name=<certificate provider FQDN>`
and `--provider-key=<provider public key>` can be specified in
order to change the default settings.

Using DNSCrypt in combination with a DNS cache
----------------------------------------------

The DNSCrypt proxy is **not** a DNS cache. This means that incoming
queries will **not** be cached and every single query will require a
round-trip to the upstream resolver.

For optimal performance, the recommended way of running DNSCrypt is to
run it as a forwarder for a local DNS cache, like `unbound`, `pdns` or
`dnscache`.

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

    # dnscrypt-proxy --local-port=40 --daemonize

Queries over TCP
----------------

Some routers and firewalls can block outgoing DNS queries or
transparently redirect them to their own resolver. This especially
happens on public Wifi hotspots, such as coffee shops.

As a workaround, the DNSCrypt proxy can force outgoing queries to be
sent over TCP. For example, TCP port 443, which is commonly used for
communication over HTTPS, may not be filtered.

The `tcp-port=<port>` command-line switch forces this behavior. When
an incoming query is received, the daemon immediately replies with a
"response truncated" message, forcing the client to retry over TCP.
The daemon then encrypts and signs the query and forwards it over TCP
to the resolver.

TCP is slower than UDP, and this workaround should never be used
except when bypassing a filter is actually required. Moreover,
multiple queries over a single TCP connections aren't supported yet.

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
signing and encrypting them, in order to add the EDNS0 mechanism. By
default, a conservative payload size of 1280 bytes is advertised.

This size can be made larger by starting the proxy with the
`--edns-payload-size=<bytes>` command-line switch. Values up to 4096
are usually safe.

A value below or equal to 512 will disable this mechanism, unless a
client sends a packet with an OPT section providing a payload size.

GUIs for dnscrypt-proxy
-----------------------

If you need a simple graphical user interface in order to start/stop
the proxy and change your DNS settings, check out the following
projects:

- [DNSCrypt OSX Client](https://github.com/opendns/dnscrypt-osx-client):
a preferences pane, a menu bar indicator and a service to change the
DNS settings. OSX only, written in Objective C. 64-bit CPU required.
Experimental.

- [DNSCrypt WinClient](https://github.com/Noxwizard/dnscrypt-winclient):
Easily enable/disable DNSCrypt on multiple adapters. Windows only,
written in .NET.

