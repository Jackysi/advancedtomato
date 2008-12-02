
/*
 *********************************************************
 *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
 *********************************************************

 This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 the contents of this file may not be disclosed to third parties,
 copied or duplicated in any form without the prior written
 permission of CyberTAN Inc.

 This software should be used as a reference only, and it not
 intended for production use!


 THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/

/* milli_httpd - pretty small HTTP server
** A combination of
** micro_httpd - really small HTTP server
** and
** mini_httpd - small HTTP server
**
** Copyright © 1999,2000 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdarg.h>
#include <syslog.h>

#include <cy_conf.h>
#include "httpd.h"
#include <bcmnvram.h>
#include <code_pattern.h>
#include <utils.h>
#include <shutils.h>
#if defined(HTTPS_SUPPORT)
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif
#ifdef HSIAB_SUPPORT
#include <hsiabutils.h>
#define MAX_BUF_LEN    254
#endif

#include <error.h>
#include <sys/signal.h>

#define SERVER_NAME "httpd"
//#define SERVER_PORT 80
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define TIMEOUT	1

/* A multi-family sockaddr. */
typedef union {
    struct sockaddr sa;
    struct sockaddr_in sa_in;
    } usockaddr;

/* Globals. */
#if defined(HTTPS_SUPPORT) // jimmy, https, 8/4/2003
 static SSL * ssl;
#define DEFAULT_HTTPS_PORT 443
#define CERT_FILE "/tmp/cert.pem"
#define KEY_FILE "/tmp/key.pem"
//#define DEBUG_CIPHER
#endif
#ifdef DEBUG_CIPHER
char *set_ciphers = NULL;
int get_ciphers = 0;
#endif
#define DEFAULT_HTTP_PORT 80
extern int do_ssl;
int server_port;
char pid_file[80];
#ifdef SAMBA_SUPPORT
extern int smb_getlock;
extern int httpd_fork; 
void smb_handler()
{
	printf("============child exit=====waitting ...====\n");
	smb_getlock=0;
	wait(NULL);
	printf("wait finish \n"); 
}	
#endif
//static FILE *conn_fp;
static webs_t conn_fp; // jimmy, https, 8/4/2003
static char auth_userid[AUTH_MAX];
static char auth_passwd[AUTH_MAX];
static char auth_realm[AUTH_MAX];
#ifdef MULTIPLE_LOGIN_SUPPORT
static char auth_id_pw[AUTH_MAX];
static char auth_mod[AUTH_MAX];
#endif
#ifdef GET_POST_SUPPORT
int post;
#endif
int auth_fail = 0;
int httpd_level;

char http_client_ip[20];
extern char *get_mac_from_ip(char *ip);

/* Forwards. */
static int initialize_listen_socket( usockaddr* usaP );
static int auth_check( char* dirname, char* authorization );
static void send_authenticate( char* realm );
static void send_error( int status, char* title, char* extra_header, char* text );
static void send_headers( int status, char* title, char* extra_header, char* mime_type );
static int b64_decode( const char* str, unsigned char* space, int size );
static int match( const char* pattern, const char* string );
static int match_one( const char* pattern, int patternlen, const char* string );
static void handle_request(void);

static int
initialize_listen_socket( usockaddr* usaP )
    {
    int listen_fd;
    int i;

    memset( usaP, 0, sizeof(usockaddr) );
    usaP->sa.sa_family = AF_INET;
    usaP->sa_in.sin_addr.s_addr = htonl( INADDR_ANY );
    usaP->sa_in.sin_port = htons( server_port );

    listen_fd = socket( usaP->sa.sa_family, SOCK_STREAM, 0 );
    if ( listen_fd < 0 )
	{
	perror( "socket" );
	return -1;
	}
    (void) fcntl( listen_fd, F_SETFD, 1 );
    i = 1;
    if ( setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, (char*) &i, sizeof(i) ) < 0 )
	{
	perror( "setsockopt" );
	return -1;
	}
    if ( bind( listen_fd, &usaP->sa, sizeof(struct sockaddr_in) ) < 0 )
	{
	perror( "bind" );
	return -1;
	}
    if ( listen( listen_fd, 1024 ) < 0 )
	{
	perror( "listen" );
	return -1;
	}
    return listen_fd;
    }


