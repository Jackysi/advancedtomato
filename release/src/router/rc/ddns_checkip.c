#include <stdio.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <setjmp.h>

//#define STANDALONE 1
#define TZO_SUPPORT
#define DYNDNS_SUPPORT

#ifdef STANDALONE
  #define MAIN main
  #define nvram_set(a,b)
#else
#include <bcmnvram.h>
  #define MAIN ddns_checkip_main
#endif

int debug = 0;
int waittime = 3;
char *nvname = NULL;
int timeout = 3;

int log_to_file(char *datas)
{
	FILE *fp;

        if ((fp = fopen("/tmp/ddns_msg", "w")) != NULL) {
		fprintf(fp, "%s", datas);
		fflush(fp);
		fclose(fp);
		return 1;
	}
	else
		return 0;
}

#ifdef TZO_SUPPORT
typedef struct
{
	unsigned int ID ;
	unsigned int Request  ;
	unsigned int IPAddress  ;
	unsigned int Signature  ;
} IPECHO_MESSAGE ;

#define IPECHO_PORT 	        21332
#define TZO_IPECHO_HEADER_SIZE  16
#define IPECHO_ID_SERVER	0x2b4e7cd1
#define IPECHO_ID_CLIENT 	0x43f752e4
#define IPECHO_REQUEST_ECHO 	0x100
#define IPECHO_SIGNATURE_INIT 	0x3e2198a6    

#if LINKSYS_MODEL == WRT54GS
 #define TZO_IPECHO_SERVER	"wrt54gsv3.linksysEchoList.tzo.com"
#elif LINKSYS_MODEL == WRT54GSV4
 #define TZO_IPECHO_SERVER	"wrt54gsv4.linksysEchoList.tzo.com"
#elif LINKSYS_MODEL == WRT54G
 #define TZO_IPECHO_SERVER	"wrt54gv4.linksysEchoList.tzo.com"
#else
 #define TZO_IPECHO_SERVER	"linksysecholist.tzo.com"
#endif

static void
MsgPrint(IPECHO_MESSAGE *msg)
{	
	printf("ID: 	   0x%x\n", msg->ID);
	printf("Request:   0x%x\n", msg->Request);
	printf("IPAddress: 0x%x\n", msg->IPAddress);
	printf("Signature: 0x%x\n", msg->Signature);
}

static unsigned int 
CalcIPEchoSignature(IPECHO_MESSAGE * lpMsg)
{
	unsigned int Result = IPECHO_SIGNATURE_INIT ;

	Result ^= lpMsg->ID ;
	Result += lpMsg->Request ;
	Result ^= lpMsg->IPAddress ;

	return (Result) ;
}

static jmp_buf time_to_wakeup;
static void
dcc_alarm_handler()
{
		longjmp(time_to_wakeup, 1);
}

/* The gethostbyname() use connect() to communicate with DNS server.
   Sometime the function will be blocked for serval minutes.
   So we use alarm()/setjmp()/longjmp() to implement a nonblock gethostbyname().
*/
static struct hostent *
mygethostbyname(char *HostName, int timeout)
{
	void *prev_signal;
	struct hostent *lpHostEnt ;

	prev_signal = signal(SIGALRM, dcc_alarm_handler);

	if(!setjmp(time_to_wakeup)) {
		if(debug)	printf("Set ALARM for %d seconds\n", timeout);
		alarm(timeout);	// Enable the alarm

		if(debug)	printf("Resolving %s\n", HostName);
		lpHostEnt = gethostbyname(HostName);

		alarm(0);	// disconnect the alarm
		signal(SIGALRM, prev_signal);	// Restore previous behavior

		return lpHostEnt;
	}	
	else {
			fprintf(stderr, "Timeout looking for host: %s\n", HostName);
			signal(SIGALRM, prev_signal);
			return NULL;
	}

}

static unsigned int 
FetchIPAddress(char *  HostName)
{
	struct hostent *lpHostEnt ;
	unsigned int IPAddress ;
	char * lpIPAddress = (char *)&IPAddress ;
	int i ;
  
	if (HostName == NULL)
		return(-1) ;

	if ((IPAddress = inet_addr(HostName)) != INADDR_NONE) {
		goto ok;
	}

	if(timeout)
  		if ((lpHostEnt = mygethostbyname(HostName, timeout)) == 0)
			return(-1);
	else
  		if ((lpHostEnt = gethostbyname(HostName)) == 0)
			return(-1);

	for (i=0; i<4; i++)
		lpIPAddress[i] = lpHostEnt->h_addr[i] ;

ok:

	if(debug) {
		struct sockaddr_in sin;

		sin.sin_addr.s_addr = IPAddress;
		printf("%s is %s\n", HostName, inet_ntoa(sin.sin_addr));
	}

	return (IPAddress) ;
}

