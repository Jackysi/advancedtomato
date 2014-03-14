/*
 * tomatoanon.c
 *
 * Copyright (C) 2012 shibby
 *
 */
#include <rc.h>
#include <sys/stat.h>

void start_tomatoanon(void)
{

//  only if enable...
    if (!nvram_match("tomatoanon_enable", "1")) return;
    if (!nvram_match("tomatoanon_answer", "1")) return;

    xstart( "tomatoanon" );

    if (nvram_match("tomatoanon_notify", "1"))
         xstart( "tomatoanon", "checkver" );

    return;
}

void stop_tomatoanon(void)
{
    xstart( "cru", "d", "anonupdate" );
    xstart( "cru", "d", "checkver" );

    return;
}