static int
auth_check( char* dirname, char* authorization )
    {
    char authinfo[500];
    char* authpass;
    int l;
#ifdef MULTIPLE_LOGIN_SUPPORT
    int count;
    char word[1024], *next,*accounts;
    char delim[] = "<&nbsp;>";
#endif
    /* Is this directory unprotected? */
    if ( !strlen(auth_passwd) )
	/* Yes, let the request go through. */
	return 1;

    /* Basic authorization info? */
    if ( !authorization || strncmp( authorization, "Basic ", 6 ) != 0 ) {
	//send_authenticate( dirname );
	ct_syslog(LOG_INFO, httpd_level, "Authentication fail");
	return 0;
    }

    /* Decode it. */
    l = b64_decode( &(authorization[6]), authinfo, sizeof(authinfo) );
    authinfo[l] = '\0';
    /* Split into user and password. */
    authpass = strchr( authinfo, ':' );
    if ( authpass == (char*) 0 ) {
	/* No colon?  Bogus auth info. */
	//send_authenticate( dirname );
	return 0;
    }
    *authpass++ = '\0';

    /* Is this the right user and password? */
#ifdef DDM_SUPPORT
 #ifdef MULTIPLE_LOGIN_SUPPORT
	count =0;
	accounts = nvram_safe_get("http_login");
	split(word, accounts, next, delim) {
		int len = 0;
		char *name,*psw,*mod;

    		memset(auth_passwd,0,sizeof(auth_passwd));
    		memset(auth_mod,0,sizeof(auth_mod));
    		memset(auth_userid,0,sizeof(auth_userid));

		if ((name=strstr(word, "$NAME:")) == NULL ||
		    (psw=strstr(word, "$PSW:")) == NULL ||
		    (mod=strstr(word, "$MOD:")) == NULL)
		   	 continue;

		/* $NAME */
		if (sscanf(name, "$NAME:%3d:", &len) != 1)
		    continue;
		strncpy(auth_userid, name + sizeof("$NAME:nnn:") - 1, len);
                        name[len] = '\0';

		/* $PSW */
		if (sscanf(psw, "$PSW:%3d:", &len) != 1)
		    continue;
		strncpy(auth_passwd, psw + sizeof("$PSW:nnn:") - 1, len);
			psw[len] = '\0';

		/* $MOD */
		if (sscanf(mod, "$MOD:%3d:", &len) != 1)
		    continue;
		strncpy(auth_mod, mod + sizeof("$MOD:nnn:") - 1, len);
			mod[len] = '\0';

//		printf("accounts[%d]= name:%s, psw:%s, mod:%s  system= id:%s, psw:%s\n", count, auth_userid, auth_passwd, auth_mod, authinfo, authpass);
		count++;

    		if (strcmp( auth_userid, authinfo ) == 0 && strcmp( auth_passwd, authpass ) == 0 )
		{
			nvram_set("current_login_name",auth_userid);
			return 1;
		}
	}
 #else
    if ( strcmp( auth_userid, authinfo ) == 0 && strcmp( auth_passwd, authpass ) == 0 )
    {
	    return 1;
    }
 #endif
#else
    if (strcmp( auth_passwd, authpass ) == 0 )
	return 1;
#endif
    //send_authenticate( dirname );
    return 0;
    }


static void
send_authenticate( char* realm )
    {
    char header[10000];

    (void) snprintf(
	header, sizeof(header), "WWW-Authenticate: Basic realm=\"%s\"", realm );
    send_error( 401, "Unauthorized", header, "Authorization required." );
    }


static void
send_error( int status, char* title, char* extra_header, char* text )
    {

    // jimmy, https, 8/4/2003, fprintf -> wfprintf, fflush -> wfflush
    send_headers( status, title, extra_header, "text/html" );
    (void) wfprintf( conn_fp, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\n<BODY BGCOLOR=\"#cc9999\"><H4>%d %s</H4>\n", status, title, status, title );
    (void) wfprintf( conn_fp, "%s\n", text );
    (void) wfprintf( conn_fp, "</BODY></HTML>\n" );
    (void) wfflush( conn_fp );
    } 

static void
send_headers( int status, char* title, char* extra_header, char* mime_type )
    {
    time_t now;
    char timebuf[100];

    // jimmy, https, 8/4/2003, fprintf -> wfprintf
    wfprintf( conn_fp, "%s %d %s\r\n", PROTOCOL, status, title );
    wfprintf( conn_fp, "Server: %s\r\n", SERVER_NAME );
    now = time( (time_t*) 0 );
    strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &now ) );
    wfprintf( conn_fp, "Date: %s\r\n", timebuf );
    if ( extra_header != (char*) 0 && *extra_header)
	wfprintf( conn_fp, "%s\r\n", extra_header );
    if ( mime_type != (char*) 0 )
	wfprintf( conn_fp, "Content-Type: %s\r\n", mime_type );
    wfprintf( conn_fp, "Connection: close\r\n" );
    wfprintf( conn_fp, "\r\n" );
    }


