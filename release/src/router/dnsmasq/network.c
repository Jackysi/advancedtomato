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

static struct irec *add_iface(struct irec *list, unsigned int flags, 
			      char *name, union mysockaddr *addr, 
			      struct iname *names, struct iname *addrs)
{
  struct irec *iface;

  /* we may need to check the whitelist */
  if (names)
    { 
      struct iname *tmp;
      for(tmp = names; tmp; tmp = tmp->next)
	if (strcmp(tmp->name, name) == 0)
	  {
	    tmp->found = 1;
	    break;
	  }
      if (!(flags & IFF_LOOPBACK) && !tmp) 
	/* not on whitelist and not loopback */
	return list;
    }
  
  if (addrs)
    { 
      struct iname *tmp;
      for(tmp = addrs; tmp; tmp = tmp->next)
	if (sockaddr_isequal(&tmp->addr, addr))
	  {
	    tmp->found = 1;
	    break;
	  }
      
      if (!tmp) 
	/* not on whitelist */
	return list;
    }
  
  /* check whether the interface IP has been added already 
     it is possible to have multiple interfaces with the same address. */
  for (iface = list; iface; iface = iface->next) 
    if (sockaddr_isequal(&iface->addr, addr))
      break;
  if (iface) 
    return list;
  
  /* If OK, add it to the head of the list */
  iface = safe_malloc(sizeof(struct irec));
  iface->addr = *addr;
  iface->next = list;
  return iface;
}

