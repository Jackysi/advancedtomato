#include <netdb.h>
#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_FUNC_GETHOSTBYNAME_R_6
struct hostent * gethostname_re (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen)
{
	struct hostent *hp;
	int herr,res;

	if (*hstbuflen == 0)
	{
		*hstbuflen = 1024; 
		*tmphstbuf = (char *)malloc (*hstbuflen);
	}

	while (( res = 
		gethostbyname_r(host,hostbuf,*tmphstbuf,*hstbuflen,&hp,&herr))
		&& (errno == ERANGE))
	{
		/* Enlarge the buffer. */
		*hstbuflen *= 2;
		*tmphstbuf = (char *)realloc (*tmphstbuf,*hstbuflen);
	}
	if (res)
		return NULL;
	return hp;
}
#endif
#ifdef HAVE_FUNC_GETHOSTBYNAME_R_5
struct hostent * gethostname_re (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen)
{
	struct hostent *hp;
	int herr;

	if (*hstbuflen == 0)
	{
		*hstbuflen = 1024;
		*tmphstbuf = (char *)malloc (*hstbuflen);
	}

	while ((NULL == ( hp = 
		gethostbyname_r(host,hostbuf,*tmphstbuf,*hstbuflen,&herr)))
		&& (errno == ERANGE))
	{
		/* Enlarge the buffer. */
		*hstbuflen *= 2;
		*tmphstbuf = (char *)realloc (*tmphstbuf,*hstbuflen);
	}
	return hp;
}
#endif
#ifdef HAVE_FUNC_GETHOSTBYNAME_R_3
struct hostent * gethostname_re (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen)
{
	if (*hstbuflen == 0)
	{
		*hstbuflen = sizeof(struct hostent_data);
		*tmphstbuf = (char *)malloc (*hstbuflen);
	}
	else if (*hstbuflen < sizeof(struct hostent_data))
	{
		*hstbuflen = sizeof(struct hostent_data);
		*tmphstbuf = (char *)realloc(*tmphstbuf, *hstbuflen);
	}
	memset((void *)(*tmphstbuf),0,*hstbuflen);

	if (0 != gethostbyname_r(host,hostbuf,(struct hostent_data *)*tmphstbuf))
		return NULL;
	return hostbuf;
}
#endif
#ifdef HAVE_FUNC_GETHOSTBYNAME_R_0
#warning WARNING! Your system does not have a thread-safe DNS resolver (gethostbyname_r)!
#warning WARNING! Name service lookups may be corrupted. Consider switching to 
#warning WARNING! a more thread-friendly platform.
struct hostent * gethostname_re (const char *host,struct hostent *hostbuf,char **tmphstbuf,size_t *hstbuflen)
{
	return gethostbyname(host);
}
#endif