/* Base-64 decoding.  This represents binary data as printable ASCII
** characters.  Three 8-bit binary bytes are turned into four 6-bit
** values, like so:
**
**   [11111111]  [22222222]  [33333333]
**
**   [111111] [112222] [222233] [333333]
**
** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
*/

static int b64_decode_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
    };

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
** Return the actual number of bytes generated.  The decoded size will
** be at most 3/4 the size of the encoded, and may be smaller if there
** are padding characters (blanks, newlines).
*/
static int
b64_decode( const char* str, unsigned char* space, int size )
    {
    const char* cp;
    int space_idx, phase;
    int d, prev_d=0;
    unsigned char c;

    space_idx = 0;
    phase = 0;
    for ( cp = str; *cp != '\0'; ++cp )
	{
	d = b64_decode_table[(int)*cp];
	if ( d != -1 )
	    {
	    switch ( phase )
		{
		case 0:
		++phase;
		break;
		case 1:
		c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
		if ( space_idx < size )
		    space[space_idx++] = c;
		++phase;
		break;
		case 2:
		c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
		if ( space_idx < size )
		    space[space_idx++] = c;
		++phase;
		break;
		case 3:
		c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
		if ( space_idx < size )
		    space[space_idx++] = c;
		phase = 0;
		break;
		}
	    prev_d = d;
	    }
	}
    return space_idx;
    }


/* Simple shell-style filename matcher.  Only does ? * and **, and multiple
** patterns separated by |.  Returns 1 or 0.
*/
int
match( const char* pattern, const char* string )
    {
    const char* or;

    //Added by Daniel(2004-07-29) for DEBUG	
    //cprintf("\nIn match(), pattern = %s, file = %s\n",pattern, string);	

    for (;;)
	{
	or = strchr( pattern, '|' );
	if ( or == (char*) 0 )
	    return match_one( pattern, strlen( pattern ), string );
	if ( match_one( pattern, or - pattern, string ) )
	    return 1;
	pattern = or + 1;
	}
    }


static int
match_one( const char* pattern, int patternlen, const char* string )
    {
    const char* p;

    for ( p = pattern; p - pattern < patternlen; ++p, ++string )
	{
	if ( *p == '?' && *string != '\0' )
	    continue;
	if ( *p == '*' )
	    {
	    int i, pl;
	    ++p;
	    if ( *p == '*' )
		{
		/* Double-wildcard matches anything. */
		++p;
		i = strlen( string );
		}
	    else
		/* Single-wildcard matches anything but slash. */
		i = strcspn( string, "/" );
	    pl = patternlen - ( p - pattern );
	    for ( ; i >= 0; --i )
		if ( match_one( p, pl, &(string[i]) ) )
		    return 1;
	    return 0;
	    }
	if ( *p != *string )
	    return 0;
	}
    if ( *string == '\0' )
	return 1;
    return 0;
    }


void

//do_file(char *path, FILE *stream)
do_file(char *path, webs_t stream) //jimmy, https, 8/4/2003
{
	FILE *fp;
	int c;
	
	if (!(fp = fopen(path, "r")))
		return;
	while ((c = getc(fp)) != EOF) 
		//fputc(c, stream);
		wfputc(c, stream); // jimmy, https, 8/4/2003
	fclose(fp);
}

#ifdef HSIAB_SUPPORT
static char * // add by jimmy 2003-5-13
get_aaa_url(int inout_mode, char *client_ip)
{
	static char line[MAX_BUF_LEN];
        char cmd[MAX_BUF_LEN];

        strcpy(line,"");
	if (inout_mode == 0)
     		snprintf(cmd, sizeof(cmd),"GET aaa_login_url %s", client_ip);
	else
     		snprintf(cmd, sizeof(cmd),"GET aaa_logout_url %s", client_ip);
		
        send_command(cmd, line);
					
        return line;
}

char *    // add by honor 2003-04-16, modify by jimmy 2003-05-13
get_client_ip(int conn_fp)
{
        struct sockaddr_in sa;
        int len = sizeof(struct sockaddr_in);
	static char ip[20];

        getpeername(conn_fp, (struct sockaddr *)&sa, &len);
	strcpy(ip,inet_ntoa(sa.sin_addr));
	return(ip);
}
#endif

