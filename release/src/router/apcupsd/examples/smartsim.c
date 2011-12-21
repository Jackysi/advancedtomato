/*
 * smartsim.c
 *
 * A SmartUPS serial protocol simulator.
 *
 * This is a basic SmartUPS protocol simulator. It answers
 * queries from the host (i.e., apcuspd) and allows you to toggle
 * flags such as onbattery and adjust readouts such as timeleft.
 * This can be very useful for exercising apcupsd with a series
 * of events that would otherwise require draining your UPS battery
 * repeatedly.
 *
 * Point smartsim at a serial port (/dev/ttyS0 is the default) and
 * connect that port via a null modem cable to another serial port
 * on which you are running apcupsd configured for the apcsmart
 * driver.
 *
 * smartsim responds to the Smart protocol commands listed in the
 * upscmds[] table. More protocol commands can easily be added.
 *
 * Keyboard commands consisting of a single keypress cause smartsim
 * to toggle flags and readouts. These commands can be found in the
 * keycmds[] table. Supported keypress commands:
 *
 *    ?   Help
 *    b   Onbattery toggle
 *    o   Overload toggle
 *    t   Trim toggle
 *    s   Boost toggle
 *    l   BattLow toggle
 *    x   Selftest toggle
 *    r   ReplaceBatt toggle
 *    d   BattDetach toggle
 *    c   CommFail toggle
 *    7/4 BattPct (inc/dec)   (use numeric keypad)
 *    8/5 LoadPct (inc/dec)   (use numeric keypad)
 *    9/6 TimeLeft (inc/dec)  (use numeric keypad)
 *
 * Some keypress commands produce procotol events. For example,
 * toggling on/off battery changes the status variable and also
 * issues '!' and '$' "interrupt" events.
 */

/*
 * Copyright (C) 2005 Adam Kropelin
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define DEFAULT_LINEF      60.0
#define DEFAULT_LOADPCT    25.0
#define DEFAULT_BATTPCT    100.0
#define DEfAULT_TIMELEFT   20.0
#define DEFAULT_BATTV      24.0
#define DEFAULT_LINEV      120.0
#define DEFAULT_OUTV       120.0
#define DEFAULT_REG2       0x00

int debug = 1;
int ups = -1;

/* UPS variables */
float linefreq = DEFAULT_LINEF;
float loadpct = DEFAULT_LOADPCT;
float battpct = DEFAULT_BATTPCT;
float timeleft = DEfAULT_TIMELEFT;
float battv = DEFAULT_BATTV;
float linev = DEFAULT_LINEV;
float outv = DEFAULT_OUTV;
unsigned char reg2 = DEFAULT_REG2;
char xfercause[] = "O";
char selftest[] = "OK";
int onbatt = 0;
int online = 1;
int overload = 0;
int battlow = 0;
int rebatt = 0;
int trim = 0;
int boost = 0;
int commfail = 0;

