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

/* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
/* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
//static struct frec *ftab;
struct frec *ftab;

static struct frec *get_new_frec(time_t now);
static struct frec *lookup_frec(unsigned short id);
static struct frec *lookup_frec_by_sender(unsigned short id,
					  union mysockaddr *addr);
static unsigned short get_id(void);

char query_name[254];

/* May be called more than once. */
void forward_init(int first)
{
  int i;

  if (first)
    ftab = safe_malloc(FTABSIZ*sizeof(struct frec));
  for (i=0; i<FTABSIZ; i++)
    ftab[i].new_id = 0;
}

/* returns new last_server */	
struct server *forward_query(int udpfd, int peerfd, int peerfd6,
			     union mysockaddr *udpaddr, HEADER *header, 
			     int plen, int strict_order, char *dnamebuff, 
			     struct server *servers, struct server *last_server)
{
  time_t now = time(NULL);
  struct frec *forward;
  char *domain = NULL;
  struct server *serv;
  int gotname = extract_request(header, (unsigned int)plen, dnamebuff);

  /* may be  recursion not speced or no servers available. */
  if (!header->rd || !servers)
    forward = NULL;
  else if ((forward = lookup_frec_by_sender(ntohs(header->id), udpaddr)))
    {
      /* retry on existing query, send to next server */
      domain = forward->sentto->domain;
      if (!(forward->sentto = forward->sentto->next))
	forward->sentto = servers; /* at end of list, recycle */
      header->id = htons(forward->new_id);
    }
  else
    {
      /* new query, pick nameserver and send */
      forward = get_new_frec(now);
      
      /* If the query ends in the domain in one of our servers, set
	 domain to point to that name. We find the largest match to allow both
	 domain.org and sub.domain.org to exist. */
      
      if (gotname)
	{
	  unsigned int namelen = strlen(dnamebuff);
	  unsigned int matchlen = 0;
	  for (serv=servers; serv; serv=serv->next)
	    if (serv->domain)
	      {
		unsigned int domainlen = strlen(serv->domain);
		//cprintf("tallest:=====( domainlen=%d, namelen=%d, dnamebuff=%s, serv->domain=%s, servip=%s, sa=%x )=====\n"
		//	,domainlen, namelen, dnamebuff, serv->domain, inet_ntoa(serv->addr.in.sin_addr), serv->addr.sa.sa_data);
		if (namelen >= domainlen &&
		    strcmp(dnamebuff + namelen - domainlen, serv->domain) == 0 &&
		    domainlen > matchlen)
		  {
			//cprintf("tallest:=====( Match it!! dnamebuff=%s, serv->domain=%s, Using DNS=%s )=====\n", dnamebuff, serv->domain, inet_ntoa(serv->addr.in.sin_addr));
		    domain = serv->domain;
		    matchlen = domainlen;
		  }
	      }
	}
      
      /* In strict_order mode, or when using domain specific servers
	 always try servers in the order specified in resolv.conf,
	 otherwise, use the one last known to work. */
      
      if (domain || strict_order)
	forward->sentto = servers;
      else
	forward->sentto = last_server;
	
      forward->source = *udpaddr;
      forward->new_id = get_id();
      forward->fd = udpfd;
      forward->orig_id = ntohs(header->id);
      header->id = htons(forward->new_id);
    }
  
  /* check for send errors here (no route to host) 
     if we fail to send to all nameservers, send back an error
     packet straight away (helps modem users when offline)  */
  
