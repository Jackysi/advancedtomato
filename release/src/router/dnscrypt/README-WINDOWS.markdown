Using DNSCrypt on Windows
=========================

On Windows, `dnscrypt-proxy` can be started from the command-line the same way
as on other operating systems.

Alternatively, it can run as a Windows Service.

Note: Also check out Dominus Temporis' excellent tutorial on
[DNSCrypt on Windows](http://dominustemporis.com/2014/05/dnscrypt-on-windows-update/)

Quickstart
----------

1) Download and extract the latest
[Windows package for dnscrypt](http://download.dnscrypt.org/dnscrypt-proxy/LATEST-win32-full.zip).

2) Copy the `dnscrypt-proxy-win32` folder anywhere.

3) Open an elevated command prompt, enter the `dnscrypt-proxy-win32` folder
and type:

    dnscrypt-proxy.exe -R "name" --test=0

Replace `name` with one of the resolvers from CSV file. The (possibly
updated) file can also be viewed online:
[public DNS resolvers supporting DNSCrypt](https://github.com/jedisct1/dnscrypt-proxy/blob/master/dnscrypt-resolvers.csv)

This command should display the server key fingerprint and exit. If
this is not the case, try a different server. If this is the case,
install the service:

    dnscrypt-proxy.exe -R "name" --install

4) Change your DNS settings to `127.0.0.1`

Congratulations, you're now using DNSCrypt!

How to open an elevated command prompt
--------------------------------------

On Windows 8.1 and Windows 10, press the Windows key + the X key and
select "Windows Command Prompt (Admin)" or "Windows PowerShell (Admin)".

Advanced usage
--------------

The Windows build of `dnscrypt-proxy` adds the following command-line
options:

- `--install`: install the proxy as a service.
- `--uninstall`: uninstall the service.

Startup options should specified as subkeys from this registry key:
`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\dnscrypt-proxy\Parameters`

The service is named `dnscrypt-proxy`.

The following subkeys are recognized and should be self-explanatory:

    Plugins           (REG_MULTI_SZ)
    LocalAddress      (REG_SZ)
    ProviderKey       (REG_SZ)
    ProviderName      (REG_SZ)
    ResolverAddress   (REG_SZ)
    ResolverName      (REG_SZ)
    ResolversList     (REG_SZ)
    EDNSPayloadSize   (DWORD)
    MaxActiveRequests (DWORD)
    TCPOnly           (DWORD)
    ClientKeyFile     (REG_SZ)
    EphemeralKeys     (DWORD)

For example, in order to listen to local address that is not the default
`127.0.0.1`, the key to put the custom IP address is
`HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\dnscrypt-proxy\Parameters\LocalAddress`.

Mandatory entries to run `dnscrypt-proxy` as a Windows service are:
- `ResolversList`: has to be set to the full path to the `dnscrypt-resolvers.csv` file.
- `ResolverName`: has to be set to the resolver name to be used. See
the `dnscrypt-resolvers.csv` file for a list of compatible public resolvers.

These entries are automatically created/updated when installing the service.

Plugins should be listed as paths to the `.DLL` files, optionally
followed by a coma and plugin-specific arguments:

    dnscrypt-proxy -R name --plugin=libdcplugin_example_ldns_aaaa_blocking.dll
    dnscrypt-proxy -R name --plugin=libdcplugin_example_ldns_blocking.dll,--domains=C:/blacklisted-domains.txt

The service should be restarted after the registry has been updated.
