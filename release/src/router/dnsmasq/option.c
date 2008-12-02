/* dnsmasq is Copyright (c) 2000 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/* Author's email: simon@thekelleys.org.uk */

#include "dnsmasq.h"

struct myoption {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

#ifdef HAVE_GETOPT_LONG
static struct myoption opts[] = { 
  {"version", 0, 0, 'v'},
  {"no-hosts", 0, 0, 'h'},
  {"no-poll", 0, 0, 'n'},
  {"help", 0, 0, 'w'},
  {"no-daemon", 0, 0, 'd'},
  {"log-queries", 0, 0, 'q'},
  {"user", 1, 0, 'u'},
  {"resolv-file", 1, 0, 'r'},
  {"mx-host", 1, 0, 'm'},
  {"mx-target", 1, 0, 't'},
  {"cache-size", 1, 0, 'c'},
  {"port", 1, 0, 'p'},
  {"dhcp-lease", 1, 0, 'l'},
  {"domain-suffix", 1, 0, 's'},
  {"interface", 1, 0, 'i'},
  {"listen-address", 1, 0, 'a'},
  {"bogus-priv", 0, 0, 'b'},
  {"selfmx", 0, 0, 'e'},
  {"filterwin2k", 0, 0, 'f'},
  {"pid-file", 1, 0, 'x'},
  {"strict-order", 0, 0, 'o'},
  {"server", 1, 0, 'S'},
  {"conf-file", 1, 0, 'C'},
  {0, 0, 0, 0}
};
#endif

struct optflags {
  char c;
  unsigned int flag; 
};

static struct optflags optmap[] = {
  { 'b', OPT_BOGUSPRIV },
  { 'f', OPT_FILTER },
  { 'q', OPT_LOG },
  { 'e', OPT_SELFMX },
  { 'h', OPT_NO_HOSTS },
  { 'n', OPT_NO_POLL },
  { 'd', OPT_DEBUG },
  { 'o', OPT_ORDER },
  { 'v', 0},
  { 'w', 0},
  { 0, 0 }
};

static char *usage =
"Usage: dnsmasq [options]\n"
"\nValid options are :\n"
"-a, --listen-address=ipaddr Specify local address(es) to listen on.\n"
"-b, --bogus-priv            Fake reverse lookups for RFC1918 private address ranges.\n"
"-c, --cache-size=cachesize  Specify the size of the cache in entries (defaults to %d).\n"
#ifdef HAVE_FILE_SYSTEM
"-C, --conf-file=path        Specify configuration file (defaults to " CONFFILE ").\n"
#endif
"-d, --no-daemon             Do NOT fork into the background: run in debug mode.\n"
"-e, --selfmx                Return self-pointing MX records for local hosts.\n"
"-f, --filterwin2k           Don't forward spurious DNS requests from Windows hosts.\n"
#ifdef HAVE_FILE_SYSTEM
"-h, --no-hosts              Do NOT load " HOSTSFILE " file.\n"
#endif
"-i, --interface=interface   Specify interface(s) to listen on.\n"
#ifdef HAVE_FILE_SYSTEM
"-l, --dhcp-lease=path       Specify the path to the DHCP lease file.\n"
#endif
"-m, --mx-host=host_name     Specify the MX name to reply to.\n"
#ifdef HAVE_FILE_SYSTEM
"-n, --no-poll               Do NOT poll " RESOLVFILE " file, reload only on SIGHUP.\n"
"-o, --strict-order          Use nameservers strictly in the order given in " RESOLVFILE ".\n"
#endif
"-p, --port=number           Specify port to listen for DNS requests on (defaults to 53).\n"
"-q, --log-queries           Log queries.\n"
#ifdef HAVE_FILE_SYSTEM
"-r, --resolv-file=path      Specify path to resolv.conf (defaults to " RESOLVFILE ").\n"
#endif
"-S, --server=/domain/ipaddr Specify address(es) of upstream servers with optional domain.\n"
"-s, --domain-suffix=domain  Specify the domain suffix which DHCP entries will use.\n"
"-t, --mx-target=host_name   Specify the host in an MX reply.\n"
#if defined(CHUSER)
"-u, --user=username         Change to this user after startup. (defaults to " CHUSER ").\n" 
#endif
"-v, --version               Display dnsmasq version.\n"
"-w, --help                  Display this message.\n"
#ifdef HAVE_FILE_SYSTEM
"-x, --pid-file=path         Specify path of PID file. (defaults to " RUNFILE ").\n"
#endif
"\n";


unsigned int read_opts (int argc, char **argv, char *buff, char **resolv_file, 
			char **mxname, char **mxtarget, char **lease_file, 
			char **username, char **domain_suffix, char **runfile, 
			struct iname **if_names, struct iname **if_addrs,
			struct server **serv_addrs, int *cachesize, int *port, unsigned long *timetolive)
{
  int option = 0, i;
  unsigned int flags = 0;
#ifdef HAVE_FILE_SYSTEM
  FILE *f = NULL;
  char *conffile = CONFFILE;
  int conffile_set = 0;
#endif


