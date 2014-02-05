/*
 * snmp.c
 *
 * Copyright (C) 2011 shibby
 *
 */
#include <rc.h>
#include <sys/stat.h>

void start_snmp(void)
{
    FILE *fp;

//  only if enable...
    if( nvram_match( "snmp_enable", "1" ) )
    {

//      writing data to file
        if( !( fp = fopen( "/etc/snmpd.conf", "w" ) ) )
        {
            perror( "/etc/snmpd.conf" );
            return;
        }
            fprintf(fp, "agentaddress udp:%d\n", nvram_get_int( "snmp_port" ) );
            fprintf(fp, "syslocation %s\n", nvram_safe_get( "snmp_location" ) );
            fprintf(fp, "syscontact %s <%s>\n", nvram_safe_get( "snmp_contact" ),nvram_safe_get( "snmp_contact" ) );
            fprintf(fp, "rocommunity %s\n", nvram_safe_get( "snmp_ro" ) );

    fclose( fp );
    chmod( "/etc/snmpd.conf", 0644 );

    xstart( "snmpd", "-c", "/etc/snmpd.conf" );
    }

    return;
}

void stop_snmp(void)
{
    killall("snmpd", SIGTERM);
    return;
}
