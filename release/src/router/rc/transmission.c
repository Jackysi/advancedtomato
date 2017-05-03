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
    char *pl;
    char *pm;
    char *pn;
    char *po;
    char *pp;
    char *pr;
    char *ps;
    char *pt;
    char *pu;

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
    if (nvram_match( "bt_settings", "down_dir" ) ){ pk = nvram_safe_get( "bt_dir" ); }
        else if (nvram_match( "bt_settings", "custom" ) ) { pk = nvram_safe_get( "bt_settings_custom" ); }
        else { pk = nvram_safe_get( "bt_settings" ); }
    if (nvram_match( "bt_auth", "1") ) { pl = "true"; } else { pl = "false"; }
    if (nvram_match( "bt_blocklist", "1") ) { pm = "true"; } else { pm = "false"; }
    if (nvram_match( "bt_binary", "internal" ) ) { pn = "/usr/bin"; }
        else if (nvram_match( "bt_binary", "optware" ) ) { pn = "/opt/bin"; }
        else { pn = nvram_safe_get( "bt_binary_custom" ); }
    if (nvram_match( "bt_lpd", "1") ) { po = "true"; } else { po = "false"; }
    if (nvram_match( "bt_utp", "1") ) { pp = "true"; } else { pp = "false"; }
    if (nvram_match( "bt_ratio_idle_enable", "1") ) { pr = "true"; } else { pr = "false"; }
    if (nvram_match( "bt_dl_queue_enable", "1") ) { pt = "true"; } else { pt = "false"; }
    if (nvram_match( "bt_ul_queue_enable", "1") ) { pu = "true"; } else { pu = "false"; }


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
    fprintf( fp, "\"lpd-enabled\": %s, \n", po );
    fprintf( fp, "\"utp-enabled\": %s, \n", pp );
    fprintf( fp, "\"ratio-limit-enabled\": %s, \n", ph );
    fprintf( fp, "\"ratio-limit\": %s, \n", nvram_safe_get( "bt_ratio" ) );
    fprintf( fp, "\"idle-seeding-limit-enabled\": %s, \n", pr );
    fprintf( fp, "\"idle-seeding-limit\": %s, \n", nvram_safe_get( "bt_ratio_idle" ) );
    fprintf( fp, "\"blocklist-enabled\": %s, \n", pm );
    fprintf( fp, "\"blocklist-url\": \"%s\", \n", nvram_safe_get( "bt_blocklist_url" ) );
    fprintf( fp, "\"download-queue-enabled\": %s, \n", pt );
    fprintf( fp, "\"download-queue-size\": %s, \n", nvram_safe_get( "bt_dl_queue_size" ) );
    fprintf( fp, "\"seed-queue-enabled\": %s, \n", pu );
    fprintf( fp, "\"seed-queue-size\": %s, \n", nvram_safe_get( "bt_ul_queue_size" ) );
    fprintf( fp, "\"message-level\": %s, \n", nvram_safe_get( "bt_message" ) );
    fprintf( fp, "%s\n", nvram_safe_get("bt_custom"));
    fprintf( fp, "\"rpc-authentication-required\": %s \n", pl );
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
    fprintf( fp, "sleep %s\n", nvram_safe_get( "bt_sleep" ) );

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

    fprintf( fp, "rm %s/.settings/blocklists/*\n", pk );

    if ( nvram_match( "bt_blocklist", "1") )
    {
        fprintf( fp, "wget %s -O %s/.settings/blocklists/level1.gz\n", nvram_safe_get( "bt_blocklist_url" ), pk );
        fprintf( fp, "gunzip %s/.settings/blocklists/level1.gz\n", pk );
    }

//crash fix?
    fprintf( fp, "EVENT_NOEPOLL=1; export EVENT_NOEPOLL\n" );
//
    if ( nvram_match( "bt_log", "1") )
    {
    fprintf( fp, "%s/transmission-daemon -g %s/.settings -e %s/transmission.log\n", pn, pk, nvram_safe_get( "bt_log_path" ) );
    } else {
    fprintf( fp, "%s/transmission-daemon -g %s/.settings\n", pn, pk );
    }
    fprintf( fp, "logger \"Transmission daemon successfully started\" \n");
    fprintf( fp, "sleep 2\n" );


    fprintf( fp, "/usr/bin/btcheck addcru\n");

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
    fprintf( fp, "COUNT=0\n");
    fprintf( fp, "TIMEOUT=10\n");
    fprintf( fp, "SLEEP=1\n");
    fprintf( fp, "logger \"Terminating transmission-daemon...\" \n");
    fprintf( fp, "killall transmission-daemon\n");
    fprintf( fp, "while [ `pidof transmission-daemon | awk '{print $1}'` ]; do\n");
    fprintf( fp, "sleep $SLEEP\n");
    fprintf( fp, "COUNT=$(($COUNT + $SLEEP))\n");
    fprintf( fp, "if [ $COUNT -ge $TIMEOUT ]; then\n");
    fprintf( fp, "logger \"Killing transmission-daemon...\" \n");
    fprintf( fp, "killall -KILL transmission-daemon\n");
    fprintf( fp, "fi\n");
    fprintf( fp, "done\n");
    fprintf( fp, "if [ $COUNT -lt $TIMEOUT ]; then\n");
    fprintf( fp, "logger \"Transmission daemon successfully stopped.\" \n");
    fprintf( fp, "else\n");
    fprintf( fp, "logger \"Transmission daemon forcefully stopped.\" \n");
    fprintf( fp, "fi\n");
    fprintf( fp, "/usr/bin/btcheck addcru\n");
    fprintf( fp, "exit 0\n");

    fclose( fp );
    chmod( "/tmp/stop_transmission.sh", 0755 );

    xstart( "/tmp/stop_transmission.sh" );
    return;
}