  while (1)
    {
#ifdef HAVE_FILE_SYSTEM
      if (!f)
#endif
#ifdef HAVE_GETOPT_LONG
	option = getopt_long(argc, argv, "owefnbvhdqr:m:p:c:l:s:i:t:u:a:x:S:C:T:", 
			     (struct option *)opts, NULL);
#else
        option = getopt(argc, argv, "owefnbvhdqr:m:p:c:l:s:i:t:u:a:x:S:C:T:");
#endif
#ifdef HAVE_FILE_SYSTEM
      else
	{ /* f non-NULL, reading from conffile. */
	  if (!fgets(buff, MAXDNAME, f))
	    {
	      /* At end of file, all done */
	      fclose(f);
	      break;
	    }
	  else
	    {
	      char *p;
	      /* fgets gets end of line char too. */
	      while (strlen(buff) > 0 && 
		     (buff[strlen(buff)-1] == '\n' || 
		      buff[strlen(buff)-1] == ' ' || 
		      buff[strlen(buff)-1] == '\t'))
		buff[strlen(buff)-1] = 0;
	      if (*buff == '#' || *buff == 0)
		continue; /* comment */
	      if ((p=strchr(buff, '=')))
		{
		  optarg = p+1;
		  *p = 0;
		}
	      else
		optarg = NULL;
	      
	      option = 0;
	      for (i=0; opts[i].name; i++) 
		if (strcmp(opts[i].name, buff) == 0)
		  option = opts[i].val;
	      if (!option)
		die("Bad option in %s: %s", conffile);
	    }
	}
#endif
      
      if (option == -1)
	{ /* end of command line args, start reading conffile. */
#ifdef HAVE_FILE_SYSTEM
	  if (!conffile)
	    break; /* "confile=" option disables */
	  option = 0;
	  if (!(f = fopen(conffile, "r")))
	    {   
	      if (errno == ENOENT && !conffile_set)
		break; /* No conffile, all done. */
	      else
		die("Cannot read %s: %s", conffile);
	    }
#else
	    break;
#endif
	}
     
      if (
#ifdef HAVE_FILE_SYSTEM
      	!f && 
#endif
      	option == 'w')
	{
	  fprintf (stderr, usage,  CACHESIZ);
	  exit(0);
	}

      if (
#ifdef HAVE_FILE_SYSTEM
      	!f && 
#endif
      	option == 'v')
        {
          fprintf(stderr, "dnsmasq version %s\n", VERSION);
          exit(0);
        }
      
      for (i=0; optmap[i].c; i++)
	if (option == optmap[i].c)
	  {
	    flags |= optmap[i].flag;
	    option = 0;
	    if (
#ifdef HAVE_FILE_SYSTEM
	    	f && 
#endif
	    	optarg)
	      die("Extraneous parameter for %s in config file.", buff);
	    break;
	  }
      
      if (option && option != '?')
	{
	  if (
#ifdef HAVE_FILE_SYSTEM
	    	f && 
#endif
	  	!optarg)
	    die("Missing parameter for %s in config file.", buff);
	  
	  switch (option)
	    { 
#ifdef HAVE_FILE_SYSTEM
	     case 'C': 
	       conffile = safe_string_alloc(optarg);
	       conffile_set = 1;
	       break;
	      
	    case 'x': 
	      *runfile = safe_string_alloc(optarg);
	      break;
	      
	    case 'r':
	      *resolv_file = safe_string_alloc(optarg);
	      break;
	      
	    case 'l':
	      *lease_file = safe_string_alloc(optarg);
	      break;
#endif
	      
	    case 'm':
	      canonicalise(optarg);
	      *mxname = safe_string_alloc(optarg);
	      break;
	      
	    case 't':
	      canonicalise(optarg);
	      *mxtarget = safe_string_alloc(optarg);
	      break;
#if 1 //def MPPPOE_SUPPORT  
	    case 'T':
	      //*timetolive = safe_string_alloc(optarg);
	      *timetolive = (unsigned long)atoi(optarg);
	      break;
#endif
	    case 's':
	      canonicalise(optarg);
	      *domain_suffix = safe_string_alloc(optarg);
	      break;
	      
	    case 'u':
	      *username = safe_string_alloc(optarg);
	      break;
	      
	    case 'i':
	      {
		struct iname *new = safe_malloc(sizeof(struct iname));
		new->next = *if_names;
		*if_names = new;
		new->name = safe_string_alloc(optarg);
		new->found = 0;
		break;
	      }
	      
	    case 'a':
	      {
		struct iname *new = safe_malloc(sizeof(struct iname));
		new->next = *if_addrs;
		*if_addrs = new;
		new->found = 0;
		if (inet_pton(AF_INET, optarg, &new->addr.in.sin_addr))
		  new->addr.sa.sa_family = AF_INET;
#ifdef HAVE_IPV6
		else if (inet_pton(AF_INET6, optarg, &new->addr.in6.sin6_addr))
		  new->addr.sa.sa_family = AF_INET6;
#endif
		else
		  option = '?'; /* error */
		break;
	      }
	      
	    case 'S':
	      {
		struct server *new = safe_malloc(sizeof(struct server));
		char *end = strrchr(optarg, '/');
		new->next = *serv_addrs;
		*serv_addrs = new;
		new->from_resolv = 0;
		new->domain = NULL;
		if (*optarg == '/' && end)
		  {
		    *end = 0;
		    canonicalise(optarg+1);
		    new->domain = safe_string_alloc(optarg+1);
		    optarg = end+1;
		  }
		if (inet_pton(AF_INET, optarg, &new->addr.in.sin_addr))
		  {
#ifdef HAVE_SOCKADDR_SA_LEN
		    new->addr.in.sin_len = sizeof(struct sockaddr_in);
#endif
		    new->addr.sa.sa_family = AF_INET;
		    new->addr.in.sin_port = htons(NAMESERVER_PORT);
		  }
#ifdef HAVE_IPV6
		else if (inet_pton(AF_INET6, optarg, &new->addr.in6.sin6_addr))
		  {
#ifdef HAVE_SOCKADDR_SA_LEN
		    new->addr.in6.sin6_len = sizeof(struct sockaddr_in6);
#endif
		    new->addr.sa.sa_family = AF_INET6;
		    new->addr.in6.sin6_port = htons(NAMESERVER_PORT);
		    new->addr.in6.sin6_flowinfo = htonl(0);
		  }
#endif
		else
		  option = '?'; /* error */
		break;
	      }
	      
	    case 'c':
	      {
		int size = atoi(optarg);
		/* zero is OK, and means no caching. */
		
		if (size < 0)
		  size = 0;
		else if (size > 1000)
		  size = 1000;
		
		*cachesize = size;
		break;
	      }
	      
	    case 'p':
	      *port = atoi(optarg);
	      break;
	      
	    }
	}
      
      if (option == '?')
	die("bad command line options: try --help.", NULL);
	      
    }
      