  if (forward)
    {
      struct server *firstsentto = forward->sentto;
      int sendnotfail = 0;// add by zg 2006.10.23 to fix cdrouter3.3 item 129(cdrouter_app_25) bug 
      while (1)
	{ 
	  int af = forward->sentto->addr.sa.sa_family; 
	  int fd = af == AF_INET ? peerfd : peerfd6;
	  
	  /* only send to servers dealing with our domain.
	     domain may be NULL, in which case server->domain 
	     must be NULL also. */
	  
	  if ((!domain && !forward->sentto->domain) ||
	      (domain && forward->sentto->domain && strcmp(domain, forward->sentto->domain) == 0))
	    {
	      if (sendto(fd, (char *)header, plen, 0,
			 &forward->sentto->addr.sa,
			 sa_len(&forward->sentto->addr)) != -1)
		{
                  sendnotfail = 1;// add by zg 2006.10.23 to fix cdrouter3.3 item 129(cdrouter_app_25) bug
                  
                        /* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
                        /* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
                        if(forward->dnsMsgBufPtr == NULL)
                        {
                                forward->dnsMsgBufPtr = get_dns_msg_buf();
                                if(forward->dnsMsgBufPtr != NULL)
                                {
                                        struct DNS_MSG_BUF * dnsMsgBufPtr;
                                        //DBG_printf("send query ok and backup it\n");
                                        dnsMsgBufPtr = forward->dnsMsgBufPtr;
                                        dnsMsgBufPtr->udpfd = udpfd;
                                        dnsMsgBufPtr->plen = plen;
                                        dnsMsgBufPtr->firstsentto = firstsentto;
                                        if (!(dnsMsgBufPtr->nextsentto = forward->sentto->next))
                                        {
                                                dnsMsgBufPtr->nextsentto = servers;
                                        }
                                        dnsMsgBufPtr->servers = servers;
                                        memcpy(&dnsMsgBufPtr->udpaddr, udpaddr, sizeof(union mysockaddr));
                                        memcpy(dnsMsgBufPtr->header, header, plen);
                                        ((HEADER *)(&dnsMsgBufPtr->header))->id = forward->orig_id;
                                }
                        }

		        if (af == AF_INET)
		           log_query(F_SERVER | F_IPV4 | F_FORWARD, gotname ? dnamebuff : "query", 
			      (struct all_addr *)&forward->sentto->addr.in.sin_addr);
#ifdef HAVE_IPV6
		        else
		           log_query(F_SERVER | F_IPV6 | F_FORWARD, gotname ? dnamebuff : "query", 
			      (struct all_addr *)&forward->sentto->addr.in6.sin6_addr);
#endif
                        break;
                        /* *******end by zg porting DWG814I Source code on 2006.11.06 ******* */

		  /* for no-domain, dont't update last_server */
                  // modify by zg 2006.10.23 to fix cdrouter3.3 item 129(cdrouter_app_25) bug 
		  //return domain ? last_server : (forward->sentto->next ? forward->sentto->next : servers);
		}
	    } 
	  
	  if (!(forward->sentto = forward->sentto->next))
	    forward->sentto = servers;
	  
	  /* check if we tried all without success */
	  if (forward->sentto == firstsentto)
	    break;
	}
      
      /* could not send on, prepare to return */
      /******** add by zg 2006.10.23 to fix cdrouter3.3 item 129(cdrouter_app_25) bug ********/
      if(!sendnotfail)
      { 
          header->id = htons(forward->orig_id);
          forward->new_id = 0; /* cancel */
      }
      else
          return last_server;
      /**************** end by zg 2006.10.23 ***************/
    }	  
  
  /* could not send on, return empty answer */
  header->qr = 1; /* response */
  header->aa = 0; /* authoritive - never */
  header->ra = 1; /* recursion if available */
  header->tc = 0; /* not truncated */
  header->rcode = NOERROR; /* no error */
  header->ancount = htons(0); /* no answers */
  header->nscount = htons(0);
  header->arcount = htons(0);
  sendto(udpfd, (char *)header, plen, 0, &udpaddr->sa, sa_len(udpaddr));

  return last_server;
}

/* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
/* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
/* reforward last query */
int reforward_query(struct frec *forward, int peerfd, int peerfd6, int strict_order, char *dnamebuff)
{
        char *domain;
        struct DNS_MSG_BUF * dnsMsgBufPtr;
        HEADER *header;
        int plen;
                                                                                                                             
        if (forward == NULL)
        {
                return -1;
        }
                                                                                                                             
        dnsMsgBufPtr = forward->dnsMsgBufPtr;
        if (dnsMsgBufPtr == NULL)
        {
                return -1;
        }
                                                                                                                             
        if(dnsMsgBufPtr->nextsentto == dnsMsgBufPtr->firstsentto)
        {
                return -1;
        }
                                                                                                                             
        plen = dnsMsgBufPtr->plen;
        header = (HEADER *)&dnsMsgBufPtr->header;
                                                                                                                             
        /* retry on existing query, send to next server */
        domain = dnsMsgBufPtr->firstsentto->domain;
        //DBG_printf("reforward_query header->id=%d forward->new_id=%d\n", header->id, htons(forward->new_id));
        header->id = htons(forward->new_id);
                                                                                                                             
        /* check for send errors here (no route to host)
        if we fail to send to all nameservers, send back an error
        packet straight away (helps modem users when offline)  */
                                                                                                                             
                                                                                                                             
        while (1)
        {
                int af = dnsMsgBufPtr->nextsentto->addr.sa.sa_family;
                int fd = af == AF_INET ? peerfd : peerfd6;
                                                                                                                             
                /* only send to servers dealing with our domain.
                domain may be NULL, in which case server->domain
                must be NULL also. */
                                                                                                                             
                if ((!domain && !dnsMsgBufPtr->nextsentto->domain) ||
                        (domain && dnsMsgBufPtr->nextsentto->domain &&
                        strcmp(domain, dnsMsgBufPtr->nextsentto->domain) == 0))
                {
                                if (sendto(fd, (char *)header, plen, 0, &dnsMsgBufPtr->nextsentto->addr.sa,
                                        sa_len(&dnsMsgBufPtr->nextsentto->addr)) != -1)
                                {
                                        //junzhao 2004.5.31
                                        if (af == AF_INET)
                                        {
                                                log_query(F_SERVER | F_IPV4 | F_FORWARD, dnsMsgBufPtr->gotname ? dnamebuff :
"query",
                                                        (struct all_addr *)&dnsMsgBufPtr->nextsentto->addr.in.sin_addr);
                                        }
#ifdef HAVE_IPV6
                                        else
                                        {
                                                log_query(F_SERVER | F_IPV6 | F_FORWARD, dnsMsgBufPtr->gotname ? dnamebuff :
"query",
                                                        (struct all_addr *)&dnsMsgBufPtr->nextsentto->addr.in6.sin6_addr);
                                        }
#endif
                                                                                                                             
                                        if (!(dnsMsgBufPtr->nextsentto = dnsMsgBufPtr->nextsentto->next))
                                        {
                                                dnsMsgBufPtr->nextsentto = dnsMsgBufPtr->servers;
                                        }
                                        return 0;
                                }
                }
                                                                                                                             
                                                                                                                             
                if (!(dnsMsgBufPtr->nextsentto = dnsMsgBufPtr->nextsentto->next))
                {
                        dnsMsgBufPtr->nextsentto = dnsMsgBufPtr->servers;
                }
                                                                                                                             
                /* check if we tried all without success */
                if (dnsMsgBufPtr->nextsentto == dnsMsgBufPtr->firstsentto)
                {
                        break;
                }
        }//while 1
                                                                                                                             
        /* could not send on, prepare to return */
        return -1;
}
/* ******* end by zg porting DWG814I Source code on 2006.11.06 ******* */

/* returns new last_server */
struct server *reply_query(int fd, char *packet, char *dnamebuff, struct server *last_server,unsigned long *timetolive)
{
  /* packet from peer server, extract data for cache, and send to
     original requester */
  struct frec *forward;
  HEADER *header;
  int n = recv(fd, packet, PACKETSZ, 0);
  