static int 
CheckForNewIPAddress(unsigned int *Addr)
{
	IPECHO_MESSAGE Msg ;
	int Socket ;
	fd_set SocketArray ;
	int Result, ResponseLength ;
	struct sockaddr_in sin;
	int Tries = 2 ;
	unsigned int EchoServerIPAddress ;
	struct timeval tv = {0,0}  ;

	if (((EchoServerIPAddress = FetchIPAddress(TZO_IPECHO_SERVER)) == -1))
	{
		printf("strange server response, could not make connection to remote server\n");
		log_to_file("dyn_strange");

		fprintf(stderr, "Unable to contact %s\n", TZO_IPECHO_SERVER);
		return(-1) ;
	}

	if ((Socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("strange server response, could not create a socket\n");
		log_to_file("dyn_strange");
		return(-1) ;
	}

	sin.sin_family = AF_INET ;
	sin.sin_addr.s_addr = EchoServerIPAddress ;
	sin.sin_port = htons(IPECHO_PORT) ;

	Msg.ID = IPECHO_ID_CLIENT ;
	Msg.Request = IPECHO_REQUEST_ECHO ;
	Msg.IPAddress = *Addr ;
	Msg.Signature = CalcIPEchoSignature(&Msg) ;

	ResponseLength = 0 ;

	if(debug) {
		printf("Send:\n");
		MsgPrint(&Msg);
		printf("\n");
	}

	if ((sendto(Socket, (unsigned char *)&Msg, sizeof(Msg), 0, (const struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		printf("strange server response, could not send data to remote server\n");
		log_to_file("dyn_strange");
		close(Socket) ;
		return(-1) ;
	}

	while (Tries--)
	{
		tv.tv_sec = waittime ;
		tv.tv_usec = 0 ;

		FD_ZERO(&SocketArray);
		FD_SET(Socket, &SocketArray);

		Result = select(Socket+1, &SocketArray, NULL, NULL, &tv) ;

		if (Result)
			break ;
		sleep(1) ;
	}

	if (!Result)
	{
		printf("strange server response, no resonce from server\n");
		log_to_file("dyn_strange");
		close(Socket) ;
		return(-1) ;
	}

	if ((ResponseLength = recvfrom(Socket, (unsigned char *)&Msg, 16, 0, 0, 0)) <= 0)
	{
		printf("strange server response, bad responce from server\n");
		log_to_file("dyn_strange");
		close(Socket) ;
		return(-1) ;
	}

	close(Socket) ;

	if(debug) {
		printf("Receive:\n");
		MsgPrint(&Msg);
		printf("\n");
	}

	if (ResponseLength == sizeof(IPECHO_MESSAGE))
	{
		*Addr = htonl(Msg.IPAddress);
		return(1);
	}
	else {
		printf("strange server response, are you connecting to the right server?\n");
		log_to_file("dyn_strange");
		return(-1);
	}
}

static int 
get_if_addr(struct sockaddr_in *sin)
{
	if (CheckForNewIPAddress((unsigned int *)&sin->sin_addr) == 1)
	{
		return(0) ;
	}

	return (-1);
}

static int
TZOECHO_check_ip(void)
{
	struct sockaddr_in sin;

	memset(&sin, '\0', sizeof(sin));

	if(get_if_addr(&sin) == 0) {

		if(nvname)
			nvram_set(nvname, inet_ntoa(sin.sin_addr));

		printf("Public IP:%s\n", inet_ntoa(sin.sin_addr));
		return(0);
	}
	else
		return(-1);
}

static int
TZO_check_ip(void)
{
	char cmd[254], ip[20];
	int ret;
	FILE *fp;
	
	snprintf(cmd, sizeof(cmd), "wget http://echo.tzo.com -q -O /tmp/public_ip\n");
	ret = system(cmd);

	if(ret == 0) {
		if((fp = fopen("/tmp/public_ip", "r"))) {
			char string[254];
			fgets(string, sizeof(string), fp);
			sscanf(string, "%*[^:]:%s", ip);

			if(nvname)
				nvram_set(nvname, ip);

			printf("Public IP:%s\n", ip);
			return 0;
		}
	}
	else
		return(-1);
	
}
#endif

#ifdef DYNDNS_SUPPORT
static int
dyndns_check_ip(void)
{
	char cmd[254], ip[20];
	int ret;
	FILE *fp;
	
	snprintf(cmd, sizeof(cmd), "wget http://checkip.dyndns.org -q -O /tmp/public_ip\n");
	ret = system(cmd);

	if(ret == 0) {
		if((fp = fopen("/tmp/public_ip", "r"))) {
			char string[254];
			char *str;

			fgets(string, sizeof(string), fp);

			str = strstr(string, "Current IP Address:");

			if(str) {
				sscanf(str, "%*[^:]: %[^<]", ip);

				if(nvname)
					nvram_set(nvname, ip);

				printf("Public IP:%s\n", ip);
			}
			return(0);
		}
	}
	else
		return(-1);
}
#endif

static void
usage(char *name)
{
	printf("Usage: %s -h -d -t [tzo|tzo-echo|dyndns] -n public_ip -T timeout\n", name);
}

int
MAIN(int argc, char *argv[])
{
	int c;
	char *type = NULL;
	int ret = 0;
 
	while ((c = getopt(argc, argv, "hdt:w:n:T:")) != -1)
		switch(c) {
			case 'h':
				usage(argv[0]);
				exit(0);
			case 'd':
				debug = 1;
				break;
			case 't':
				type = optarg;
				break;
			case 'w':
				waittime = atoi(optarg);
				break;
			case 'n':
				nvname = optarg;
				break;
			case 'T':
				timeout = atoi(optarg);
				break;
			break;
		}

	if(!type) {
		usage(argv[0]);
		exit(0);
	}
	if(!strcmp(type, "tzo")) {
#ifdef TZO_SUPPORT
		ret = TZO_check_ip();
#endif
	}
	else if(!strcmp(type, "tzo-echo")) {
#ifdef TZO_SUPPORT
		ret = TZOECHO_check_ip();
#endif
	}
	else if(!strcmp(type, "dyndns")) {
#ifdef DYNDNS_SUPPORT
		ret = dyndns_check_ip();
#endif
	}
	else {
		usage(argv[0]);
		exit(0);
	}
	return ret;
}