struct irec *find_all_interfaces(struct iname *names,
				 struct iname *addrs,
				 int port)
{
  /* this code is adapted from Stevens, page 434. It finally
     destroyed my faith in the C/unix API */
  int len = 100 * sizeof(struct ifreq);
  int lastlen = 0;
  char *buf, *ptr;
  struct ifconf ifc;
  struct irec *ret = NULL;
  int fd = socket(PF_INET, SOCK_DGRAM, 0);
  
  if (fd == -1)
    die("cannot create socket to enumerate interfaces: %s", NULL);
        
  while (1)
    {
      buf = safe_malloc(len);
      ifc.ifc_len = len;
      ifc.ifc_buf = buf;
      if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
	{
	  if (errno != EINVAL || lastlen != 0)
	    die("ioctl error while enumerating interfaces: %s", NULL);
	}
      else
	{
	  if (ifc.ifc_len == lastlen)
	    break; /* got a big enough buffer now */
	  lastlen = ifc.ifc_len;
	}
      len += 10* sizeof(struct ifreq);
      safe_free(buf);
    }

  for (ptr = buf; ptr < buf + ifc.ifc_len; )
    {
      struct ifreq *ifr = (struct ifreq *) ptr;
      union mysockaddr addr;
#ifdef HAVE_SOCKADDR_SA_LEN
      ptr += ifr->ifr_addr.sa_len + IF_NAMESIZE;
#else
      ptr += sizeof(struct ifreq);
#endif

      /* copy address since getting flags overwrites */
      if (ifr->ifr_addr.sa_family == AF_INET)
	{
	  addr.in = *((struct sockaddr_in *) &ifr->ifr_addr);
	  addr.in.sin_port = htons(port);
	  
	  if (ioctl(fd, SIOCGIFFLAGS, ifr) < 0)
	    die ("ioctl error getting interface flags: %s", NULL);
	      	  
	  ret = add_iface(ret, ifr->ifr_flags, ifr->ifr_name, &addr, 
			  names, addrs);
	}
#ifdef HAVE_IPV6
      else if (ifr->ifr_addr.sa_family == AF_INET6)
	{
#ifdef HAVE_BROKEN_SOCKADDR_IN6
	  addr.in6 = *((struct my_sockaddr_in6 *) &ifr->ifr_addr);
#else
	  addr.in6 = *((struct sockaddr_in6 *) &ifr->ifr_addr);
#endif
	  addr.in6.sin6_port = htons(port);
	  addr.in6.sin6_flowinfo = htonl(0);
	  
	  if (ioctl(fd, SIOCGIFFLAGS, ifr) < 0)
	    die("ioctl error getting interface flags: %s", NULL);
	    	  
	  ret = add_iface(ret, ifr->ifr_flags, ifr->ifr_name, &addr, 
			      names, addrs);
	}
#endif /* IPV6 */
	
    }
  safe_free(buf);
  close(fd);
  
#if defined(HAVE_LINUX_IPV6_PROC) && defined(HAVE_IPV6)
  /* IPv6 addresses don't seem to work with SIOCGIFCONF. Barf */
  /* This code snarfed from net-tools 1.60 and certainly linux specific, though
     it shouldn't break on other Unices, and their SIOGIFCONF might work. */
  {
    FILE *f = fopen(IP6INTERFACES, "r");
    
    if (f)
      {
	union mysockaddr addr;
	unsigned int plen, scope, flags, if_idx;
	char devname[20], addrstring[32];
	
	while (fscanf(f, "%32s %02x %02x %02x %02x %20s\n",
		      addrstring, &if_idx, &plen, &scope, &flags, devname) != EOF) 
	  {
	    int i;
	    unsigned char *addr6p = (unsigned char *) &addr.in6.sin6_addr;
	    memset(&addr, 0, sizeof(addr));
	    addr.sa.sa_family = AF_INET6;
	    for (i=0; i<16; i++)
	      {
		unsigned int byte;
		sscanf(addrstring+i+i, "%02x", &byte);
		addr6p[i] = byte;
	      }
#ifdef HAVE_SOCKADDR_SA_LEN 
	    /* For completeness - should never be defined on Linux. */
	    addr.in6.sin6_len = sizeof(struct sockaddr_in6);
#endif
	    addr.in6.sin6_port = htons(port);
	    addr.in6.sin6_flowinfo = htonl(0);
	    addr.in6.sin6_scope_id = htonl(scope);
	    
	    ret = add_iface(ret, flags, devname, &addr, names, addrs);
	  }
	
	fclose(f);
      }
  }
#endif /* LINUX */

  /* if a whitelist provided, make sure the if names on it were OK */
  while(names)
    {
      if (!names->found)
	die("unknown interface %s", names->name);
      
      names = names->next;
    }
   
  while(addrs)
    {
      if (!addrs->found)
	{
#ifdef HAVE_IPV6
	  char addrbuff[INET6_ADDRSTRLEN];
#else
	  char addrbuff[INET_ADDRSTRLEN];
#endif
	  if (addrs->addr.sa.sa_family == AF_INET)
	    inet_ntop(AF_INET, &addrs->addr.in.sin_addr,
		      addrbuff, INET_ADDRSTRLEN);
#ifdef HAVE_IPV6
	  else
	    inet_ntop(AF_INET6, &addrs->addr.in6.sin6_addr,
		      addrbuff, INET6_ADDRSTRLEN);
#endif
	  die("no interface with address %s", addrbuff);
	}
      addrs = addrs->next;
    }
    
  return ret;
}

struct server *check_servers(struct server *new,
			     struct irec *interfaces, int peerfd, int peerfd6)
{
#ifdef HAVE_IPV6
  char addrbuff[INET6_ADDRSTRLEN];
#else
  char addrbuff[INET_ADDRSTRLEN];
#endif
  struct server *ret = NULL;
  
  /* forward table rules reference servers, so have to blow them away */
  forward_init(0);
  
