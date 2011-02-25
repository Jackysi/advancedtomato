/*
 * transmission.c
 *
 * Copyright (C) 2011 shibby
 *
 */
#include <stdlib.h>
#include <rc.h>
#include <shutils.h>
#include <utils.h>
#include <syslog.h>
#include <sys/stat.h>

void start_bittorrent(void)
{
    FILE *fp;
    char *pb;
    char *pc;
    char *pd;
    char *pe;
    char *ph;
    char *pi;
    char *pj;
    char *pk;

// make sure its really stop
    stop_bittorrent();

//only if enable...
    if( !nvram_match( "bt_enable", "1" ) ) return;

    //collecting data
    if (nvram_match( "bt_rpc_enable", "1") ) { pb = "true"; } else { pb = "false"; }
    if (nvram_match( "bt_dl_enable", "1") ) { pc = "true"; } else { pc = "false"; }
    if (nvram_match( "bt_ul_enable", "1") ) { pd = "true"; } else { pd = "false"; }
    if (nvram_match( "bt_incomplete", "1") ) { pe = "true"; } else { pe = "false"; }
    if (nvram_match( "bt_ratio_enable", "1") ) { ph = "true"; } else { ph = "false"; }
    if (nvram_match( "bt_dht", "1") ) { pi = "true"; } else { pi = "false"; }
    if (nvram_match( "bt_pex", "1") ) { pj = "true"; } else { pj = "false"; }
    if (nvram_match( "bt_settings", "down_dir" ) ) { pk = nvram_safe_get( "bt_dir" ); } else { pk = nvram_safe_get( "bt_settings" ); }

    //writing data to file
    if( !( fp = fopen( "/tmp/settings.json", "w" ) ) )
    {
        perror( "/tmp/settings.json" );
        return;
    }

    fprintf( fp, "{\n" );
    fprintf( fp, "\"peer-port\": %s, \n", nvram_safe_get( "bt_port" ) );
    fprintf( fp, "\"speed-limit-down-enabled\": %s, \n", pc );
    fprintf( fp, "\"speed-limit-up-enabled\": %s, \n", pd );
    fprintf( fp, "\"speed-limit-down\": %s, \n", nvram_safe_get( "bt_dl" ) );
    fprintf( fp, "\"speed-limit-up\": %s, \n", nvram_safe_get( "bt_ul" ) );
    fprintf( fp, "\"rpc-enabled\": %s, \n", pb );
    fprintf( fp, "\"rpc-bind-address\": \"0.0.0.0\", \n");
    fprintf( fp, "\"rpc-port\": %s, \n", nvram_safe_get( "bt_port_gui" ) );
    fprintf( fp, "\"rpc-whitelist-enabled\": false, \n");
    fprintf( fp, "\"rpc-username\": \"%s\", \n", nvram_safe_get( "bt_login" ) );
    fprintf( fp, "\"rpc-password\": \"%s\", \n", nvram_safe_get( "bt_password" ) );
    fprintf( fp, "\"download-dir\": \"%s\", \n", nvram_safe_get( "bt_dir" ) );
    fprintf( fp, "\"incomplete-dir-enabled\": \"%s\", \n", pe );
    fprintf( fp, "\"incomplete-dir\": \"%s/.incomplete\", \n", nvram_safe_get( "bt_dir" ) );
    fprintf( fp, "\"peer-limit-global\": %s, \n", nvram_safe_get( "bt_peer_limit_global" ) );
    fprintf( fp, "\"peer-limit-per-torrent\": %s, \n", nvram_safe_get( "bt_peer_limit_per_torrent" ) );
    fprintf( fp, "\"upload-slots-per-torrent\": %s, \n", nvram_safe_get( "bt_ul_slot_per_torrent" ) );
    fprintf( fp, "\"dht-enabled\": %s, \n", pi );
    fprintf( fp, "\"pex-enabled\": %s, \n", pj );
    fprintf( fp, "\"ratio-limit-enabled\": %s, \n", ph );
    fprintf( fp, "\"ratio-limit\": %s, \n", nvram_safe_get( "bt_ratio" ) );
    fprintf( fp, "%s\n", nvram_safe_get("bt_custom"));
    fprintf( fp, "\"rpc-authentication-required\": true \n");
    fprintf( fp, "}\n");

    fclose( fp );
    chmod( "/tmp/settings.json", 0644 );

//start file
    if( !( fp = fopen( "/tmp/start_transmission.sh", "w" ) ) )
    {
        perror( "/tmp/start_transmission.sh" );
        return;
    }

    fprintf( fp, "#!/bin/sh\n" );
    fprintf( fp, "sleep 2\n");

    if ( nvram_match( "bt_incomplete", "1") )
    {
        fprintf( fp, "if [ ! -d \"%s/.incomplete\" ]; then\n", pk );
        fprintf( fp, "mkdir %s/.incomplete\n", pk );
        fprintf( fp, "fi\n");
    }

    fprintf( fp, "if [ ! -d \"%s/.settings\" ]; then\n", pk );
    fprintf( fp, "mkdir %s/.settings\n", pk );
    fprintf( fp, "fi\n");
    fprintf( fp, "mv /tmp/settings.json %s/.settings\n", pk );
    fprintf( fp, "/usr/bin/transmission-daemon -g %s/.settings\n", pk );
    fprintf( fp, "logger \"Transmission daemon successfully started\" \n");
    fprintf( fp, "sleep 2\n" );
    fprintf( fp, "iptables -A INPUT -p tcp --dport %s -j ACCEPT\n", nvram_safe_get( "bt_port" ) );

    if (nvram_match( "bt_rpc_wan", "1") ) 
    {
    fprintf( fp, "iptables -A INPUT -p tcp --dport %s -j ACCEPT\n", nvram_safe_get( "bt_port_gui" ) );
    }
    fclose( fp );

    chmod( "/tmp/start_transmission.sh", 0755 );

    xstart( "/tmp/start_transmission.sh" );
    return;

}

void stop_bittorrent(void)
{
    FILE *fp;

//stop file
    if( !( fp = fopen( "/tmp/stop_transmission.sh", "w" ) ) )
    {
        perror( "/tmp/stop_transmission.sh" );
        return;
    }

    fprintf( fp, "#!/bin/sh\n" );
    fprintf( fp, "killall -KILL transmission-daemon\n");
    fprintf( fp, "logger \"Transmission daemon successfully stoped\" \n");
    if (nvram_match( "new_qoslimit_enable", "1") )
    {
        fprintf( fp, "service new_qoslimit restart\n" );
    }
    else
    {
        fprintf( fp, "service firewall restart\n" );
    }
    fprintf( fp, "sleep 2\n");
    fclose( fp );
    chmod( "/tmp/stop_transmission.sh", 0755 );

    xstart( "/tmp/stop_transmission.sh" );
    return;
}