static int
check_connect_type(void) 
{
	struct wl_assoc_mac *wlmac = NULL;
	int count_wl = 0;
	int i;

	if(nvram_invmatch("web_wl_filter", "1"))
		return 0;

	wlmac = get_wl_assoc_mac(&count_wl);
	
	for(i=0 ; i<count_wl ; i++) {
		if(!strcmp(wlmac[i].mac, nvram_safe_get("http_client_mac"))) {
			cprintf("Cann't accept wireless access\n");
			return -1;
		}
	}
	
	return 0;
}


#ifdef GOOGLE_SUPPORT
static void
add_google_pass_mac(void)
{
	char *google_mac = nvram_safe_get("google_pass_mac");
	char *mac = nvram_safe_get("http_client_mac");
	char buf[1024];

	if(!strstr(google_mac, mac)) {
		snprintf(buf, sizeof(buf), "%s %s", google_mac, mac);
		nvram_set("google_pass_mac", buf);

		nvram_commit();
	}

	snprintf(buf, sizeof(buf), "iptables -I google -i %s -p tcp -m mac --mac-source %s -j ACCEPT", 
		nvram_safe_get("lan_ifname"), nvram_safe_get("http_client_mac"));	
	cprintf("buf[%s]\n", buf);
	system(buf);
	snprintf(buf, sizeof(buf), "iptables -t nat -I google -i %s -p tcp -m mac --mac-source %s -j ACCEPT", 
		nvram_safe_get("lan_ifname"), nvram_safe_get("http_client_mac"));	
	cprintf("buf[%s]\n", buf);
	system(buf);
}
#endif

static void
handle_request(void)
{
    char line[10000], *cur;
    char *method, *path, *protocol, *authorization, *boundary;
    char *cp;
    char *file;
#ifdef MULTIPLE_LOGIN_SUPPORT
    char *file_ext;
#endif
    int len;
    struct mime_handler *handler;
    int cl = 0, flags;
    int auth_no = 0;
    char *http_host = NULL;
    /* Initialize the request variables. */
    authorization = boundary = NULL;
    bzero( line, sizeof line );

    /* Parse the first line of the request. */
    if ( wfgets( line, sizeof(line), conn_fp ) == (char *)0 ) {	//jimmy,https,8/4/2003
	    send_error( 400, "Bad Request", (char*) 0, "No request found." );
	    return;
    }

    /* To prevent http receive https packets, cause http crash (by honor 2003/09/02) */	
    if ( strncasecmp(line, "GET", 3) && strncasecmp(line, "POST", 4)) {
	fprintf(stderr, "Bad Request!\n");
	return;
    }

    method = path = line;
    strsep(&path, " ");
    if (!path) {	// Avoid http server crash, added by honor 2003-12-08
	send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );
	return;
    }
    while (*path == ' ') path++;
    protocol = path;
    strsep(&protocol, " ");
    if (!protocol) {	// Avoid http server crash, added by honor 2003-12-08
	send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );
	return;
    }
    while (*protocol == ' ') protocol++;
    cp = protocol;
    strsep(&cp, " ");
    cur = protocol + strlen(protocol) + 1;
    
    /* Parse the rest of the request headers. */
    //while ( fgets( cur, line + sizeof(line) - cur, conn_fp ) != (char*) 0 )
    while ( wfgets( cur, line + sizeof(line) - cur, conn_fp ) != 0 ) //jimmy,https,8/4/2003
    {
		
	if ( strcmp( cur, "\n" ) == 0 || strcmp( cur, "\r\n" ) == 0 ){
	    break;
	}
	else if ( strncasecmp( cur, "Authorization:", 14 ) == 0 )
	    {
	    cp = &cur[14];
	    cp += strspn( cp, " \t" );
	    authorization = cp;
	    cur = cp + strlen(cp) + 1;
	    }
	else if (strncasecmp( cur, "Content-Length:", 15 ) == 0) {
		cp = &cur[15];
		cp += strspn( cp, " \t" );
		cl = strtoul( cp, NULL, 0 );
	}
	else if ((cp = strstr( cur, "boundary=" ))) {
            boundary = &cp[9];
	    for( cp = cp + 9; *cp && *cp != '\r' && *cp != '\n'; cp++ );
	    *cp = '\0';
	    cur = ++cp;
	}
	else if (strncasecmp(cur, "Host:" , 5 ) == 0) {
            cp = &cur[5 + 1];
            cp += strspn( cp, " \t" );
            http_host = cp;
            cur = cp + strlen(cp) + 1;
            chomp(http_host);
        }

	}
    
