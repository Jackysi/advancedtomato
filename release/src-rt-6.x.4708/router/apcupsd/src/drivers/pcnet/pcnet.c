/*
 * pcnet.c
 *
 * Driver for PowerChute Network Shutdown protocol.
 */

/*
 * Copyright (C) 2006 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "apc.h"
#include "md5.h"
#include <sys/socket.h>
#include <netinet/in.h>

/* UPS broadcasts status packets to UDP port 3052 */
#define PCNET_DEFAULT_PORT 3052

/*
 * Number of seconds with no data before we declare COMMLOST.
 * UPS should report in every 25 seconds. We allow 2 missing
 * reports plus a fudge factor.
 */
#define COMMLOST_TIMEOUT   55

/* Win32 needs a special close for sockets */
#ifdef HAVE_MINGW
#define close(fd) closesocket(fd)
#endif

typedef struct {
   char device[MAXSTRING];             /* Copy of ups->device */
   char *ipaddr;                       /* IP address of UPS */
   char *user;                         /* Username */
   char *pass;                         /* Pass phrase */
   bool auth;                          /* Authenticate? */
   unsigned long uptime;               /* UPS uptime counter */
   unsigned long reboots;              /* UPS reboot counter */
   time_t datatime;                    /* Last time we got valid data */
} PCNET_DATA;

/* Convert UPS response to enum and string */
static SelfTestResult decode_testresult(const char* str)
{
   /*
    * Responses are:
    * "OK" - good battery, 
    * "BT" - failed due to insufficient capacity, 
    * "NG" - failed due to overload, 
    * "NO" - no results available (no test performed in last 5 minutes) 
    */
   if (str[0] == 'O' && str[1] == 'K')
      return TEST_PASSED;
   else if (str[0] == 'B' && str[1] == 'T')
      return TEST_FAILCAP;
   else if (str[0] == 'N' && str[1] == 'G')
      return TEST_FAILLOAD;

   return TEST_NONE;
}

/* Convert UPS response to enum and string */
static LastXferCause decode_lastxfer(const char *str)
{
   Dmsg1(80, "Transfer reason: %c\n", *str);

   switch (*str) {
   case 'N':
      return XFER_NA;
   case 'R':
      return XFER_RIPPLE;
   case 'H':
      return XFER_OVERVOLT;
   case 'L':
      return XFER_UNDERVOLT;
   case 'T':
      return XFER_NOTCHSPIKE;
   case 'O':
      return XFER_NONE;
   case 'K':
      return XFER_FORCED;
   case 'S':
      return XFER_SELFTEST;
   default:
      return XFER_UNKNOWN;
   }
}

