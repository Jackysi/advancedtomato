/*
 * ups.c
 *
 * Copyright (C) 2011 shibby
 *
 */
#include <rc.h>
#include <sys/stat.h>

void start_ups(void)
{
    FILE *fpm;
    FILE *fpc;

//  always copy and try start service if USB support is enable
//  if service will not find apc ups, then will turn off automaticaly
//    if( nvram_match( "ups_enable", "1" ) )
//    {

        eval("cp", "/www/apcupsd/tomatodata.cgi", "/www/ext/cgi-bin/tomatodata.cgi");
        eval("cp", "/www/apcupsd/tomatoups.cgi", "/www/ext/cgi-bin/tomatoups.cgi");

//      writing data to file

/* turn off at the moment. We will use this in the future.
        if( !( fpc = fopen( "/etc/apcupsd.conf", "w" ) ) )
        {
            perror( "/etc/apcupsdd.conf" );
            return;
        }
        fprintf( fpc, "USBCABLE usb \n");
        fprintf( fpc, "UPSTYPE usb \n");
        fprintf( fpc, "DEVICE \n");
        fprintf( fpc, "EVENTSFILE /var/log/apcupsd.events \n");

        fclose( fpc );
        chmod( "/etc/apcupsd.conf", 0644 );

        if( !( fpm = fopen( "/etc/apcemail.conf", "w" ) ) )
        {
            perror( "/etc/apcemail.conf" );
            return;
        }
        fprintf( fpm, "SYSADMIN=myemail@domain.com \n");
        fprintf( fpm, "APCUPSD_MAIL=\"sendmail -S server.home -f root\" \n");

        fclose(fpm);
        chmod( "/etc/apcemail.conf", 0644 );
*/
        xstart( "apcupsd" );
//    }

    return;
}

void stop_ups(void)
{
    killall("apcupsd", SIGTERM);
//    eval("rm", "/etc/apcupsd.conf");
//    eval("rm", "/etc/apcemail.conf");
//    eval("rm", "/www/ext/cgi-bin/tomatodata.cgi");
//    eval("rm", "/www/ext/cgi-bin/tomatoups.cgi");
    return;
}