#ifdef HSIAB_SUPPORT    
    if(strstr(path,"/logout/")){
	char *logout_url; 
	char buf[MAX_BUF_LEN];
	char value[MAX_BUF_LEN];
	
    	snprintf(value, sizeof(value), "LOG aaa_redirect logout %s %s", nvram_safe_get("http_client_ip"), nvram_safe_get("http_client_mac"));
        send_command(value, NULL);
	
	logout_url = get_aaa_url(1, nvram_safe_get("http_client_ip"));
	if(!strcmp(logout_url, "fail")){
		printf("Get logout URL fail!\n");
		return;
	}
        fprintf(stderr, "User will redirect to this URL...[%s]\n", logout_url);
        sprintf(buf,"Location: %s",logout_url);
        (void) send_headers(302,"Redirect",(char *)buf,"text/plain");
	         return;
				 
    }
#endif

    if ( strcasecmp( method, "get" ) != 0 && strcasecmp(method, "post") != 0 ) {
	send_error( 501, "Not Implemented", (char*) 0, "That method is not implemented." );
	return;
    }
    if ( path[0] != '/' ) {
	send_error( 400, "Bad Request", (char*) 0, "Bad filename." );
	return;
    }
    file = &(path[1]);
    len = strlen( file );
    //Added by Daniel(2004-07-29) for DEBUG in EZC	
    //cprintf("\nIn handle_request() file: [%s] \n", file);
    if ( file[0] == '/' || strcmp( file, ".." ) == 0 || strncmp( file, "../", 3 ) == 0 || strstr( file, "/../" ) != (char*) 0 || strcmp( &(file[len-3]), "/.." ) == 0 ) {
	send_error( 400, "Bad Request", (char*) 0, "Illegal filename." );
	return;
    }
    if ( strcmp( &(file[len-1]), "/" ) == 0 && len > 0)  //Amin 931231 deny request "http://ip/*/" return index.asp
    {
        send_error( 400, "Bad Request", (char*) 0, "Illegal filename." );
        return;
    }

#ifdef MULTIPLE_LOGIN_SUPPORT
    if ( file[0] == '\0' || file[len-1] == '/' )
    {
	file = "index.asp";
    }
#else
    if ( file[0] == '\0' || file[len-1] == '/' )
    {
 #ifdef MUST_CHANGE_PWD_SUPPORT
	if(!nvram_match("is_not_first_access","1"))
	  		file = "First_Management.asp";
	else
 #endif
		file = "index.asp";
    }
#endif
#ifdef GOOGLE_SUPPORT
	if (http_host &&
	    strncasecmp( http_host, nvram_safe_get("lan_ipaddr"), strlen(nvram_safe_get("lan_ipaddr"))) &&
	    strncasecmp( http_host, nvram_safe_get("wan_ipaddr"), strlen(nvram_safe_get("wan_ipaddr")))) {
		/* Redirect packet for first time */	
		file = "google_redirect1.asp";
		auth_no = 1;
	}
	else if (http_host &&
	        !strncasecmp( http_host, nvram_safe_get("lan_ipaddr"), strlen(nvram_safe_get("lan_ipaddr"))) &&
		strstr(path, "accept_terms=I+Accept")) {
		/* Redirect packet for accepting terms */	
			file = "google_redirect2.asp";
			auth_no = 1;
			add_google_pass_mac();
	}