#define dbg(str, args...)     \
do                            \
{                             \
   if (debug)                 \
   {                          \
      printf(str, ## args);   \
      fflush(stdout);         \
   }                          \
}                             \
while(0)

/* Response callbacks */
static void rsp_string(const void* arg);
static void rsp_float(const void* arg);
static void rsp_cmds(const void* arg);
static void rsp_status(const void* arg);
static void rsp_hex(const void *arg);

/* Mapping of UPS commands to response callbacks */
struct upscmd
{
   char cmd;
   void (*func)(const void*);
   const void* arg;
} upscmds[] =
{
   { 'Y',    rsp_string, "SM" },
   { '\x01', rsp_string, "SMART-UPS Simulator" },
   { 'c',    rsp_string, "SMARTSIM" },
   { 'F',    rsp_float,  &linefreq },
   { 'Q',    rsp_status, NULL },
   { 'P',    rsp_float,  &loadpct },
   { 'f',    rsp_float,  &battpct },
   { 'a',    rsp_cmds,   NULL },
   { 'j',    rsp_float,  &timeleft },
   { 'G',    rsp_string, xfercause },
   { 'X',    rsp_string, selftest },
   { 'B',    rsp_float,  &battv },
   { 'L',    rsp_float,  &linev },
   { 'O',    rsp_float,  &outv },
   { '\'',   rsp_hex,    &reg2 },
   { 'V',    rsp_string, "Lzy" },
   { '\0',   NULL,       NULL }
};

/* The alert characters we support */
const char alerts[] = "!$%+#";

static void key_toggle(const void* arg);
static void key_onbatt(const void* arg);
static void key_battlow(const void* arg);
static void key_inc(const void* arg);
static void key_dec(const void* arg);
static void key_selftest(const void* arg);
static void key_rebatt(const void* arg);
static void key_batdet(const void *arg);
static void key_commfail(const void *arg);
static void key_help(const void *arg);

/* Mapping of keyboard commands to callbacks */
struct keycmd
{
   char key;
   void (*func)(const void*);
   void* arg;
} keycmds[] =
{
   { 'b',  key_onbatt,    NULL },
   { 'o',  key_toggle,    &overload },
   { 't',  key_toggle,    &trim },
   { 's',  key_toggle,    &boost },
   { 'l',  key_battlow,   NULL },
   { '7',  key_inc,       &battpct },
   { '4',  key_dec,       &battpct },
   { '8',  key_inc,       &loadpct },
   { '5',  key_dec,       &loadpct },
   { '9',  key_inc,       &timeleft },
   { '6',  key_dec,       &timeleft },
   { 'x',  key_selftest,  NULL },
   { 'r',  key_rebatt,    NULL },
   { 'd',  key_batdet,    NULL },
   { 'c',  key_commfail,  NULL },
   { '?',  key_help,      NULL },
   { '\0', NULL,          NULL }
};

/* Defaults */
#define DEFAULT_DEVICE  "/dev/ttyS0";

void wups(const char* str, int len)
{
   if (debug)
      write(fileno(stdout), str, len);

   write(ups, str, len);
}

void rsp_string(const void* arg)
{
   wups((char*)arg, strlen((char*)arg));
   wups("\r\n", 2);
}

void rsp_float(const void* arg)
{
   char buf[20];

   sprintf(buf, "%03.3f\r\n", *(float*)arg);
   wups(buf, strlen(buf));
}

void rsp_hex(const void* arg)
{
   char buf[20];

   sprintf(buf, "%02x\r\n", *(unsigned char*)arg);
   wups(buf, strlen(buf));
}

void rsp_cmds(const void* arg)
{
   int x;

   /* Protocol version */
   wups("3.", 2);

   /* Supported alert characters */
   wups(alerts, sizeof(alerts)-1);
   wups(".", 1);

   /* Supported commands */
   for (x=0; upscmds[x].func; x++)
      wups(&upscmds[x].cmd, 1);

   wups("\r\n", 2);
}

void rsp_status(const void* arg)
{
   char buf[20];

   sprintf(buf, "%02x\r\n",
      (trim << 1) |
      (boost << 2) |
      (online << 3) |
      (onbatt << 4) |
      (overload << 5) |
      (battlow << 6) |
      (rebatt << 7));

   wups(buf, strlen(buf));
}

static void key_toggle(const void* arg)
{
   *(int *)arg = !*(int *)arg;
}

static void key_onbatt(const void* arg)
{
   if (!onbatt)
   {
      online = 0;
      onbatt = 1;
      xfercause[0] = 'L';
      linev = 0;
      dbg("ALERT: ");
      wups("!", 1);
      dbg("\n");
   }
   else
   {
      online = 1;
      onbatt = 0;
      linev = DEFAULT_LINEV;
      dbg("ALERT: ");
      wups("$", 1);
      dbg("\n");
   }
}

static void key_battlow(const void* arg)
{
   if (!battlow)
   {
      battlow = 1;
      dbg("ALERT: ");
      wups("%", 1);
      dbg("\n");
   }
   else
   {
      battlow = 0;
      dbg("ALERT: ");
      wups("+", 1);
      dbg("\n");
   }
}

static void key_inc(const void* arg)
{
   *(float*)arg += 1;
   dbg("%3.3f\n", *(float*)arg);
}

static void key_dec(const void* arg)
{
   *(float*)arg -= 1;
   dbg("%3.3f\n", *(float*)arg);
}

static void key_selftest(const void* arg)
{
   key_onbatt(NULL);

   if (onbatt)
      xfercause[0] = 'S';
}

static void key_rebatt(const void* arg)
{
   if (!rebatt)
   {
      rebatt = 1;
      dbg("ALERT: ");
      wups("#", 1);
      dbg("\n");
   }
   else
   {
      rebatt = 0;
   }
}

static void key_batdet(const void *arg)
{
   if (reg2 & 0x20)
      reg2 &= ~0x20;
   else
      reg2 |= 0x20;
}

static void key_commfail(const void *arg)
{
   if (commfail)
   {
      commfail = 0;
      dbg("COMMFAIL disabled\n");
   }
   else
   {
      commfail = 1;
      dbg("COMMFAIL enabled\n");
   }

}

static void key_help(const void *arg)
{
   dbg("Commands:\n");
   dbg("?   Help\n");
   dbg("b   Onbattery toggle\n");
   dbg("o   Overload toggle\n");
   dbg("t   Trim toggle\n");
   dbg("s   Boost toggle\n");
   dbg("l   BattLow toggle\n");
   dbg("x   Selftest toggle\n");
   dbg("r   ReplaceBatt toggle\n");
   dbg("d   BattDetach toggle\n");
   dbg("c   CommFail toggle\n");
   dbg("7/4 BattPct (inc/dec)   (use numeric keypad)\n");
   dbg("8/5 LoadPct (inc/dec)   (use numeric keypad)\n");
   dbg("9/6 TimeLeft (inc/dec)  (use numeric keypad)\n");
}

void handle_ups_cmd(char cmd)
{
   int x;

   if (commfail)
   {
      dbg("<COMMFAIL>\n");
      return;
   }

   for (x=0; upscmds[x].func; x++)
   {
      if (upscmds[x].cmd == cmd)
      {
         upscmds[x].func(upscmds[x].arg);
         break;
      }
   }

   if (!upscmds[x].func)
      rsp_string("NA");
}

void handle_key_cmd(char cmd)
{
   int x;

   for (x=0; keycmds[x].func; x++)
   {
      if (keycmds[x].key == cmd)
      {
         keycmds[x].func(keycmds[x].arg);
         break;
      }
   }
}

#define max(a,b) \
   ( ((a) > (b)) ? (a) : (b) )

int main(int argc, char* argv[])
{
   fd_set fds;
   int rc;
   char cmd;
   struct termios tio;
   const char* dev;
   int con;

   /* Allow serial port device to be supplied on command line */
   if (argc == 2)
      dev = argv[1];
   else
      dev = DEFAULT_DEVICE;

   /* Open serial port device */
   ups = open(dev, O_RDWR | O_NOCTTY);
   if (ups < 0)
   {
      perror("open");
      return 1;
   }

   /* Set serial port for 2400 baud, N81 */
   tio.c_cflag = B2400 | CS8 | CLOCAL | CREAD;
   tio.c_iflag = IGNPAR;    /* Ignore errors, raw input */
   tio.c_oflag = 0;         /* Raw output */
   tio.c_lflag = 0;         /* No local echo */
   cfsetospeed(&tio, B2400);
   cfsetispeed(&tio, B2400);
   tcflush(ups, TCIFLUSH);
   tcsetattr(ups, TCSANOW, &tio);
   tcflush(ups, TCIFLUSH);

   /* Disable echo and line buffering on stdin */
   con = fileno(stdin);
   tcgetattr(con, &tio);
   tio.c_lflag &= ~(ECHO|ICANON);
   tcsetattr(con, TCSANOW, &tio);

   while (1)
   {
      FD_ZERO(&fds);
      FD_SET(ups, &fds);
      FD_SET(con, &fds);

      do
      {
         rc = select(max(con,ups)+1, &fds, NULL, NULL, NULL);
      }
      while (rc == -1 && (errno == EAGAIN || errno == EINTR));

      if (rc == -1)
      {
         perror("select");
         return 1;
      }

      if (FD_ISSET(ups, &fds))
      {
         do
         {
            rc = read(ups, &cmd, 1);
         }
         while (rc != 1 && (errno == EAGAIN || errno == EINTR));

         if (rc != 1)
         {
            perror("read");
            return 1;
         }

         dbg("CMD: ");
         if (cmd == 0)
            dbg("<NUL>");
         else if (cmd >= 1 && cmd <= 26)
            dbg("^%c", cmd+64);
         else
            dbg("%c", cmd);
         dbg("\nRSP: ");

         handle_ups_cmd(cmd);
      }

      if (FD_ISSET(con, &fds))
      {
         do
         {
            rc = read(con, &cmd, 1);
         }
         while (rc != 1 && (errno == EAGAIN || errno == EINTR));

         if (rc != 1)
         {
            perror("read");
            return 1;
         }

         handle_key_cmd(cmd);
      }
   }

   return 0;
}
