/*
 * tor.c
 *
 * Copyright (C) 2011 shibby
 *
 */
#include <rc.h>
#include <sys/stat.h>

void start_tor(void)
{
    FILE *fp;
    char *ip;

//  only if enable...
    if( nvram_match( "tor_enable", "1" ) )
    {

    if (nvram_match( "tor_iface", "br0" ) ) { ip = nvram_safe_get( "lan_ipaddr" ); }
            else if (nvram_match( "tor_iface", "br1" ) ) { ip = nvram_safe_get( "lan1_ipaddr" ); }
            else if (nvram_match( "tor_iface", "br2" ) ) { ip = nvram_safe_get( "lan2_ipaddr" ); }
            else if (nvram_match( "tor_iface", "br3" ) ) { ip = nvram_safe_get( "lan3_ipaddr" ); }
            else { ip = nvram_safe_get( "lan_ipaddr" ); }


//      writing data to file
        if( !( fp = fopen( "/etc/tor.conf", "w" ) ) )
        {
            perror( "/etc/tor.conf" );
            return;
        }
            fprintf(fp, "SocksPort %d\n", nvram_get_int( "tor_socksport" ) );
//            fprintf(fp, "SocksBindAddress 127.0.0.1\n");
            fprintf(fp, "AllowUnverifiedNodes middle,rendezvous\n");
            fprintf(fp, "RunAsDaemon 1\n");
            fprintf(fp, "Log notice syslog\n");
            fprintf(fp, "DataDirectory %s\n", nvram_safe_get( "tor_datadir" ) );
            fprintf(fp, "TransPort %s:%s\n", ip, nvram_safe_get( "tor_transport" ) );
            fprintf(fp, "DNSPort %s:%s\n", ip, nvram_safe_get( "tor_dnsport" ) );
            fprintf(fp, "User nobody\n");
            fprintf(fp, "%s\n", nvram_safe_get("tor_custom"));

    fclose( fp );
    chmod( "/etc/tor.conf", 0644 );
    chmod( "/dev/null", 0666 );

    mkdir( nvram_safe_get("tor_datadir"), 0777 );
    xstart( "chown", "nobody:nobody", nvram_safe_get("tor_datadir") );

    xstart( "tor", "-f", "/etc/tor.conf" );
    }

    return;
}

void stop_tor(void)
{
    killall("tor", SIGTERM);
    return;
}
