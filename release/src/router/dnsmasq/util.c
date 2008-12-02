/* dnsmasq is Copyright (c) 2000 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/


/* Code in this file contributed by Rob Funk. */

#include "dnsmasq.h"

/* Prefer arc4random(3) over random(3) over rand(3) */
/* Also prefer /dev/urandom over /dev/random, to preserve the entropy pool */
#ifdef HAVE_ARC4RANDOM
# define rand()		arc4random()
# define srand(s)	(NULL)
# define RANDFILE	(NULL)
#else
# ifdef HAVE_RANDOM
#  define rand()	random()
#  define srand(s)	srandom(s)
# endif
# ifdef HAVE_DEV_URANDOM
#  define RANDFILE	"/dev/urandom"
# else
#  ifdef HAVE_DEV_RANDOM
#   define RANDFILE	"/dev/random"
#  else
#   define RANDFILE	(NULL)
#  endif
# endif
#endif

unsigned short rand16(void)
{
  static int been_seeded = 0;
#ifdef HAVE_FILE_SYSTEM
  const char *randfile = RANDFILE;
#endif
  
  if (! been_seeded) 
    {
#ifdef HAVE_FILE_SYSTEM
      int fd, n = 0;
#endif
      unsigned int c = 0, seed = 0, badseed;
#ifdef HAVE_FILE_SYSTEM
      char sbuf[sizeof(seed)];
      char *s;
#endif
      struct timeval now;

      /* get the bad seed as a backup */
      /* (but we'd rather have something more random) */
      gettimeofday(&now, NULL);
      badseed = now.tv_sec ^ now.tv_usec ^ (getpid() << 16);

#ifdef HAVE_FILE_SYSTEM
      fd = open(randfile, O_RDONLY);
      if (fd < 0) 
	seed = badseed;
      else
	{
	  s = (char *) &seed;
	  while ( (c < sizeof(seed)) &&
		  ((n = read(fd, sbuf, sizeof(seed)) > 0)) ) 
	    {
	      memcpy(s, sbuf, n);
	      s += n;
	      c += n;
	    }
	  if (n < 0)
	    {
	      seed = badseed;
	    }
	}
      if (seed != badseed)
	close(fd);
#else
      seed = badseed;
#endif
      
      srand(seed);
      been_seeded = 1;
    }
  
  /* Some rand() implementations have less randomness in low bits
   * than in high bits, so we only pay attention to the high ones.
   * But most implementations don't touch the high bit, so we 
   * ignore that one.
   */
  return( (unsigned short) (rand() >> 15) );
}

void canonicalise(char *s)
{
  char *p;
  int l = strlen(s);
  
  for (p=s; *p; p++)
    *p = tolower(*p);

  if (l>0 && s[l-1] == '.')
    s[l-1] = 0;
}

/*
* memory counters - debugging only
*/
#if defined(DEBUG_MEMORY)
static unsigned int allocated_memory_count = 0;
void dnsr_mem_used(void)
{
	printf("Total memory used: %u bytes\n", allocated_memory_count);
}
#endif

void safe_free(void *mem)
{
#if defined(DEBUG_MEMORY)
	unsigned int size;
	char *ptr = mem;
	if (ptr == NULL)
	{
		printf("safe_free: NULL pointer!\n");
		return;
	}
	else if (((unsigned int)ptr)&3)
	{
		printf("safe_free: Memory at 0x%08x not aligned!\n", mem);
	}
	else
	{
		if (*(--(unsigned int *)ptr) != 0xDEADBEEF)
		{
			printf("safe_free: Memory at 0x%08x untagged!\n", mem);
		}
		else
		{
			size = *(--(unsigned int *)ptr);
			allocated_memory_count -= size;
			mem = ptr;
		}
	}
#endif
	free(mem);
}

/* for use during startup */
void *safe_malloc(int size)
{
#if defined(DEBUG_MEMORY)
  char *ret = memalign(4, ((size+3)&~3)+3*sizeof(unsigned int));
#else
  void *ret = malloc(size);
#endif
  
  if (!ret)
    die("could not get memory", NULL);

#if defined(DEBUG_MEMORY)
   *(unsigned int *)ret = size;
   ret += sizeof(unsigned int);
   *(unsigned int *)ret = 0xDEADBEEF;
   ret += sizeof(unsigned int);
   memset(ret, 0, size);
   *(unsigned int *)(ret+((size+3)&~3)) = 0xDEADBEEF;
   allocated_memory_count += size;
#endif

  return ret;
}
    
char *safe_string_alloc(char *cp)
{
  char *ret = NULL;

  if (cp && strlen(cp) != 0)
    {
      ret = safe_malloc(strlen(cp)+1);
      strcpy(ret, cp);
    }

  return ret;
}

void die(char *message, char *arg1)
{
  char *errmess = strerror(errno);
  
  if (!arg1)
    arg1 = errmess;
  
  fprintf(stderr, "dnsmasq: ");
  fprintf(stderr, message, arg1, errmess);
  fprintf(stderr, "\n");
  
  syslog(LOG_CRIT, message, arg1, errmess);
  syslog(LOG_CRIT, "FAILED to start up");
  exit(1);
}

int sockaddr_isequal(union mysockaddr *s1, union mysockaddr *s2)
{
  if (s1->sa.sa_family == s2->sa.sa_family)
    { 
      if (s1->sa.sa_family == AF_INET &&
	  s1->in.sin_port == s2->in.sin_port &&
	  memcmp(&s1->in.sin_addr, &s2->in.sin_addr, sizeof(struct in_addr)) == 0)
	return 1;
#ifdef HAVE_IPV6      
      if (s1->sa.sa_family == AF_INET6 &&
	  s1->in6.sin6_port == s2->in6.sin6_port &&
	  s1->in6.sin6_flowinfo == s2->in6.sin6_flowinfo &&
	  memcmp(&s1->in6.sin6_addr, &s2->in6.sin6_addr, sizeof(struct in6_addr)) == 0)
	return 1;
#endif
    }
  return 0;
}

int sa_len(union mysockaddr *addr)
{
#ifdef HAVE_SOCKADDR_SA_LEN
  return addr->sa.sa_len;
#else
#ifdef HAVE_IPV6
  if (addr->sa.sa_family == AF_INET6)
    return sizeof(addr->in6);
  else
#endif
    return sizeof(addr->in); 
#endif
}
