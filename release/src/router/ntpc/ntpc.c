/*

	ntpc/ntpsync
	Copyright (C) 2006-2009 Jonathan Zarate

	Licensed under GNU GPL v2 or later.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <syslog.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <shared.h>


#define I_MISC		0
#define I_ORTIME	6
#define	I_TXTIME	10

#define TIMEFIX		2208988800UL


static void add_word(char *buffer, const char *word, int max)
{
	if ((*buffer != 0) && (buffer[strlen(buffer) - 1] != ' '))
		strlcat(buffer, " ", max);
	strlcat(buffer, word, max);
}

// 0 = ok, 1 = failed, 2 = permanent failure
static int ntpc(struct in_addr addr)
{
	uint32_t packet[12];
	struct timeval txtime;
	struct timeval rxtime;
	struct timeval tv;
	uint32_t u;
	uint32_t txn;
	int fd;
	fd_set fds;
	int len;
	struct sockaddr_in sa;
	char s[64], q[128];
	time_t t;
	time_t ntpt;
	time_t diff;

	memset(&sa, 0, sizeof(sa));
	sa.sin_addr = addr;
	sa.sin_port = htons(123);
	sa.sin_family = AF_INET;

	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		printf("Unable to create a socket\n");
		return 1;
	}

	if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
		printf("Unable to connect\n");
	}
	else {
		memset(&packet, 0, sizeof(packet));
		packet[I_MISC] = htonl((4 << 27) | (3 << 24));	// VN=v4 | mode=3 (client)
//		packet[I_MISC] = htonl((3 << 27) | (3 << 24));	// VN=v3 | mode=3 (client)
		gettimeofday(&txtime, NULL);
		packet[I_TXTIME] = txn = htonl(txtime.tv_sec + TIMEFIX);
		send(fd, packet, sizeof(packet), 0);

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = 3;	// no more than 3 seconds
		tv.tv_usec = 0;
		if (select(fd + 1, &fds, NULL, NULL, &tv) != 1) {
			printf("Timeout\n");
		}
		else {
			len = recv(fd, packet, sizeof(packet), 0);
			if (len != sizeof(packet)) {
				printf("Invalid packet size\n");
			}
			else {
				gettimeofday(&rxtime, NULL);

				u = ntohl(packet[0]);

				_dprintf("u = 0x%08x\n", u);
				_dprintf("LI = %u\n", u >> 30);
				_dprintf("VN = %u\n", (u >> 27) & 0x07);
				_dprintf("mode = %u\n", (u >> 24) & 0x07);
				_dprintf("stratum = %u\n", (u >> 16) & 0xFF);
				_dprintf("poll interval = %u\n", (u >> 8) & 0xFF);
				_dprintf("precision = %u\n", u & 0xFF);

				if ((u & 0x07000000) != 0x04000000) {	// mode != 4 (server)
					printf("Invalid response\n");
				}
				else {
					close(fd);

					// notes:
					//	- Windows' ntpd returns vn=3, stratum=0

					if ((u & 0x00FF0000) == 0) {			// stratum == 0
						printf("Received stratum=0\n");
						if (!nvram_match("ntp_kiss_ignore", "1")) {
							return 2;
						}
					}

					ntpt = ntohl(packet[I_TXTIME]) - TIMEFIX;
					t = (rxtime.tv_sec - txtime.tv_sec) >> 1;
					diff = (ntpt - rxtime.tv_sec) + t;

					_dprintf("txtime = %ld\n", txtime.tv_sec);
					_dprintf("rxtime = %ld\n", rxtime.tv_sec);
					_dprintf("ntpt   = %ld\n", ntpt);
					_dprintf("rtt/2  = %ld\n", t);
					_dprintf("diff   = %ld\n", diff);

					if (diff != 0) {
						gettimeofday(&tv, NULL);
						tv.tv_sec  += diff;
//						tv.tv_usec = 0;// sorry, I'm not a time geek :P
						settimeofday(&tv, NULL);

						_dprintf("new    = %lu\n", tv.tv_sec);

						strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z", localtime(&tv.tv_sec));
						sprintf(q, "Time Updated: %s [%s%lds]", s, diff > 0 ? "+" : "", diff);
					}
					else {
						t = time(0);
						strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z", localtime(&t));
						sprintf(q, "Time: %s, no change was needed.", s);
					}
					printf("\n\n%s\n", q);
					syslog(LOG_INFO, q);
					return 0;
				}
			}
		}
	}

	close(fd);
	return 1;
}


// -----------------------------------------------------------------------------


static int ntpc_main(int argc, char **argv)
{
	struct hostent *he;
	struct in_addr ia;
	const char *s;
	int i;

	for (i = 1; i < argc; ++i) {
		if ((he = gethostbyname(argv[i])) != NULL) {
			memcpy(&ia, he->h_addr_list[0], sizeof(ia));
			s = inet_ntoa(ia);
			if (strcmp(s, argv[i]) == 0) printf("Trying %s: ", s);
				else printf("Trying %s [%s]:", argv[i], s);
			if (ntpc(ia) == 0) return 0;
		}
		else {
			printf("Unable to resolve: %s\n", argv[i]);
		}
	}

	if (argc < 2) {
		printf("Usage: ntpc <server>\n");
	}

	return 1;
}


// -----------------------------------------------------------------------------


static int ntpsync_main(int argc, char **argv)
{
	char *servers, *t, *p;
	char *addr[10];
	char *ips;
	char *nvkiss;
	int count;
	int i;
	struct hostent *he;
	struct in_addr ia;
	int retries;
	int nu;
	char s[512];
	time_t tt;
	struct tm *tms;
	enum { USER, INIT, CRON } mode;

	nu = nvram_get_int("ntp_updates");

	mode = USER;
	if (argc == 2) {
		if (strcmp(argv[1], "--cron") == 0) {		// try for a few minutes
			if (nu <= 0) {
				eval("cru", "d", "ntpsync");
				return 0;
			}
			mode = CRON;
		}
		else if (strcmp(argv[1], "--init") == 0) {	// try forever, schedule cron job
			if (fork() != 0) return 0;
			mode = INIT;
		}
		else {
			return 1;
		}
	}
	else if (argc != 1) {
		return 1;
	}

	_dprintf("[ntpsync %ld] start\n", get_uptime());

	if ((mode != INIT) && (nvram_match("ntp_tdod", "0")) && (
		((!check_wanup("wan")) && (get_wan_proto() != WP_DISABLED))
		|| (check_wanup("wan2") && (get_wanx_proto("wan2") != WP_DISABLED))
#ifdef TCONFIG_MULTIWAN
		|| (check_wanup("wan3") && (get_wanx_proto("wan3") != WP_DISABLED))
		|| (check_wanup("wan4") && (get_wanx_proto("wan4") != WP_DISABLED))
#endif
		)) {
		_dprintf("WAN is down, not updating.");
		return 1;
	}

	srand(time(0));
	retries = 0;

	while (1) {
		_dprintf("[ntpsync %ld] while\n", get_uptime());

		count = 0;

		servers = p = strdup(nvram_safe_get("ntp_server"));

		if (!servers) {
			printf("Not enough memory\n");
			return 1;
		}
		while ((count < 10) && ((t = strsep(&p, " ")) != NULL)) {
			if (*t != 0) addr[count++] = t;
		}
		if (count == 0) addr[count++] = "pool.ntp.org";

		while (count > 0) {
			i = (rand() % count);
			_dprintf("[ntpsync] i=%d addr=%s\n", i, addr[i]);

			if ((he = gethostbyname(addr[i])) != NULL) {
				memcpy(&ia, he->h_addr_list[0], sizeof(ia));
				ips = inet_ntoa(ia);
				_dprintf("ip = %s\n", ips);

				nvkiss = nvram_safe_get("ntp_kiss");
				if (find_word(nvkiss, ips)) {
					_dprintf("kiss: %s\n", ips);
				}
				else {
					switch (ntpc(ia)) {
					case 0:
						_dprintf("[ntpsync] %ld OK\n", get_uptime());
						if (mode == INIT) {
							tt = time(0);
							if ((nu > 0) && ((tms = localtime(&tt)) != NULL)) {

								// add some randomness to make the servers happier / avoid the xx:00 rush
								sprintf(s, "%d ", (tms->tm_min + 20 + (rand() % 20)) % 60);

								// schedule every nu hours
								for (i = 0; i < 24; ++i) {
									if ((i % nu) == 0) sprintf(s + strlen(s), "%s%d", i ? "," : "", (i + tms->tm_hour + 1)  % 24);
								}
								strcat(s, " * * * ntpsync --cron");
								eval("cru", "a", "ntpsync", s);
							}
						}

						// make sure access restriction is ok
						eval("rcheck");
						_dprintf("[ntpsync] %ld exit\n", get_uptime());
						return 0;
					case 2:
						while ((nvkiss) && (strlen(nvkiss) > 128)) nvkiss = strchr(nvkiss + 1, ' ');
						if (nvkiss) strlcpy(s, nvkiss, sizeof(s));
							else s[0] = 0;
						add_word(s, ips, sizeof(s));
						nvram_set("ntp_kiss", s);
						syslog(LOG_WARNING, "Received a kiss of death packet from %s (%s).", addr[i], ips);
						break;
					}
				}
			}

			addr[i] = addr[--count];
		}
		free(servers);

		if (mode == USER) break;
		if ((mode == CRON) && (retries == 5)) break;

		if (++retries > 300) retries = 300;		// 5m
		_dprintf("[ntpsync %ld] sleep=%d\n", get_uptime(), retries);
		sleep(retries);
	}

	_dprintf("[ntpsync] %ld exit\n", get_uptime());
	return 1;
}

// -----------------------------------------------------------------------------


static int ntpstep_main(int argc, char **argv)
{
	struct timeval tv;
	char s[256];
	
	if (argc != 2) {
		printf("Usage: ntpstep <seconds>\n");
		return 1;
	}
	
	gettimeofday(&tv, NULL);
	tv.tv_sec += atol(argv[1]);
	settimeofday(&tv, NULL);

	strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z", localtime(&tv.tv_sec));
	printf("%s\n", s);
	return 0;
}


// -----------------------------------------------------------------------------

int main(int argc, char **argv)
{
	openlog("ntpc", LOG_PID, LOG_USER);
	if (!nvram_contains_word("log_events", "ntp")) {
		setlogmask(LOG_MASK(LOG_EMERG));	// can't set to 0
	}

	if (strstr(argv[0], "ntpsync") != NULL) return ntpsync_main(argc, argv);
	if (strstr(argv[0], "ntpstep") != NULL) return ntpstep_main(argc, argv);

	return ntpc_main(argc, argv);
}


/*

                       1                   2                    3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0 1
  +---+-----+-----+---------------+---------------+----------------+
  |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    | 0
  +---+-----+-----+---------------+---------------+----------------+
  |                          Root  Delay                           | 1
  +----------------------------------------------------------------+
  |                       Root  Dispersion                         | 2
  +----------------------------------------------------------------+
  |                     Reference Identifier                       | 3
  +----------------------------------------------------------------+
  |                    Reference Timestamp (64)                    | 4
  +----------------------------------------------------------------+
  |                    Originate Timestamp (64)                    | 6
  +----------------------------------------------------------------+
  |                     Receive Timestamp (64)                     | 8
  +----------------------------------------------------------------+
  |                     Transmit Timestamp (64)                    | 10
  +----------------------------------------------------------------+
  |                 Key Identifier (optional) (32)                 | 12
  +----------------------------------------------------------------+
  |                 Message Digest (optional) (128)                | 13+
  +----------------------------------------------------------------+

timestamp:
	since 1900

1970-1900:
	25,567 days can be converted to one of these units:
		* 2,208,988,800 seconds
		* 36,816,480 minutes
		* 613,608 hours
		* 3652 weeks (rounded down)

refs:
	http://www.faqs.org/rfcs/rfc2030.html
	http://www.ntp.org/ntpfaq/NTP-s-algo.htm

*/
