/*
 *   $Id: device-bsd44.c,v 1.13 2004/02/05 18:44:00 lutchann Exp $
 *
 *   Authors:
 *    Craig Metz		<cmetz@inner.net>
 *
 *   This software is Copyright 1996,1997 by the above mentioned author(s), 
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <lutchann@litech.org>.
 *
 */

#include <config.h>
#include <includes.h>
#include <radvd.h>
#include <defaults.h>
#include <pathnames.h>		/* for PATH_PROC_NET_IF_INET6 */

static uint8_t ll_prefix[] = { 0xfe, 0x80 };

/*
 * this function gets the hardware type and address of an interface,
 * determines the link layer token length and checks it against
 * the defined prefixes
 */
int
setup_deviceinfo(int sock, struct Interface *iface)
{
	struct ifconf ifconf;
	int nlen;
	uint8_t *p, *end;
	struct AdvPrefix *prefix;
	char zero[HWADDR_MAX];

	/* just allocate 8192 bytes, should be more than enough.. */
	if (!(ifconf.ifc_buf = malloc(ifconf.ifc_len = (32 << 8))))
	{
		flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
		goto ret;
	}

	if (ioctl(sock, SIOCGIFCONF, &ifconf) < 0)
	{
		flog(LOG_ERR, "ioctl(SIOCGIFCONF) failed: %s(%d)", strerror(errno), errno);
		goto ret;
	}

	p = (uint8_t *)ifconf.ifc_buf;
	end = p + ifconf.ifc_len;
	nlen = strlen(iface->Name);

	while(p < end)
	{
		p += IFNAMSIZ;
		
		if ((p + 2) >= end)
			break;
			
		if ((p + *p) >= end)
			break;
			
		if ((*(p + 1) == AF_LINK) &&
		    (((struct sockaddr_dl *)p)->sdl_nlen == nlen) &&
		    (!memcmp(iface->Name, ((struct sockaddr_dl *)p)->sdl_data, nlen)))
		{
		
			if (((struct sockaddr_dl *)p)->sdl_alen > HWADDR_MAX)
			{
				flog(LOG_ERR, "address length %d too big for",
					((struct sockaddr_dl *)p)->sdl_alen,
					iface->Name);
				goto ret;
			}
		
			memcpy(iface->if_hwaddr, LLADDR((struct sockaddr_dl *)p), ((struct sockaddr_dl *)p)->sdl_alen);
			iface->if_hwaddr_len = ((struct sockaddr_dl *)p)->sdl_alen << 3;

          		switch(((struct sockaddr_dl *)p)->sdl_type) {
            		case IFT_ETHER:
            		case IFT_ISO88023:
            			iface->if_prefix_len = 64;
              			iface->if_maxmtu = 1500;
              			break;
            		case IFT_FDDI:
            			iface->if_prefix_len = 64;
              			iface->if_maxmtu = 4352;
              			break;
            		default:
            			iface->if_prefix_len = -1;
				iface->if_maxmtu = -1;
				break;
          		}

			dlog(LOG_DEBUG, 3, "link layer token length for %s is %d", iface->Name,
				iface->if_hwaddr_len);

			dlog(LOG_DEBUG, 3, "prefix length for %s is %d", iface->Name,
				iface->if_prefix_len);

			if (iface->if_prefix_len != -1) {
				memset(zero, 0, ((struct sockaddr_dl *)p)->sdl_alen);
				if (!memcmp(iface->if_hwaddr, zero, ((struct sockaddr_dl *)p)->sdl_alen))
					flog(LOG_WARNING, "WARNING, MAC address on %s is all zero!",
						iface->Name);
			}
			
			prefix = iface->AdvPrefixList;
			while (prefix)
			{
				if ((iface->if_prefix_len != -1) &&
					(iface->if_prefix_len != prefix->PrefixLen))
				{
					flog(LOG_WARNING, "prefix length should be %d for %s",
						iface->if_prefix_len, iface->Name);
 				}
 			
 				prefix = prefix->next;
			}
          		
          		free(ifconf.ifc_buf);
          		return 0;
        	}
        
    		p += *p;	
	}

ret:
	iface->if_maxmtu = -1;
	iface->if_hwaddr_len = -1;
	iface->if_prefix_len = -1;
	free(ifconf.ifc_buf);
	return 0;
}

int setup_linklocal_addr(int sock, struct Interface *iface)
{
	struct ifconf ifconf;
	int nlen;
	uint8_t *p, *end;
	int index = 0;

	/* just allocate 8192 bytes, should be more than enough.. */
	if (!(ifconf.ifc_buf = malloc(ifconf.ifc_len = (32 << 8))))
	{
		flog(LOG_CRIT, "malloc failed: %s", strerror(errno));
		goto ret;
	}

	if (ioctl(sock, SIOCGIFCONF, &ifconf) < 0)
	{
		flog(LOG_ERR, "ioctl(SIOCGIFCONF) failed: %s(%d)", strerror(errno), errno);
		goto ret;
	}

	p = (uint8_t *)ifconf.ifc_buf;
	end = p + ifconf.ifc_len;
	nlen = strlen(iface->Name);

	while(p < end)
  	{
		p += IFNAMSIZ;
	
		if ((p + 2) >= end)
			break;
			
		if ((p + *p) >= end)
			break;
			
		if ((*(p + 1) == AF_LINK) &&
		    (((struct sockaddr_dl *)p)->sdl_nlen == nlen) &&
		    (!memcmp(iface->Name, ((struct sockaddr_dl *)p)->sdl_data, nlen)))
		{
			index = ((struct sockaddr_dl *)p)->sdl_index;
		}
		
   	 	if (index && (*(p + 1) == AF_INET6))
		  if (!memcmp(&((struct sockaddr_in6 *)p)->sin6_addr, ll_prefix, sizeof(ll_prefix)))
		  {
			memcpy(&iface->if_addr, &((struct sockaddr_in6 *)p)->sin6_addr, sizeof(struct in6_addr));
			iface->if_index = index;

			free(ifconf.ifc_buf);
			return 0;
      	  	  }
      	  
		p += *p;

	}

ret:
	flog(LOG_ERR, "no linklocal address configured for %s", iface->Name);
	free(ifconf.ifc_buf);
	return -1;
}

int setup_allrouters_membership(int sock, struct Interface *iface)
{
	return (0);
}

int check_allrouters_membership(int sock, struct Interface *iface)
{
	return (0);
}

/* UNTESTED - This code is from device-linux.c and has not been tested
 * under BSD.  If it is broken in the distribution and you fix it, please
 * send me the patch.  -lutchann */

int
get_v4addr(const char *ifn, unsigned int *dst)
{
	struct ifreq	ifr;
	struct sockaddr_in *addr;
	int fd;

	if( ( fd = socket(AF_INET,SOCK_DGRAM,0) ) < 0 )
	{
		flog(LOG_ERR, "create socket for IPv4 ioctl failed for %s: %s",
			ifn, strerror(errno));
		return (-1);
	}
	
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifn, IFNAMSIZ-1);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	ifr.ifr_addr.sa_family = AF_INET;
	
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
	{
		flog(LOG_ERR, "ioctl(SIOCGIFADDR) failed for %s: %s",
			ifn, strerror(errno));
		close( fd );
		return (-1);
	}

	addr = (struct sockaddr_in *)(&ifr.ifr_addr);

	dlog(LOG_DEBUG, 3, "IPv4 address for %s is %s", ifn,
		inet_ntoa( addr->sin_addr ) ); 

	*dst = addr->sin_addr.s_addr;

	close( fd );

	return 0;
}