  while (new)
    {
      struct server *tmp = new->next;

      if (new->addr.sa.sa_family == AF_INET)
	inet_ntop(AF_INET, &new->addr.in.sin_addr, addrbuff, INET_ADDRSTRLEN);
#ifdef HAVE_IPV6
      else if (new->addr.sa.sa_family == AF_INET6)
	inet_ntop(AF_INET6, &new->addr.in6.sin6_addr, addrbuff, INET6_ADDRSTRLEN);
#endif

      if (new->addr.sa.sa_family == AF_INET && peerfd == -1)
	{
	  syslog(LOG_WARNING, 
		 "ignoring nameserver %s - no IPv4 kernel support", addrbuff);
	  safe_free(new);
	}
#ifdef HAVE_IPV6
      else if (new->addr.sa.sa_family == AF_INET6 && peerfd6 == -1)
	{
	  syslog(LOG_WARNING, 
		 "ignoring nameserver %s - no IPv6 kernel support", addrbuff);
	  safe_free(new);
	}
#endif
      else 
	{
	  struct irec *iface;
	  for (iface = interfaces; iface; iface = iface->next)
	    if (sockaddr_isequal(&new->addr, &iface->addr))
	      {
		syslog(LOG_WARNING, "ignoring nameserver %s - local interface", addrbuff);
		break;
	      }
	  if (iface)
	    safe_free(new);
	  else
	    {
	      /* reverse order - gets it right. */
	      new->next = ret;
	      ret = new;
	      if (new->domain)
		syslog(LOG_INFO, "using nameserver %s for domain %s", addrbuff, new->domain);
	      else
		syslog(LOG_INFO, "using nameserver %s", addrbuff); 
	    }
	}
      
      new = tmp;
    }
 
 return ret;
}
  
struct server *reload_servers(char *fname, char *buff, struct server *serv)
{
  FILE *f;
  char *line;
  struct server *old_servers = NULL;
  struct server *new_servers = NULL;

  /* move old servers to free list - we can reuse the memory 
     and not risk malloc if there are the same or fewer new servers. 
     Servers which were specced on the command line go to the new list. */
  while (serv)
    {
      struct server *tmp = serv->next;
      if (serv->from_resolv)
	{
	  serv->next = old_servers;
	  old_servers = serv;
	}
      else
	{
	  serv->next = new_servers;
	  new_servers = serv;
	}
      serv = tmp;
    }

  /* buff happens to be NAXDNAME long... */
#ifdef HAVE_FILE_SYSTEM
  f = fopen(fname, "r");
  if (!f)
    {
      syslog(LOG_ERR, "failed to read %s: %m", fname);
    }
  else
    {
      syslog(LOG_DEBUG, "reading %s", fname);
      while ((line = fgets(buff, MAXDNAME, f)))
	{
	  union  mysockaddr addr;
	  char *token = strtok(line, " \t\n");
	  struct server *serv;
	  
	  if (!token || strcmp(token, "nameserver") != 0)
	    continue;
	  if (!(token = strtok(NULL, " \t\n")))
	    continue;
	  
	  if (inet_pton(AF_INET, token, &addr.in.sin_addr))
	    {
#ifdef HAVE_SOCKADDR_SA_LEN
	      addr.in.sin_len = sizeof(struct sockaddr_in);
#endif
	      addr.in.sin_family = AF_INET;
	      addr.in.sin_port = htons(NAMESERVER_PORT);
	    }
#ifdef HAVE_IPV6
	  else if (inet_pton(AF_INET6, token, &addr.in6.sin6_addr))
	    {
#ifdef HAVE_SOCKADDR_SA_LEN
	      addr.in6.sin6_len = sizeof(struct sockaddr_in6);
#endif
	      addr.in6.sin6_family = AF_INET6;
	      addr.in6.sin6_port = htons(NAMESERVER_PORT);
	      addr.in6.sin6_flowinfo = htonl(0);
	    }
#endif /* IPV6 */
	  else
	    continue;
	  
	  if (old_servers)
	    {
	      serv = old_servers;
	      old_servers = old_servers->next;
	    }
	  else if (!(serv = safe_malloc(sizeof (struct server))))
	    continue;
	  
	  /* this list is reverse ordered: 
	     it gets reversed again in check_servers */
	  serv->next = new_servers;
	  new_servers = serv;
	  serv->addr = addr;
	  serv->domain = NULL;
	  serv->from_resolv = 1;
	}
  
      fclose(f);
    }
#else
#endif

  /* Free any memory not used. */
  while(old_servers)
    {
      struct server *tmp = old_servers->next;
      safe_free(old_servers);
      old_servers = tmp;
    }

  return new_servers;
}







