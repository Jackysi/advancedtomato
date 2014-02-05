/*
 * nfs.c
 *
 * Copyright (C) 2011 shibby
 *
 */
#include <rc.h>
#include <sys/stat.h>

void start_nfs(void)
{
    FILE *fp;
    char *buf;
    char *g;
    char *p;
    int i;
    char *dir,*address,*access,*sync,*subtree,*other;

//  only if enable...
    if( nvram_match( "nfs_enable", "1" ) )
    {

//  read exports from nvram
    if ((buf = strdup(nvram_safe_get("nfs_exports"))) != NULL) {

//      writing data to file
        if( !( fp = fopen( "/etc/exports", "w" ) ) )
        {
            perror( "/etc/exports" );
            return;
        }

        g = buf;

//      dir < address < access < sync < subtree < other

        while ((p = strsep(&g, ">")) != NULL) {
            i = vstrsep(p, "<", &dir, &address, &access, &sync, &subtree, &other);
            if (i!=6) continue;
            fprintf(fp, "%s %s(%s,%s,%s,%s)\n", dir,address,access,sync,subtree,other);
        }
        free(buf);
    }

    fclose( fp );
    chmod( "/etc/exports", 0644 );

    }

    xstart( "/usr/sbin/nfs.rc" );
    return;

}

void stop_nfs(void)
{
    killall("portmap", SIGTERM);
    killall("statd", SIGTERM);
    killall("nfsd", SIGTERM);
    killall("mountd", SIGTERM);
    return;
}