static bool pcnet_process_data(UPSINFO* ups, const char *key, const char *value)
{
   unsigned long cmd;
   int ci;
   bool ret;

   /* Make sure we have a value */
   if (*value == '\0')
      return false;

   /* Detect remote shutdown command */
   if (strcmp(key, "SD") == 0)
   {
      cmd = strtoul(value, NULL, 10);
      switch (cmd)
      {
      case 0:
         Dmsg0(80, "SD: The UPS is NOT shutting down\n");
         ups->clear_shut_remote();
         break;

      case 1:
         Dmsg0(80, "SD: The UPS is shutting down\n");
         ups->set_shut_remote();
         break;

      default:
         Dmsg1(80, "Unrecognized SD value %s!\n", value);
         break;
      }
 
      return true;
   }

   /* Key must be 2 hex digits */
   if (!isxdigit(key[0]) || !isxdigit(key[1]))
      return false;

   /* Convert command to CI */
   cmd = strtoul(key, NULL, 16);
   for (ci=0; ci<CI_MAXCI; ci++)
      if (ups->UPS_Cmd[ci] == cmd)
         break;

   /* No match? */
   if (ci == CI_MAXCI)
      return false;

   /* Mark this CI as available */
   ups->UPS_Cap[ci] = true;

   /* Handle the data */
   ret = true;
   switch (ci) {
      /*
       * VOLATILE DATA
       */
   case CI_STATUS:
      Dmsg1(80, "Got CI_STATUS: %s\n", value);
      ups->Status &= ~0xFF;        /* clear APC byte */
      ups->Status |= strtoul(value, NULL, 16) & 0xFF;  /* set APC byte */
      break;
   case CI_LQUAL:
      Dmsg1(80, "Got CI_LQUAL: %s\n", value);
      astrncpy(ups->linequal, value, sizeof(ups->linequal));
      break;
   case CI_WHY_BATT:
      Dmsg1(80, "Got CI_WHY_BATT: %s\n", value);
      ups->lastxfer = decode_lastxfer(value);
      break;
   case CI_ST_STAT:
      Dmsg1(80, "Got CI_ST_STAT: %s\n", value);
      ups->testresult = decode_testresult(value);
      break;
   case CI_VLINE:
      Dmsg1(80, "Got CI_VLINE: %s\n", value);
      ups->LineVoltage = atof(value);
      break;
   case CI_VMIN:
      Dmsg1(80, "Got CI_VMIN: %s\n", value);
      ups->LineMin = atof(value);
      break;
   case CI_VMAX:
      Dmsg1(80, "Got CI_VMAX: %s\n", value);
      ups->LineMax = atof(value);
      break;
   case CI_VOUT:
      Dmsg1(80, "Got CI_VOUT: %s\n", value);
      ups->OutputVoltage = atof(value);
      break;
   case CI_BATTLEV:
      Dmsg1(80, "Got CI_BATTLEV: %s\n", value);
      ups->BattChg = atof(value);
      break;
   case CI_VBATT:
      Dmsg1(80, "Got CI_VBATT: %s\n", value);
      ups->BattVoltage = atof(value);
      break;
   case CI_LOAD:
      Dmsg1(80, "Got CI_LOAD: %s\n", value);
      ups->UPSLoad = atof(value);
      break;
   case CI_FREQ:
      Dmsg1(80, "Got CI_FREQ: %s\n", value);
      ups->LineFreq = atof(value);
      break;
   case CI_RUNTIM:
      Dmsg1(80, "Got CI_RUNTIM: %s\n", value);
      ups->TimeLeft = atof(value);
      break;
   case CI_ITEMP:
      Dmsg1(80, "Got CI_ITEMP: %s\n", value);
      ups->UPSTemp = atof(value);
      break;
   case CI_DIPSW:
      Dmsg1(80, "Got CI_DIPSW: %s\n", value);
      ups->dipsw = strtoul(value, NULL, 16);
      break;
   case CI_REG1:
      Dmsg1(80, "Got CI_REG1: %s\n", value);
      ups->reg1 = strtoul(value, NULL, 16);
      break;
   case CI_REG2:
      ups->reg2 = strtoul(value, NULL, 16);
      ups->set_battpresent(!(ups->reg2 & 0x20));
      break;
   case CI_REG3:
      Dmsg1(80, "Got CI_REG3: %s\n", value);
      ups->reg3 = strtoul(value, NULL, 16);
      break;
   case CI_HUMID:
      Dmsg1(80, "Got CI_HUMID: %s\n", value);
      ups->humidity = atof(value);
      break;
   case CI_ATEMP:
      Dmsg1(80, "Got CI_ATEMP: %s\n", value);
      ups->ambtemp = atof(value);
      break;
   case CI_ST_TIME:
      Dmsg1(80, "Got CI_ST_TIME: %s\n", value);
      ups->LastSTTime = atof(value);
      break;
      
      /*
       * STATIC DATA
       */
   case CI_SENS:
      Dmsg1(80, "Got CI_SENS: %s\n", value);
      astrncpy(ups->sensitivity, value, sizeof(ups->sensitivity));
      break;
   case CI_DWAKE:
      Dmsg1(80, "Got CI_DWAKE: %s\n", value);
      ups->dwake = (int)atof(value);
      break;
   case CI_DSHUTD:
      Dmsg1(80, "Got CI_DSHUTD: %s\n", value);
      ups->dshutd = (int)atof(value);
      break;
   case CI_LTRANS:
      Dmsg1(80, "Got CI_LTRANS: %s\n", value);
      ups->lotrans = (int)atof(value);
      break;
   case CI_HTRANS:
      Dmsg1(80, "Got CI_HTRANS: %s\n", value);
      ups->hitrans = (int)atof(value);
      break;
   case CI_RETPCT:
      Dmsg1(80, "Got CI_RETPCT: %s\n", value);
      ups->rtnpct = (int)atof(value);
      break;
   case CI_DALARM:
      Dmsg1(80, "Got CI_DALARM: %s\n", value);
      astrncpy(ups->beepstate, value, sizeof(ups->beepstate));
      break;
   case CI_DLBATT:
      Dmsg1(80, "Got CI_DLBATT: %s\n", value);
      ups->dlowbatt = (int)atof(value);
      break;
   case CI_IDEN:
      Dmsg1(80, "Got CI_IDEN: %s\n", value);
      if (ups->upsname[0] == 0)
         astrncpy(ups->upsname, value, sizeof(ups->upsname));
      break;
   case CI_STESTI:
      Dmsg1(80, "Got CI_STESTI: %s\n", value);
      astrncpy(ups->selftest, value, sizeof(ups->selftest));
      break;
   case CI_MANDAT:
      Dmsg1(80, "Got CI_MANDAT: %s\n", value);
      astrncpy(ups->birth, value, sizeof(ups->birth));
      break;
   case CI_SERNO:
      Dmsg1(80, "Got CI_SERNO: %s\n", value);
      astrncpy(ups->serial, value, sizeof(ups->serial));
      break;
   case CI_BATTDAT:
      Dmsg1(80, "Got CI_BATTDAT: %s\n", value);
      astrncpy(ups->battdat, value, sizeof(ups->battdat));
      break;
   case CI_NOMOUTV:
      Dmsg1(80, "Got CI_NOMOUTV: %s\n", value);
      ups->NomOutputVoltage = (int)atof(value);
      break;
   case CI_NOMBATTV:
      Dmsg1(80, "Got CI_NOMBATTV: %s\n", value);
      ups->nombattv = atof(value);
      break;
   case CI_REVNO:
      Dmsg1(80, "Got CI_REVNO: %s\n", value);
      astrncpy(ups->firmrev, value, sizeof(ups->firmrev));
      break;
   case CI_EXTBATTS:
      Dmsg1(80, "Got CI_EXTBATTS: %s\n", value);
      ups->extbatts = (int)atof(value);
      break;
   case CI_BADBATTS:
      Dmsg1(80, "Got CI_BADBATTS: %s\n", value);
      ups->badbatts = (int)atof(value);
      break;
   case CI_UPSMODEL:
      Dmsg1(80, "Got CI_UPSMODEL: %s\n", value);
      astrncpy(ups->upsmodel, value, sizeof(ups->upsmodel));
      break;
   case CI_EPROM:
      Dmsg1(80, "Got CI_EPROM: %s\n", value);
      astrncpy(ups->eprom, value, sizeof(ups->eprom));
      break;
   default:
      Dmsg1(100, "Unknown CI (%d)\n", ci);
      ret = false;
      break;
   }
   
   return ret;
}