#endif

    for (handler = &mime_handlers[0]; handler->pattern; handler++) {
	    if (match(handler->pattern, file)) {
		    if (!auth_no && handler->auth) {
#ifdef MULTIPLE_LOGIN_SUPPORT
			    handler->auth(auth_id_pw, auth_passwd, auth_realm);
#else
			    handler->auth(auth_userid, auth_passwd, auth_realm); //r
#endif
			    auth_fail = 0;
			    if (!auth_check(auth_realm, authorization)) {
				    send_authenticate(auth_realm);
				    return;
				    //auth_fail = 1;
			    }
				   
		    }
#ifdef MULTIPLE_LOGIN_SUPPORT
    file_ext = strstr(file , ".");
//	printf("The request file name is : [%s], extern name is : [%s]\n",file,file_ext);
    if(!strcmp(file_ext,".asp") && strcmp(file, "Status_Lan2.asp") && strcmp(file, "Status_Wireless2.asp") && strcmp(file, "Status_Router2.asp") && strcmp(file, "DHCPTable2.asp") && strcmp(file, "Management_account.asp"))
    {
	if(strcmp(auth_mod, "admin"))
    	{
		if(!strcmp(file, "Management.asp"))
			auth_fail = 1;
		else
		{
//    			printf("change web url to Status_Router page\n");
    			file = "Status_Router2.asp";
		}
    	}
//    	else
//		printf("Not change web url to Status_Router page\n");
    }
#endif		    
#ifdef GET_POST_SUPPORT
		    post=0;
		    if (strcasecmp(method, "post") == 0 ) {
			post=1;
                    }			    
#else
		    if (strcasecmp(method, "post") == 0 && !handler->input) {
			    send_error(501, "Not Implemented", NULL, "That method is not implemented.");
			    return;
		    }			    
#endif
		    if (handler->input) {
			    //cprintf("\nIn Handle Request(INPUT)!!!\n");
			    handler->input(file, conn_fp, cl, boundary);
#if defined(linux)
			    if ((flags = fcntl(fileno(conn_fp), F_GETFL)) != -1 &&
				fcntl(fileno(conn_fp), F_SETFL, flags | O_NONBLOCK) != -1) {
				    /* Read up to two more characters */
				    if (fgetc(conn_fp) != EOF)
					    (void) fgetc(conn_fp);
				    fcntl(fileno(conn_fp), F_SETFL, flags);
			    }
#endif				
		    }

		    if(check_connect_type() <0 ){
			    send_error(401, "Bad Request", (char*) 0, "Cann't use wireless interface to access web.");
			    return;
	            }
		    
		    if(auth_fail == 1){
			    send_authenticate(auth_realm);
			    return;
		    }
		    else    
			    send_headers( 200, "Ok", handler->extra_header, handler->mime_type );

		    if (handler->output)
		    {	
			    //cprintf("\nIn Handle Request(OUTPUT)!!!\n");
			    handler->output(file, conn_fp);
		    }			    
		    break;
	    }
    }

    if (!handler->pattern) 
	    send_error( 404, "Not Found", (char*) 0, "File not found." );
}

void    // add by honor 2003-04-16
get_client_ip_mac(int conn_fp)
{
        struct sockaddr_in sa;
        int len = sizeof(struct sockaddr_in);
        char *m;

        getpeername(conn_fp, (struct sockaddr *)&sa, &len);
	nvram_safe_set("http_client_ip", inet_ntoa(sa.sin_addr));
	m = get_mac_from_ip(inet_ntoa(sa.sin_addr));
	nvram_safe_set("http_client_mac", m);
}

static void
handle_server_sig_int(int sig)
{
	ct_syslog(LOG_INFO, httpd_level, "httpd server %sshutdown", do_ssl ? "(ssl support) " : "");
	exit(0);
}

void settimeouts(int sock, int secs)
{
	struct timeval tv;

	tv.tv_sec = secs;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
		perror("setsockopt(SO_SNDTIMEO)");
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		perror("setsockopt(SO_RCVTIMEO)");
}