  header = (HEADER *)packet;
  if (n >= (int)sizeof(HEADER) && header->qr)
    {
      if ((forward = lookup_frec(ntohs(header->id))))
	{
	  //if (header->rcode == NOERROR || header->rcode == NXDOMAIN)
          if (header->rcode == NOERROR)
	    {
               /* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
               /* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */ 
               if(forward ->dnsMsgBufPtr != NULL)
                {
                        free_dns_msg_buf(forward ->dnsMsgBufPtr);
                        forward ->dnsMsgBufPtr = NULL;
                }
                /* ******* end by zg porting DWG814I Source code on 2006.11.06 ******* */                   

	      if (!forward->sentto->domain)
		last_server = forward->sentto; /* known good */
	      if (header->opcode == QUERY)
		{
			strncpy(query_name, dnamebuff, sizeof(query_name));
			extract_addresses(header, (unsigned int)n, dnamebuff, timetolive);		
		}
	    
	  
	  header->id = htons(forward->orig_id);
	  /* There's no point returning an upstream reply marked as truncated,
	     since that will prod the resolver into moving to TCP - which we
	     don't support. */
	  header->tc = 0; /* goodbye truncate */
	  sendto(forward->fd, packet, n, 0, 
		 &forward->source.sa, sa_len(&forward->source));
	  forward->new_id = 0; /* cancel */
           }
          
          /* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
          /* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
          else switch(header->rcode)
          {
                case SERVFAIL:
                case NOTIMP:
                case REFUSED:
                        //DBG_printf("SERVER fail, not implement, refuse, try next server\n");
                        break;
                                                                                                                             
                default:
                        //DBG_printf("SERVER response general error, return it to client\n");
                        if(forward ->dnsMsgBufPtr != NULL)
                        {
                                free_dns_msg_buf(forward ->dnsMsgBufPtr);
                                forward ->dnsMsgBufPtr = NULL;
                        }
                                                                                                                             
                        header->id = htons(forward->orig_id);
                        /* There's no point returning an upstream reply marked as truncated,
                        since that will prod the resolver into moving to TCP - which we
                        don't support. */
                        header->tc = 0; /* goodbye truncate */
                        sendto(forward->fd, packet, n, 0,
                        &forward->source.sa, sa_len(&forward->source));
                        forward->new_id = 0; /* cancel */
                        break;
          }
          /* ******* end by zg porting DWG814I Source code on 2006.11.06 ******* */
	}
    }

  return last_server;
}
      

static struct frec *get_new_frec(time_t now)
{
  int i;
  struct frec *oldest = &ftab[0];
  time_t oldtime = now;

  for(i=0; i<FTABSIZ; i++)
    {
      struct frec *f = &ftab[i];
      if (f->time <= oldtime)
	{
	  oldtime = f->time;
	  oldest = f;
	}
      if (f->new_id == 0)
	{
	  f->time = now;
          
          /* ******* zg porting DWG814I Source code on 2006.11.06 ******* */
          /* ******* To fixed cdrouterv3.3 item 333(dna_45) item 334(dns_45) failed bug ******* */
          f->dnsMsgBufPtr = NULL;

	  return f;
	}
    }

  /* table full, use oldest */

  oldest->time = now;
  return oldest;
}
 
static struct frec *lookup_frec(unsigned short id)
{
  int i;
  for(i=0; i<FTABSIZ; i++)
    {
      struct frec *f = &ftab[i];
      if (f->new_id == id)
	return f;
    }
  return NULL;
}

static struct frec *lookup_frec_by_sender(unsigned short id,
					  union mysockaddr *addr)
{
  int i;
  for(i=0; i<FTABSIZ; i++)
    {
      struct frec *f = &ftab[i];
      if (f->new_id &&
	  f->orig_id == id && 
	  sockaddr_isequal(&f->source, addr))
	return f;
    }
  return NULL;
}


/* return unique random ids between 1 and 65535 */
static unsigned short get_id(void)
{
  unsigned short ret = 0;

  while (ret == 0)
    {
      ret = rand16();
      
      /* scrap ids already in use */
      if ((ret != 0) && lookup_frec(ret))
	ret = 0;
    }

  return ret;
}