static char *digest2ascii(md5_byte_t *digest)
{
   static char ascii[33];
   char *ptr;
   int idx;

   /* Convert binary digest to ascii */
   ptr = ascii;
   for (idx=0; idx<16; idx++) {
      sprintf(ptr, "%02x", (unsigned char)digest[idx]);
      ptr += 2;
   }

   return ascii;
}

struct pair {
   const char* key;
   const char* value;
};

#define MAX_PAIRS 256

static const char *lookup_key(const char *key, struct pair table[])
{
   int idx;
   const char *ret = NULL;

   for (idx=0; table[idx].key; idx++) {
      if (strcmp(key, table[idx].key) == 0) {
         ret = table[idx].value;
         break;
      }
   }

   return ret;
}

static struct pair *auth_and_map_packet(UPSINFO* ups, char *buf, int len)
{
   PCNET_DATA *my_data = (PCNET_DATA *)ups->driver_internal_data;
   char *key, *end, *ptr, *value;
   const char *val, *hash=NULL;
   static struct pair pairs[MAX_PAIRS+1];
   md5_state_t ms;
   md5_byte_t digest[16];
   unsigned int idx;
   unsigned long uptime, reboots;

   /* If there's no MD= field, drop the packet */
   if ((ptr = strstr(buf, "MD=")) == NULL || ptr == buf)
      return NULL;