  /* port might no be known when the address is parsed - fill in here */
  if (*if_addrs)
    {  
      struct iname *tmp;
      for(tmp = *if_addrs; tmp; tmp = tmp->next)
	if (tmp->addr.sa.sa_family == AF_INET)
	  {
#ifdef HAVE_SOCKADDR_SA_LEN
	    tmp->addr.in.sin_len = sizeof(struct sockaddr_in);
#endif
	    tmp->addr.in.sin_port = htons(*port);
	  }
#ifdef HAVE_IPV6
	else
	  { 
#ifdef HAVE_SOCKADDR_SA_LEN
	    tmp->addr.in6.sin6_len = sizeof(struct sockaddr_in6);
#endif
	    tmp->addr.in6.sin6_port = htons(*port);
	    tmp->addr.in6.sin6_flowinfo = htonl(0);
	  }
#endif /* IPv6 */
    }
		      
  /* only one of these need be specified: the other defaults to the
     host-name */
  if (*mxname || *mxtarget)
    {
      if (gethostname(buff, MAXDNAME) == -1)
	die("cannot get host-name: %s", NULL);
	      
      if (!*mxname)
	*mxname = safe_string_alloc(buff);
      
      if (!*mxtarget)
	*mxtarget = safe_string_alloc(buff);
    }
  
  return flags;
}
      
      