int main(int argc, char **argv)
{
	usockaddr usa;
	int listen_fd;
	int conn_fd;
	socklen_t sz = sizeof(usa);
	int c;
	int timeout = TIMEOUT;
	int n = 0;
#if defined(HTTPS_SUPPORT) //jimmy, https, 8/4/2003
	BIO *sbio;
	SSL_CTX *ctx = NULL;
	int r;
	BIO *ssl_bio;
#endif
	strcpy(pid_file, "/var/run/httpd.pid");
	server_port = DEFAULT_HTTP_PORT;

	while ((c = getopt(argc, argv, "Sp:t:s:gh")) != -1)
                switch (c) {
#if defined(HTTPS_SUPPORT) //honor, https, 8/14/2003
                        case 'S':
				do_ssl = 1;
				server_port = DEFAULT_HTTPS_PORT;
				strcpy(pid_file, "/var/run/httpsd.pid");
				break;
#endif
			case 'p':
				server_port = atoi(optarg);
				break;
			case 't':
				timeout = atoi(optarg);
				break;
#ifdef DEBUG_CIPHER
			case 's':
				set_ciphers = optarg;
				break;
			case 'g':
				get_ciphers = 1;
				break;
#endif
			case 'h':
				fprintf(stderr, "Usage: %s [-S] [-p port]\n"
						"	-S : Support https (port 443)\n"
						"	-p port : Which port to listen?\n"
						"	-t secs : How many seconds to wait before timing out?\n"
						"	-s ciphers: set cipher lists\n"
						"	-g: get cipher lists\n"
						, argv[0]);
				exit(0);
				break;
			default:
				break;
		}
	
	
	httpd_level = ct_openlog("httpd", LOG_PID | LOG_NDELAY, LOG_DAEMON, "LOG_HTTPD");
	
	ct_syslog(LOG_INFO, httpd_level, "httpd server %sstarted at port %d\n", do_ssl ? "(ssl support) " : "", server_port);

        /* Ignore broken pipes */
        signal(SIGPIPE, SIG_IGN);
#ifdef SAMBA_SUPPORT	
	/* handle child exit */
	signal(SIGCHLD, smb_handler);    	
#endif
	signal(SIGTERM, handle_server_sig_int);	// kill

	
#if defined(HTTPS_SUPPORT) //jimmy, https, 8/4/2003	
        /* Build our SSL context */
	if(do_ssl){
		SSLeay_add_ssl_algorithms();
		SSL_load_error_strings();
		ctx = SSL_CTX_new( SSLv23_server_method() );
		if ( SSL_CTX_use_certificate_file( ctx, CERT_FILE, SSL_FILETYPE_PEM ) == 0 ) {
			cprintf("Cann't read %s\n", CERT_FILE);
			ERR_print_errors_fp( stderr );
			exit( 1 );

		}
		if(SSL_CTX_use_PrivateKey_file( ctx, KEY_FILE, SSL_FILETYPE_PEM ) == 0 ) {
			cprintf("Cann't read %s\n", KEY_FILE);
			ERR_print_errors_fp( stderr );
			exit( 1 );

		}
		if(SSL_CTX_check_private_key( ctx ) == 0 ) {
			cprintf("Check private key fail\n");
			ERR_print_errors_fp( stderr );
			exit( 1 );
		}
	}
#endif
	/* Initialize listen socket */
retry:
	if ((listen_fd = initialize_listen_socket(&usa)) < 0) {
		ct_syslog(LOG_ERR, httpd_level, "Cann't bind to any address");
		close(listen_fd);

		sleep(2);
		if(n++ <= 5) {
			cprintf("Something error occur, retry %d\n", n);
			goto retry;
		}

		exit(errno);
	}
	
#if !defined(DEBUG)
	{
	FILE *pid_fp;
	/* Daemonize and log PID */
	if (daemon(1, 1) == -1) {
		perror("daemon");
		exit(errno);
	}
	if (!(pid_fp = fopen(pid_file, "w"))) {
		perror(pid_file);
		return errno;
	}
	fprintf(pid_fp, "%d", getpid());
	fclose(pid_fp);
	}
#endif

	/* Loop forever handling requests */
	for (;;) {
		if ((conn_fd = accept(listen_fd, &usa.sa, &sz)) < 0) {
			perror("accept");
			return errno;
		}

		/* Make sure we don't linger a long time if the other end disappears */
		settimeouts(conn_fd, timeout);		

		if(check_action() == ACT_TFTP_UPGRADE ||		// We don't want user to use web during tftp upgrade.
		   check_action() == ACT_SW_RESTORE ||
		   check_action() == ACT_HW_RESTORE){
			fprintf(stderr, "http(s)d: nothing to do...\n");
			return -1;
		}
		
#if defined(HTTPS_SUPPORT) //jimmy, https, 8/4/2003
	    if(do_ssl){
		if(check_action() == ACT_WEB_UPGRADE){	// We don't want user to use web (https) during web (http) upgrade.
			fprintf(stderr, "httpsd: nothing to do...\n");
			return -1;
		}

                sbio=BIO_new_socket(conn_fd,BIO_NOCLOSE);
                ssl=SSL_new(ctx);
		
#ifdef DEBUG_CIPHER
		check_cipher();
#endif

                SSL_set_bio(ssl,sbio,sbio);

                if((r=SSL_accept(ssl)<=0)){
	                        //berr_exit("SSL accept error");
				ct_syslog(LOG_ERR, httpd_level, "SSL accept error");
				close(conn_fd);
				continue;
		}

                conn_fp=(webs_t)BIO_new(BIO_f_buffer());
                ssl_bio=BIO_new(BIO_f_ssl());
                BIO_set_ssl(ssl_bio,ssl,BIO_CLOSE);
                BIO_push((BIO *)conn_fp,ssl_bio);
	    }else
#endif
	    {
		if(check_action() == ACT_WEBS_UPGRADE){	// We don't want user to use web (http) during web (https) upgrade.
			fprintf(stderr, "httpd: nothing to do...\n");
			return -1;
		}
	    
		if (!(conn_fp = fdopen(conn_fd, "r+"))) {
			perror("fdopen");
			return errno;
		}
	    }
		get_client_ip_mac(conn_fd);
		
		handle_request();
#ifdef SAMBA_SUPPORT
                if (httpd_fork==1) {
                        httpd_fork=0;
                        close(conn_fd);
                        continue;
                }
#endif		

		wfflush(conn_fp); // jimmy, https, 8/4/2003
		wfclose(conn_fp); // jimmy, https, 8/4/2003
		close(conn_fd);
	}

	shutdown(listen_fd, 2);
	close(listen_fd);

	return 0;
}