   if (my_data->auth) {
      /* Calculate the MD5 of the packet before messing with it */
      md5_init(&ms);
      md5_append(&ms, (md5_byte_t*)buf, ptr-buf);
      md5_append(&ms, (md5_byte_t*)my_data->user, strlen(my_data->user));
      md5_append(&ms, (md5_byte_t*)my_data->pass, strlen(my_data->pass));
      md5_finish(&ms, digest);

      /* Convert binary digest to ascii */
      hash = digest2ascii(digest);
   }

   /* Build a table of pointers to key/value pairs */
   memset(pairs, 0, sizeof(pairs));
   ptr = buf;
   idx = 0;
   while (*ptr && idx < MAX_PAIRS) {
      /* Find the beginning of the line */
      while (isspace(*ptr))
         ptr++;
      key = ptr;

      /* Find the end of the line */
      while (*ptr && *ptr != '\r' && *ptr != '\n')
         ptr++;
      end = ptr;
      if (*ptr != '\0')
         ptr++;

      /* Remove trailing whitespace */
      do {
         *end-- = '\0';
      } while (end >= key && isspace(*end));

      Dmsg1(300, "process_packet: line='%s'\n", key);

      /* Split the string */
      if ((value = strchr(key, '=')) == NULL)
         continue;
      *value++ = '\0';

      Dmsg2(300, "process_packet: key='%s' value='%s'\n",
         key, value);

      /* Save key/value in table */
      pairs[idx].key = key;
      pairs[idx].value = value;
      idx++;
   }

   if (my_data->auth) {
      /* Check calculated hash vs received */
      Dmsg1(200, "process_packet: calculated=%s\n", hash);
      val = lookup_key("MD", pairs);
      if (!val || strcmp(hash, val)) {
         Dmsg0(200, "process_packet: message hash failed\n");
         return NULL;
      }
      Dmsg1(200, "process_packet: message hash passed\n", val);

      /* Check management card IP address */
      val = lookup_key("PC", pairs);
      if (!val) {
         Dmsg0(200, "process_packet: Missing PC field\n");
         return NULL;
      }
      Dmsg1(200, "process_packet: Expected IP=%s\n", my_data->ipaddr);
      Dmsg1(200, "process_packet: Received IP=%s\n", val);
      if (strcmp(val, my_data->ipaddr)) {
         Dmsg2(200, "process_packet: IP address mismatch\n",
            my_data->ipaddr, val);
         return NULL;
      }
   }

   /*
    * Check that uptime and/or reboots have advanced. If not,
    * this packet could be out of order, or an attacker may
    * be trying to replay an old packet.
    */
   val = lookup_key("SR", pairs);
   if (!val) {
      Dmsg0(200, "process_packet: Missing SR field\n");
      return NULL;
   }
   reboots = strtoul(val, NULL, 16);

   val = lookup_key("SU", pairs);
   if (!val) {
      Dmsg0(200, "process_packet: Missing SU field\n");
      return NULL;
   }
   uptime = strtoul(val, NULL, 16);

   Dmsg1(200, "process_packet: Our reboots=%d\n", my_data->reboots);
   Dmsg1(200, "process_packet: UPS reboots=%d\n", reboots);
   Dmsg1(200, "process_packet: Our uptime=%d\n", my_data->uptime);
   Dmsg1(200, "process_packet: UPS uptime=%d\n", uptime);

   if ((reboots == my_data->reboots && uptime <= my_data->uptime) ||
       (reboots < my_data->reboots)) {
      Dmsg0(200, "process_packet: Packet is out of order or replayed\n");
      return NULL;
   }

   my_data->reboots = reboots;
   my_data->uptime = uptime;
   return pairs;
}

/*
 * Read UPS events. I.e. state changes.
 */
int pcnet_ups_check_state(UPSINFO *ups)
{
   PCNET_DATA *my_data = (PCNET_DATA *)ups->driver_internal_data;
   struct timeval tv, now, exit;
   fd_set rfds;
   bool done = false;
   struct sockaddr_in from;
   socklen_t fromlen;
   int retval;
   char buf[4096];
   struct pair *map;
   int idx;

   /* Figure out when we need to exit by */
   gettimeofday(&exit, NULL);
   exit.tv_sec += ups->wait_time;

   while (!done) {

      /* Figure out how long until we have to exit */
      gettimeofday(&now, NULL);

      if (now.tv_sec > exit.tv_sec ||
         (now.tv_sec == exit.tv_sec &&
            now.tv_usec >= exit.tv_usec)) {
         /* Done already? How time flies... */
         break;
      }

      tv.tv_sec = exit.tv_sec - now.tv_sec;
      tv.tv_usec = exit.tv_usec - now.tv_usec;
      if (tv.tv_usec < 0) {
         tv.tv_sec--;              /* Normalize */
         tv.tv_usec += 1000000;
      }

      Dmsg2(100, "Waiting for %d.%d\n", tv.tv_sec, tv.tv_usec);
      FD_ZERO(&rfds);
      FD_SET(ups->fd, &rfds);

      retval = select(ups->fd + 1, &rfds, NULL, NULL, &tv);

      if (retval == 0) {
         /* No chars available in TIMER seconds. */
         break;
      } else if (retval == -1) {
         if (errno == EINTR || errno == EAGAIN)         /* assume SIGCHLD */
            continue;
         Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
         return 0;
      }

      do {
         fromlen = sizeof(from);
         retval = recvfrom(ups->fd, buf, sizeof(buf)-1, 0, (struct sockaddr*)&from, &fromlen);
      } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

      if (retval < 0) {            /* error */
         Dmsg1(200, "recvfrom error: ERR=%s\n", strerror(errno));
//         usb_link_check(ups);      /* notify that link is down, wait */
         break;
      }

      Dmsg4(200, "Packet from: %d.%d.%d.%d\n",
         (ntohl(from.sin_addr.s_addr) >> 24) & 0xff,
         (ntohl(from.sin_addr.s_addr) >> 16) & 0xff,
         (ntohl(from.sin_addr.s_addr) >> 8) & 0xff,
         ntohl(from.sin_addr.s_addr) & 0xff);

      /* Ensure the packet is nul-terminated */
      buf[retval] = '\0';

      hex_dump(300, buf, retval);

      map = auth_and_map_packet(ups, buf, retval);
      if (map == NULL)
         continue;

      write_lock(ups);

      for (idx=0; map[idx].key; idx++)
         done |= pcnet_process_data(ups, map[idx].key, map[idx].value);

      write_unlock(ups);
   }

   /* If we successfully received a data packet, update timer. */
   if (done) {
      time(&my_data->datatime);
      Dmsg1(100, "Valid data at time_t=%d\n", my_data->datatime);
   }

   return done;
}