char *
wfgets(char *buf, int len, FILE *fp)
{
#ifdef HTTPS_SUPPORT
	if(do_ssl)
		return  (char *)BIO_gets((BIO *)fp, buf, len);
	else
#endif
		return  fgets(buf, len, fp);
}

int
wfputc(char c, FILE *fp)
{
#ifdef HTTPS_SUPPORT
	if(do_ssl)
		return BIO_printf((BIO *)fp, "%c", c);
	else
#endif
		return fputc(c, fp);
}

int
wfputs(char *buf, FILE *fp)
{
#ifdef HTTPS_SUPPORT
	if(do_ssl)
		return BIO_puts((BIO *)fp, buf);
	else
#endif
		return fputs(buf, fp);
}

int
wfprintf(FILE *fp, char *fmt,...)
{
	va_list args;
	char buf[1024];
	int ret;
	
	va_start(args, fmt);	
	vsnprintf(buf, sizeof(buf), fmt, args);
#ifdef HTTPS_SUPPORT
	if(do_ssl)
		ret = BIO_printf((BIO *)fp, "%s", buf);
	else
#endif
		ret = fprintf(fp, "%s", buf);	
        va_end(args);
	return ret;
}

size_t
wfwrite(char *buf, int size, int n, FILE *fp)
{
#ifdef HTTPS_SUPPORT
	if(do_ssl)
		return BIO_write((BIO *)fp, buf , n * size);
	else
#endif
		return fwrite(buf,size,n,fp);
}

size_t
wfread(char *buf, int size, int n, FILE *fp)
{
#ifdef HTTPS_SUPPORT
	if(do_ssl)
		return BIO_read((BIO *)fp, buf , n * size);
	else
#endif
		return fread(buf,size,n,fp);
}

int
wfflush(FILE *fp)
{
#ifdef HTTPS_SUPPORT
	if(do_ssl){
		BIO_flush((BIO *)fp);
		return 1;
	}
	else
#endif
		return fflush(fp);
}

int
wfclose(FILE *fp)
{
#ifdef HTTPS_SUPPORT
	if(do_ssl){
		BIO_free_all((BIO *)fp);
		return 1;
	}
	else
#endif
		return fclose(fp);
}

#ifdef DEBUG_CIPHER
void check_cipher(void)
{
	STACK_OF(SSL_CIPHER) *sk;
	char buf[512];
	BIO *STDout = NULL;
	int i;
	static BIO *bio_stdout=NULL;
	X509 *peer;
	static BIO *bio_s_out=NULL;
	SSL_CIPHER *ciph;

	if(set_ciphers) {
		/* Set supported cipher lists */
		SSL_set_cipher_list(ssl, set_ciphers);
	}
	if(get_ciphers) {
		/* Show supported cipher lists */
		sk=SSL_get_ciphers(ssl);

		for (i=0; i<sk_SSL_CIPHER_num(sk); i++) {
			BIO_puts(STDout,SSL_CIPHER_description( sk_SSL_CIPHER_value(sk,i), buf, 512));
			printf("%d: %s", i, buf);
		}
		if (STDout != NULL) BIO_free_all(STDout);
	}
	peer=SSL_get_peer_certificate(ssl);
	if (peer != NULL)
	{
		BIO_printf(bio_s_out,"Client certificate\n");
		PEM_write_bio_X509(bio_s_out,peer);
		X509_NAME_oneline(X509_get_subject_name(peer),buf,sizeof buf);
		BIO_printf(bio_s_out,"subject=%s\n",buf);
		X509_NAME_oneline(X509_get_issuer_name(peer),buf,sizeof buf);
		BIO_printf(bio_s_out,"issuer=%s\n",buf);
		X509_free(peer);
	}

	if (SSL_get_shared_ciphers(ssl,buf,sizeof buf) != NULL)
		BIO_printf(bio_s_out,"Shared ciphers:%s\n",buf);
		
	bio_stdout=BIO_new_fp(stdout,BIO_NOCLOSE);
	ciph=SSL_get_current_cipher(ssl);
	BIO_printf(bio_stdout,"%s%s, cipher %s %s\n",
		"",
		SSL_get_version(ssl),
		SSL_CIPHER_get_version(ciph),
		SSL_CIPHER_get_name(ciph));
}
#endif