int pcnet_ups_open(UPSINFO *ups)
{
   struct sockaddr_in addr;
   PCNET_DATA *my_data = (PCNET_DATA *)ups->driver_internal_data;
   char *ptr;

   write_lock(ups);

   if (my_data == NULL) {
      my_data = (PCNET_DATA *)malloc(sizeof(*my_data));
      memset(my_data, 0, sizeof(*my_data));
      ups->driver_internal_data = my_data;
   }

   unsigned short port = PCNET_DEFAULT_PORT;
   if (ups->device[0] != '\0') {
      my_data->auth = true;

      astrncpy(my_data->device, ups->device, sizeof(my_data->device));
      ptr = my_data->device;

      my_data->ipaddr = ptr;
      ptr = strchr(ptr, ':');
      if (ptr == NULL)
         Error_abort0("Malformed DEVICE [ip:user:pass]\n");
      *ptr++ = '\0';
      
      my_data->user = ptr;
      ptr = strchr(ptr, ':');
      if (ptr == NULL)
         Error_abort0("Malformed DEVICE [ip:user:pass]\n");
      *ptr++ = '\0';

      my_data->pass = ptr;
      if (*ptr == '\0')
         Error_abort0("Malformed DEVICE [ip:user:pass]\n");

      // Last segment is optional port number
      ptr = strchr(ptr, ':');
      if (ptr)
      {
         *ptr++ = '\0';
         port = atoi(ptr);
         if (port == 0)
            port = PCNET_DEFAULT_PORT;
      }
   }

   ups->fd = socket(PF_INET, SOCK_DGRAM, 0);
   if (ups->fd == -1)
      Error_abort1("Cannot create socket (%d)\n", errno);

   int enable = 1;
   setsockopt(ups->fd, SOL_SOCKET, SO_BROADCAST, (const char*)&enable, sizeof(enable));

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if (bind(ups->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
      close(ups->fd);
      Error_abort1("Cannot bind socket (%d)\n", errno);
   }

   /* Reset datatime to now */
   time(&my_data->datatime);

   write_unlock(ups);
   return 1;
}

int pcnet_ups_setup(UPSINFO *ups)
{
   /* Seems that there is nothing to do. */
   return 1;
}

int pcnet_ups_close(UPSINFO *ups)
{
   write_lock(ups);
   
   close(ups->fd);
   ups->fd = -1;

   write_unlock(ups);
   return 1;
}

/*
 * Setup capabilities structure for UPS
 */
int pcnet_ups_get_capabilities(UPSINFO *ups)
{
   /*
    * Unfortunately, we don't know capabilities until we
    * receive the first broadcast status message.
    */
   return 1;
}

/*
 * Read UPS info that remains unchanged -- e.g. transfer
 * voltages, shutdown delay, ...
 *
 * This routine is called once when apcupsd is starting
 */
int pcnet_ups_read_static_data(UPSINFO *ups)
{
   /* All our data gathering is done in pcnet_ups_check_state() */
   return 1;
}

/*
 * Read UPS info that changes -- e.g. Voltage, temperature, ...
 *
 * This routine is called once every N seconds to get
 * a current idea of what the UPS is doing.
 */
int pcnet_ups_read_volatile_data(UPSINFO *ups)
{
   PCNET_DATA *my_data = (PCNET_DATA *)ups->driver_internal_data;
   time_t now, diff;
   
   /*
    * All our data gathering is done in pcnet_ups_check_state().
    * But we do use this function to check our commlost state.
    */

   time(&now);
   diff = now - my_data->datatime;

   if (ups->is_commlost()) {
      if (diff < COMMLOST_TIMEOUT) {
         generate_event(ups, CMDCOMMOK);
         ups->clear_commlost();
      }
   } else {
      if (diff >= COMMLOST_TIMEOUT) {
         generate_event(ups, CMDCOMMFAILURE);
         ups->set_commlost();
      }
   }

   return 1;
}

int pcnet_ups_kill_power(UPSINFO *ups)
{
   PCNET_DATA *my_data = (PCNET_DATA *)ups->driver_internal_data;
   struct sockaddr_in addr;
   char data[1024];
   int s, len=0, temp=0;
   char *start;
   const char *cs, *hash;
   struct pair *map;
   md5_state_t ms;
   md5_byte_t digest[16];

   /* We cannot perform a killpower without authentication data */
   if (!my_data->auth) {
      Error_abort0("Cannot perform killpower without authentication "
                   "data. Please set ip:user:pass for DEVICE in "
                   "apcupsd.conf.\n");
   }

   /* Open a TCP stream to the UPS */
   s = socket(PF_INET, SOCK_STREAM, 0);
   if (s == -1) {
      Dmsg1(100, "pcnet_ups_kill_power: Unable to open socket: %s\n",
         strerror(errno));
      return 0;
   }

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(80);
   inet_pton(AF_INET, my_data->ipaddr, &addr.sin_addr.s_addr);

   if (connect(s, (sockaddr*)&addr, sizeof(addr))) {
      Dmsg3(100, "pcnet_ups_kill_power: Unable to connect to %s:%d: %s\n",
         my_data->ipaddr, 80, strerror(errno));
      close(s);
      return 0;
   }

   /* Send a simple HTTP request for "/macontrol.htm". */
   asnprintf(data, sizeof(data),
      "GET /macontrol.htm HTTP/1.1\r\n"
      "Host: %s\r\n"
      "\r\n",
      my_data->ipaddr);

   Dmsg1(200, "Request:\n---\n%s---\n", data);

   if (send(s, data, strlen(data), 0) != (int)strlen(data)) {
      Dmsg1(100, "pcnet_ups_kill_power: send failed: %s\n", strerror(errno));
      close(s);
      return 0;
   }

   /*
    * Clear buffer and read data until we find the 0-length
    * chunk. We know that AP9617 uses chunked encoding, so we
    * can count on the 0-length chunk at the end.
    */
   memset(data, 0, sizeof(data));
   do {
      len += temp;
      temp = recv(s, data+len, sizeof(data)-len, 0);
   } while(temp > 0 && strstr(data, "\r\n0\r\n") == NULL);

   Dmsg1(200, "Response:\n---\n%s---\n", data);

   if (temp < 0) {
      Dmsg1(100, "pcnet_ups_kill_power: recv failed: %s\n", strerror(errno));
      close(s);
      return 0;
   }

   /*
    * Find "<html>" since that's where the real authenticated
    * data begins. Everything before that is headers. 
    */
   start = strstr(data, "<html>");
   if (start == NULL) {
      Dmsg0(100, "pcnet_ups_kill_power: Malformed data\n");
      close(s);
      return 0;
   }

   /*
    * Authenticate and map the packet contents. This will
    * extract all key/value pairs and ensure the packet 
    * authentication hash is valid.
    */
   map = auth_and_map_packet(ups, start, strlen(start));
   if (map == NULL) {
      close(s);
      return 0;
   }

   /* Check that we got a challenge string. */
   cs = lookup_key("CS", map);
   if (cs == NULL) {
      Dmsg0(200, "pcnet_ups_kill_power: Missing CS field\n");
      close(s);
      return 0;
   }

   /*
    * Now construct the hash of the packet we're about to
    * send using the challenge string from the packet we
    * just received, plus our username and passphrase.
    */
   md5_init(&ms);
   md5_append(&ms, (md5_byte_t*)"macontrol1_control_shutdown_1=1,", 32);
   md5_append(&ms, (md5_byte_t*)cs, strlen(cs));
   md5_append(&ms, (md5_byte_t*)my_data->user, strlen(my_data->user));
   md5_append(&ms, (md5_byte_t*)my_data->pass, strlen(my_data->pass));
   md5_finish(&ms, digest);
   hash = digest2ascii(digest);

   /* Send the shutdown request */
   asnprintf(data, sizeof(data),
      "POST /Forms/macontrol1 HTTP/1.1\r\n"
      "Host: %s\r\n"
      "Content-Type: application/x-www-form-urlencoded\r\n"
      "Content-Length: 72\r\n"
      "\r\n"
      "macontrol1%%5fcontrol%%5fshutdown%%5f1=1%%2C%s",
      my_data->ipaddr, hash);

   Dmsg2(200, "Request: (strlen=%d)\n---\n%s---\n", strlen(data), data);

   if (send(s, data, strlen(data), 0) != (int)strlen(data)) {
      Dmsg1(100, "pcnet_ups_kill_power: send failed: %s\n", strerror(errno));
      close(s);
      return 0;
   }

   /* That's it, we're done. */
   close(s);

   return 1;
}

int pcnet_ups_program_eeprom(UPSINFO *ups, int command, const char *data)
{
   /* Unsupported */
   return 0;
}

int pcnet_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   int temp;

   switch (command) {
   case DEVICE_CMD_CHECK_SELFTEST:
      Dmsg0(80, "Checking self test.\n");
      if (ups->UPS_Cap[CI_WHY_BATT] && ups->lastxfer == XFER_SELFTEST) {
         /*
          * set Self Test start time
          */
         ups->SelfTest = time(NULL);
         Dmsg1(80, "Self Test time: %s", ctime(&ups->SelfTest));
      }
      break;

   case DEVICE_CMD_GET_SELFTEST_MSG:
      /*
       * This is a bit kludgy. The selftest result isn't available from
       * the UPS for about 10 seconds after the selftest completes. So we
       * invoke pcnet_ups_check_state() with a 12 second timeout, 
       * expecting that it should get a status report before then.
       */

      /* Save current ups->wait_time and set it to 12 seconds */
      temp = ups->wait_time;
      ups->wait_time = 12;
      
      /* Let check_status wait for the result */
      write_unlock(ups);
      pcnet_ups_check_state(ups);
      write_lock(ups);

      /* Restore ups->wait_time */
      ups->wait_time = temp;
      break;

   default:
      return FAILURE;
   }

   return SUCCESS;
}
