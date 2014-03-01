/*
 * apctest.c
 *
 * A cable tester program for apcupsd.
 *
 * Hacked from apcupsd.c by Kern Sibbald, Sept 2000
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
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
#include <termios.h>

UPSINFO *core_ups;
UPSINFO *ups;

int le_bit = TIOCM_LE;
int st_bit = TIOCM_ST;
int sr_bit = TIOCM_SR;
int dtr_bit = TIOCM_DTR;
int rts_bit = TIOCM_RTS;
int cts_bit = TIOCM_CTS;
int cd_bit = TIOCM_CD;
int rng_bit = TIOCM_RNG;
int dsr_bit = TIOCM_DSR;

struct termios oldtio;
struct termios newtio;

/* Forward referenced functions */
static void do_dumb_testing(void);
static void test1(void);
static void test2(void);
static void test3(void);
static void test4(void);
static void test5(void);
static void test6(void);
static void guess(void);

static void do_smart_testing(void);

#ifdef HAVE_APCSMART_DRIVER
static void smart_test1(void);
static void smart_calibration(void);
static void monitor_calibration_progress(int monitor);
static void terminate_calibration(int ask);
static void program_smart_eeprom(void);
static void print_eeprom_values(UPSINFO *ups);
static void smart_ttymode(void);
static void parse_eeprom_cmds(char *eprom, char locale);
int apcsmart_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
static void print_valid_eeprom_values(UPSINFO *ups);
#endif

static void do_usb_testing(void);

#ifdef HAVE_USB_DRIVER

/* USB driver functions */
#include "drivers/usb/usb.h"

/* Our own test functions */
static void usb_kill_power_test(void);
static void usb_get_self_test_result(void);
static void usb_run_self_test(void);
static int usb_get_battery_date(void);
static void usb_set_battery_date(void);
static void usb_get_manf_date(void);
static void usb_set_alarm(void);
static void usb_set_sens(void);
static void usb_set_xferv(int lowhigh);
static void usb_calibration();
static void usb_test_alarm(void);
static int usb_get_self_test_interval(void);
static void usb_set_self_test_interval(void);
#endif

static void strip_trailing_junk(char *cmd);
static char *get_cmd(const char *prompt);
static int write_file(char *buf);


/* Static variables */
static int normal, no_cable, no_power, low_batt;
static int test1_done = 0;
static int test2_done = 0;
static int test3_done = 0;
static int test4_done = 0;
static int test5_done = 0;


/* Print a message, and also write it to an output file */
static void pmsg(const char *fmt, ...)
{
   char buf[3000];
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   avsnprintf(buf, sizeof(buf), (char *)fmt, arg_ptr);
   va_end(arg_ptr);
   printf("%s", buf);
   fflush(stdout);
   write_file(buf);
}

/* Write output into "log" file */
static int write_file(char *buf)
{
   static int out_fd = -1;

   if (out_fd == -1) {
      out_fd = open("apctest.output", O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (out_fd < 0) {
         printf("Could not create apctest.output: %s\n", strerror(errno));
         return -1;
      }
   }
   return write(out_fd, buf, strlen(buf));
}

/* Print out current time */
static void ptime(void)
{
   char dt[MAXSTRING];
   time_t now = time(NULL);

   strftime(dt, MAXSTRING, "%Y-%m-%d %T ", localtime(&now));
   pmsg(dt);
}


/*
 * The terminate function; trapping signals allows apctest
 * to exit and cleanly close logfiles, and reset the tty back
 * to its original settings. You may want to add this
 * to all configurations.  Of course, the file descriptors
 * must be global for this to work, I also set them to
 * NULL initially to allow terminate to determine whether
 * it should close them.
 */
void apctest_terminate(int sig)
{
   if (sig != 0) {
      ptime();
      pmsg("apctest exiting, signal %u\n", sig);
   }

   clear_files();

   device_close(ups);

   delete_lockfile(ups);

   clean_threads();

   closelog();
   destroy_ups(ups);
   _exit(0);
}

void apctest_error_cleanup(UPSINFO *ups)
{
   device_close(ups);
   delete_lockfile(ups);
   clean_threads();
   pmsg("apctest error termination completed\n");
   closelog();
   destroy_ups(ups);
   exit(1);
}

/*
 * Subroutine error_out prints FATAL ERROR with file,
 * line number, and the error message then cleans up 
 * and exits. It is normally called from the Error_abort
 * define, which inserts the file and line number.
 */
void apctest_error_out(const char *file, int line, const char *fmt, ...)
{
   char buf[256];
   va_list arg_ptr;
   int i;

   asnprintf(buf, sizeof(buf),
      "apctest FATAL ERROR in %s at line %d\n", file, line);
   i = strlen(buf);

   va_start(arg_ptr, fmt);
   avsnprintf((char *)&buf[i], sizeof(buf) - i, (char *)fmt, arg_ptr);
   va_end(arg_ptr);

   pmsg(buf);

   apctest_error_cleanup(core_ups);     /* finish the work */
}

/*
 * Subroutine error_exit simply prints the supplied error
 * message, cleans up, and exits.
 */
void apctest_error_exit(const char *fmt, ...)
{
   char buf[256];
   va_list arg_ptr;

   va_start(arg_ptr, fmt);
   avsnprintf(buf, sizeof(buf), (char *)fmt, arg_ptr);
   va_end(arg_ptr);

   pmsg(buf);

   apctest_error_cleanup(core_ups);     /* finish the work */
}


/* Main program */

/* This application must be linked as console app. */

int main(int argc, char *argv[])
{
   /* Set specific error_* handlers. */
   error_out = apctest_error_out;
   error_exit = apctest_error_exit;

   /*
    * Default config file. If we set a config file in startup switches, it
    * will be re-filled by parse_options()
    */
   cfgfile = APCCONF;

   ups = new_ups();                /* get new ups */
   if (!ups)
      Error_abort1("%s: init_ipc failed.\n", argv[0]);

   init_ups_struct(ups);
   core_ups = ups;                 /* this is our core ups structure */

   /* parse_options is self messaging on errors, so we need only to exit() */
   if (parse_options(argc, argv))
      exit(1);

   pmsg("\n\n");
   ptime();
   pmsg("apctest " APCUPSD_RELEASE " (" ADATE ") " APCUPSD_HOST "\n");

   pmsg("Checking configuration ...\n");
   check_for_config(ups, cfgfile);

   attach_driver(ups);
   if (ups->driver == NULL)
      Error_abort0("apctest cannot continue without a valid driver.\n");

   pmsg("Attached to driver: %s\n", ups->driver->driver_name);

   ups->start_time = time(NULL);

   /* Print configuration */
   pmsg("sharenet.type = %s\n", ups->sharenet.long_name);
   pmsg("cable.type = %s\n", ups->cable.long_name);
   pmsg("mode.type = %s\n", ups->mode.long_name);

   delete_lockfile(ups);

   pmsg("Setting up the port ...\n");
   setup_device(ups);

   if (hibernate_ups) {
      pmsg("apctest: bad option, I cannot do a killpower\n");
      apctest_terminate(0);
   }

   init_signals(apctest_terminate);

   pmsg("Doing prep_device() ...\n");
   prep_device(ups);

   /*
    * This isn't a documented option but can be used
    * for testing dumb mode on a SmartUPS if you have
    * the proper cable.
    */
   if (dumb_mode_test) {
#ifdef HAVE_APCSMART_DRIVER
      char ans[20];

      write(ups->fd, "R", 1);   /* enter dumb mode */
      *ans = 0;
      getline(ans, sizeof(ans), ups);
      pmsg("Going dumb: %s\n", ans);
#else
      pmsg("apcsmart not compiled: dumb mode test unavailable\n");
#endif
   }

   switch (ups->mode.type)
   {
   case USB_UPS:
      pmsg("\nYou are using a USB cable type, so I'm entering USB test mode\n");
      do_usb_testing();
      break;
   case APCSMART_UPS:
      pmsg("\nYou are using a SMART cable type, so I'm entering SMART test mode\n");
      do_smart_testing();
      break;
   case DUMB_UPS:
      pmsg("\nYou are using a DUMB cable type, so I'm entering DUMB test mode\n");
      do_dumb_testing();
      break;
   default:
      pmsg("Testing not yet implemented for this UPSTYPE.\n");
   }
   apctest_terminate(0);
   return -1;                      /* to keep compiler happy */
}

static void print_bits(int bits)
{
   char buf[200];

   asnprintf(buf, sizeof(buf), "IOCTL GET: %x ", bits);

   if (bits & le_bit)
      astrncat(buf, "LE ", sizeof(buf));
   if (bits & st_bit)
      astrncat(buf, "ST ", sizeof(buf));
   if (bits & sr_bit)
      astrncat(buf, "SR ", sizeof(buf));
   if (bits & dtr_bit)
      astrncat(buf, "DTR ", sizeof(buf));
   if (bits & rts_bit)
      astrncat(buf, "RTS ", sizeof(buf));
   if (bits & cts_bit)
      astrncat(buf, "CTS ", sizeof(buf));
   if (bits & cd_bit)
      astrncat(buf, "CD ", sizeof(buf));
   if (bits & rng_bit)
      astrncat(buf, "RNG ", sizeof(buf));
   if (bits & dsr_bit)
      astrncat(buf, "DSR ", sizeof(buf));

   astrncat(buf, "\n", sizeof(buf));

   pmsg(buf);
}

static void do_dumb_testing(void)
{
   int quit = FALSE;
   char *cmd;

   pmsg("Hello, this is the apcupsd Cable Test program.\n\n"
        "We are beginning testing for dumb UPSes, which\n"
        "use signaling rather than commands.\n"
        "Most tests enter a loop polling every second for 10 seconds.\n");

   while (!quit) {
      pmsg("\n"
           "1) Test 1 - normal mode\n"
           "2) Test 2 - no cable\n"
           "3) Test 3 - no power\n"
           "4) Test 4 - low battery (requires test 3 first)\n"
           "5) Test 5 - battery exhausted\n"
           "6) Test 6 - kill UPS power\n"
           "7) Test 7 - run tests 1 through 5\n"
           "8) Guess which is the appropriate cable\n"
           "Q) Quit\n\n");

      cmd = get_cmd("Select test number: ");

      if (cmd) {
         int item = atoi(cmd);

         switch (item) {
         case 1:
            test1();
            break;
         case 2:
            test2();
            break;
         case 3:
            test3();
            break;
         case 4:
            test4();
            break;
         case 5:
            test5();
            break;
         case 6:
            test6();
            break;
         case 7:
            test1();
            test2();
            test3();
            test4();
            test5();
            break;
         case 8:
            guess();
            break;
         default:
            if (tolower(*cmd) == 'q')
               quit = TRUE;
            else
               pmsg("Illegal response. Please enter 1-8,Q\n");
            break;
         }
      } else {
         pmsg("Illegal response. Please enter 1-8,Q\n");
      }
   }

   ptime();
   pmsg("End apctest.\n");
}

/*
 * Poll 10 times once per second and report any 
 * change in serial port bits.
 */
static int test_bits(int inbits)
{
   int i, nbits;
   int bits = inbits;

   for (i = 0; i < 10; i++) {
      if (ioctl(ups->fd, TIOCMGET, &nbits) < 0) {
         pmsg("ioctl error, big problem: %s\n", strerror(errno));
         return 0;
      }

      if (i == 0 || nbits != bits) {
         ptime();
         print_bits(nbits);
         bits = nbits;
      }

      sleep(1);
   }

   return bits;
}

static void test1(void)
{
   pmsg("\nFor the first test, everything should be normal.\n"
        "The UPS should be plugged in to the power, and the serial cable\n"
        "should be connected to the computer.\n\n"
        "Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   normal = test_bits(0);

   ptime();
   pmsg("Test 1: normal condition, completed.\n");
   test1_done = TRUE;
}

static void test2(void)
{
   pmsg("\nFor the second test, the UPS should be plugged in to the power, \n"
        "but the serial port should be DISCONNECTED from the computer.\n\n"
        "Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   no_cable = test_bits(0);

   ptime();
   pmsg("Test 2: no cable, completed. \n");
   test2_done = TRUE;
}

static void test3(void)
{
   pmsg("\nFor the third test, the serial cable should be plugged\n"
        "back into the UPS, but the AC power plug to the UPS should be DISCONNECTED.\n\n"
        "Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   no_power = test_bits(0);

   ptime();
   pmsg("Test 3: no power, completed.\n");
   test3_done = TRUE;
}

static void test4(void)
{
   int i, bits;

   if (!test3_done) {
      pmsg("We need the output of test 3 to run this test.\n"
           "Please run test 3 first then this test.\n");
      return;
   }

   pmsg("\nThe fourth test is the same as the third test:\n"
        "the serial cable should be plugged in to the UPS, but the AC power\n"
        "plug to the UPS should be DISCONNECTED. In addition, you should\n"
        "continue this test until the batteries are exhausted.\n"
        "If apctest detects a state change, it will stop\n"
        "the test. If not, hit control-C to stop the program\n\n"
        "PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n"
        "Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   low_batt = no_power;

   ptime();
   pmsg("Start test 4: ");
   pmsg("\n");

   /* Spin until we get a state change */
   for (i = 0;; i++) {
      if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
         pmsg("ioctl error, big problem: %s\n", strerror(errno));
         return;
      }

      if (bits != low_batt) {
         ptime();
         print_bits(bits);
         low_batt = bits;
         break;
      } else if (i == 0) {
         ptime();
         print_bits(bits);
      }

      sleep(1);
   }

   ptime();
   pmsg("Test 4: low battery, completed.\n");
   test4_done = TRUE;
}

static void test5(void)
{
   int i, bits, last_bits = 0;

   pmsg("\nThe fifth test is the same as the third test:\n"
        "the serial cable should be plugged in to the UPS, but the AC power\n"
        "plug to the UPS should be DISCONNECTED. In addition, you should\n"
        "continue this test until the batteries are exhausted.\n"
        "If apctest detects a state change, contrary to test 4, it\n"
        "will not stop. The idea is to see ANY changed bits just up to\n"
        "the very moment that the UPS powers down.\n\n"
        "PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n"
        "Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   pmsg("Start test 5:\n");

   /* Spin until we get a state change */
   for (i = 0;; i++) {
      if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
         pmsg("ioctl error, big problem: %s\n", strerror(errno));
         return;
      }

      if (i == 60)
         i = 0;                    /* force print once a minute */

      if (i == 0 || bits != last_bits) {
         ptime();
         print_bits(bits);
         last_bits = bits;
      }

      sleep(1);
   }

   /* Should never get here */
   /* NOTREACHED */
   ptime();
   pmsg("Test 5: battery exhausted, completed.\n");
   test5_done = TRUE;
}

static void test6(void)
{
   int bits;

   pmsg("\nThis test will attempt to power down the UPS.\n"
        "The serial cable should be plugged in to the UPS, but the\n"
        "AC power plug to the UPS should be DISCONNECTED.\n\n"
        "PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n"
        "Please enter any character when ready to continue: ");
   fgetc(stdin);
   pmsg("\n");

   if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
      pmsg("ioctl error, big problem: %s\n", strerror(errno));
      return;
   }

   ptime();
   print_bits(bits);
   make_file(ups, ups->pwrfailpath);
   initiate_hibernate(ups);
   unlink(ups->pwrfailpath);
   ptime();

   pmsg("returned from kill_power function.\n");

   if (ioctl(ups->fd, TIOCMGET, &bits) < 0) {
      pmsg("ioctl error, big problem: %s\n", strerror(errno));
      return;
   }

   ptime();
   print_bits(bits);
}

/*
 * Make a wild guess at the cable type 
 *
 * If I had more data on each of the cable types, this could
 * be much improved.
 */
static void guess(void)
{
   int found = 0;

   if (!(test1_done && test3_done)) {
      pmsg("Test 1 and test 3 must be performed before I can make a guess.\n");
      return;
   }
   if (!(normal & (cd_bit | cts_bit)) && (no_power & cd_bit) && (low_batt & cts_bit)) {
      pmsg("This looks like a CUSTOM_SIMPLE cable\n");
      found = 1;
   }

   if (!(normal & (cd_bit | cts_bit)) && (no_power & cts_bit) && (low_batt & cd_bit)) {
      pmsg("This looks like a 940-0020A\n");
      found = 1;
   }

   if (!(normal & (cd_bit | sr_bit)) && (no_power & cd_bit) && (low_batt & sr_bit)) {
      pmsg("This looks like a 940-0023A cable\n");
      found = 1;
   }

   if (!(normal & (rng_bit | cd_bit)) && (no_power & rng_bit) && (low_batt & cd_bit)) {
      pmsg("This looks like a 940-0095A cable\n");
      found = 1;
   }

   if (!found) {
      pmsg("Hmmm. I don't quite know what you have. Sorry.\n");
   }
}

static void do_smart_testing(void)
{
#ifdef HAVE_APCSMART_DRIVER
   char *cmd;
   int quit = FALSE;

   pmsg("Hello, this is the apcupsd Cable Test program.\n"
        "This part of apctest is for testing Smart UPSes.\n"
        "Please select the function you want to perform.\n");

   while (!quit) {
      pmsg("\n"
           "1) Query the UPS for all known values\n"
           "2) Perform a Battery Runtime Calibration\n"
           "3) Abort Battery Calibration\n"
           "4) Monitor Battery Calibration progress\n"
           "5) Program EEPROM\n"
           "6) Enter TTY mode communicating with UPS\n"
           "Q) Quit\n\n");

      cmd = get_cmd("Select function number: ");
      if (cmd) {
         int item = atoi(cmd);

         switch (item) {
         case 1:
            smart_test1();
            break;
         case 2:
            smart_calibration();
            break;
         case 3:
            terminate_calibration(1);
            break;
         case 4:
            monitor_calibration_progress(0);
            break;
         case 5:
            program_smart_eeprom();
            break;
         case 6:
            smart_ttymode();
            break;
         default:
            if (tolower(*cmd) == 'q')
               quit = TRUE;
            else
               pmsg("Illegal response. Please enter 1-6,Q\n");
            break;
            break;
         }
      } else {
         pmsg("Illegal response. Please enter 1-6,Q\n");
      }
   }
   ptime();
   pmsg("End apctest.\n");
#else
   pmsg("APC Smart Driver not configured.\n");
#endif
}

#ifdef HAVE_APCSMART_DRIVER
static void smart_ttymode(void)
{
#ifdef HAVE_MINGW
   // This is crap. Windows has no sane way (that I can find) to watch two
   // fds for activity from a single thread without involving the overly
   // complex "overlapped" io junk. So we will resort to polling.

   // Save any existing timeouts on the UPS fd
   HANDLE hnd = (HANDLE)_get_osfhandle(ups->fd);
   COMMTIMEOUTS orig_ups_ct;
   GetCommTimeouts(hnd, &orig_ups_ct);

   // Reset UPS fd timeout to 50 msec
   COMMTIMEOUTS ct;
   ct.ReadIntervalTimeout = MAXDWORD;
   ct.ReadTotalTimeoutMultiplier = 0;
   ct.ReadTotalTimeoutConstant = 50;
   ct.WriteTotalTimeoutMultiplier = 0;
   ct.WriteTotalTimeoutConstant = 0;
   SetCommTimeouts(hnd, &ct);

   pmsg("Enter an ESC character (or ctl-[) to exit.\n\n");

   char ch;
   while (1)
   {
      // Waits up to 50 msec for a char from the UPS
      if (read(ups->fd, &ch, 1) == 1)
         putch(ch);

      // Check if keyboard key was hit and read it (only Windows would
      // have a function dedicated to checking if a key has been pressed!)
      if (kbhit())
      {
         ch = getch();
         if (ch == 0x1b)
            break;
         else
            write(ups->fd, &ch, 1);
      }
   }

   // Restore original timeouts on UPS fd
   SetCommTimeouts(hnd, &orig_ups_ct);
#else
   char ch;
   struct termios t, old_term_params;
   fd_set rfds;
   int stat;

   if (tcgetattr(0, &old_term_params) != 0) {
      pmsg("Cannot tcgetattr()\n");
      return;
   }

   t = old_term_params;
   t.c_cc[VMIN] = 1;               /* satisfy read after 1 char */
   t.c_cc[VTIME] = 0;
   t.c_iflag &= ~(BRKINT | IGNPAR | PARMRK | INPCK |
      ISTRIP | ICRNL | IXON | IXOFF | INLCR | IGNCR);
   t.c_iflag |= IGNBRK;
   t.c_lflag &= ~(ICANON | ISIG | NOFLSH | TOSTOP);
   tcflush(0, TCIFLUSH);

   if (tcsetattr(0, TCSANOW, &t) == -1) {
      pmsg("Cannot tcsetattr()\n");
      return;
   }

   pmsg("Enter an ESC character (or ctl-[) to exit.\n\n");

   tcflush(ups->fd, TCIOFLUSH);
   for (;;) {
      FD_ZERO(&rfds);
      FD_SET(0, &rfds);
      FD_SET(ups->fd, &rfds);
      stat = select((ups->fd) + 1, &rfds, NULL, NULL, NULL);
      if (stat == -1) {
         pmsg("select() failed.\n");
         break;
      }
      if (FD_ISSET(0, &rfds)) {
         if (read(0, &ch, 1) != 1)
            break;

         if (ch == 0x1B)
            break;

         write(ups->fd, &ch, 1);
      }
      if (FD_ISSET(ups->fd, &rfds)) {
         if (read(ups->fd, &ch, 1) != 1)
            break;

         write(1, &ch, 1);
      }
   }

   tcsetattr(0, TCSANOW, &old_term_params);
#endif
}


/* Do runtime battery calibration */
static void smart_calibration(void)
{
   char *ans, cmd;
   char answer[2000];
   int stat, monitor, elapse;
   time_t start_time;

   pmsg("First ensure that we have a good link and \n"
        "that the UPS is functionning normally.\n");
   pmsg("Simulating UPSlinkCheck ...\n");

   tcflush(ups->fd, TCIOFLUSH);

   /* Start really simply */
   cmd = 'Y';
   stat = write(ups->fd, &cmd, 1);
   if (stat < 0)
      pmsg("Bad status from write: %s\n", strerror(errno));

   stat = getline(answer, sizeof(answer), ups);
   pmsg("Wrote: Y Got: %s\n", answer);
   if (stat == FAILURE) {
      pmsg("getline failed. Apparently the link is not up.\n"
           "Giving up.\n");
      return;
   }

   pmsg("Attempting to use smart_poll() ...\n");
   ans = smart_poll('Y', ups);
   pmsg("Sent: Y Got: %s  ", ans);
   if (strcmp(ans, "SM") == 0) {
      pmsg("Good -- smart_poll() works!.\n\n");
   } else {
      pmsg("Not good.\nGiving up.\n");
      return;
   }

   pmsg("Checking estimated runtime ...\n");
   ans = smart_poll('j', ups);
   if (*ans >= '0' && *ans <= '9') {
      int rt = atoi(ans);

      pmsg("Current runtime is %d minutes\n", rt);
   } else {
      pmsg("Unexpected response from UPS: %s\n", ans);
      return;
   }

   pmsg("Checking for battery level ...\n");
   ans = smart_poll('f', ups);
   if (strncmp(ans, "100.0", 5) == 0) {
      pmsg("Battery level is 100.0 -- OK\n");
   } else {
      pmsg("Battery level %s insufficient to run test.\n", ans);
      return;
   }

   pmsg("\nThe battery calibration should automatically end\n"
        "when the battery level drops below about 25, depending\n"
        "on your UPS.\n\n"
        "I can also optionally monitor the progress\n"
        "and stop the calibration if it goes below 10. However,\n"
        "in the case of a new battery this may prematurely end the\n"
        "calibration loosing the effect.\n\n");

   ans = get_cmd("Do you want me to stop the calibration\n"
                 "if the battery level goes too low? (y/n): ");

   if (*ans == 'Y' || *ans == 'y')
      monitor = 1;
   else
      monitor = 0;

   pmsg("\nSending Battery Calibration command. ...\n");

   ans = smart_poll('D', ups);
   if (*ans == '!' || strcmp(ans, "OK") == 0) {
      pmsg("UPS has initiated battery calibration.\n");
   } else {
      pmsg("Unexpected response from UPS: %s\n", ans);
      return;
   }

   start_time = time(NULL);
   monitor_calibration_progress(monitor);
   elapse = time(NULL) - start_time;
   pmsg("On battery %d sec or %dm%ds.\n", elapse, elapse / 60, elapse % 60);
}

static void terminate_calibration(int ask)
{
   char *ans, *cmd;

   if (ask) {
      pmsg("\nCAUTION! Don't use this command unless the UPS\n"
           "is already doing a calibration.\n");
      cmd = get_cmd("Are you sure? (yes/no): ");
      if (strcmp(cmd, "yes") != 0)
         return;
   }

   pmsg("\nAttempting to abort calibration ...\n");
   ans = smart_poll('D', ups);     /* abort calibration */
   if (*ans == '$' || strcmp(ans, "NO") == 0) {
      pmsg("Battery Runtime Calibration terminated.\n");
      pmsg("Checking estimated runtime ...\n");
      ans = smart_poll('j', ups);
      if (*ans >= '0' && *ans <= '9') {
         int rt = atoi(ans);

         pmsg("Updated runtime is %d\n", rt);
      } else {
         pmsg("Unexpected response from UPS: %s\n", ans);
      }
   } else {
      pmsg("Response to abort request: %s\n", ans);
   }
}


static void monitor_calibration_progress(int monitor)
{
   char *ans;
   int count = 6;
   int max_count = 6;
   char period = '.';

   pmsg("Monitoring the calibration progress ...\n"
        "To stop the calibration, enter a return.\n");

   for (;;) {
      fd_set rfds;
      struct timeval tv;
      int retval, percent;
      char cmd;

      FD_ZERO(&rfds);
      FD_SET(ups->fd, &rfds);
      FD_SET(STDIN_FILENO, &rfds);
      tv.tv_sec = 10;
      tv.tv_usec = 0;
      errno = 0;

      retval = select((ups->fd) + 1, &rfds, NULL, NULL, &tv);

      switch (retval) {
      case 0:
         if (++count >= max_count) {
            ans = smart_poll('f', ups); /* Get battery charge */
            percent = (int)strtod(ans, NULL);
            pmsg("\nBattery charge %d\n", percent);

            if (percent > 0) {
               if (monitor && percent <= 10) {
                  pmsg("Battery charge less than 10% terminating calibration ...\n");
                  terminate_calibration(0);
                  return;
               }
               if (percent < 30)
                  max_count = 2;   /* report faster */
            }

            ans = smart_poll('j', ups); /* Get runtime */
            if (*ans >= '0' && *ans <= '9') {
               int rt = atoi(ans);

               pmsg("Remaining runtime is %d minutes\n", rt);
               if (monitor && rt < 2) {
                  pmsg("Runtime less than 2 minutes terminating calibration ...\n");
                  terminate_calibration(0);
                  return;
               }
            }
            count = 0;
         } else {
            write(STDOUT_FILENO, &period, 1);
         }
         continue;

      case -1:
         if (errno == EINTR || errno == EAGAIN)
            continue;

         pmsg("\nSelect error. ERR=%s\n", strerror(errno));
         return;

      default:
         break;
      }

      /* *ANY* user input aborts the calibration */
      if (FD_ISSET(STDIN_FILENO, &rfds)) {
         read(STDIN_FILENO, &cmd, 1);
         pmsg("\nUser input. Terminating calibration ...\n");
         terminate_calibration(0);
         return;
      }

      if (FD_ISSET(ups->fd, &rfds)) {
         read(ups->fd, &cmd, 1);
         if (cmd == '$') {
            pmsg("\nBattery Runtime Calibration terminated by UPS.\n");
            pmsg("Checking estimated runtime ...\n");
            ans = smart_poll('j', ups);
            if (*ans >= '0' && *ans <= '9') {
               int rt = atoi(ans);

               pmsg("Remaining runtime is %d minutes\n", rt);
            } else {
               pmsg("Unexpected response from UPS: %s\n", ans);
            }
            return;
            /* ignore normal characters */
         } else if (cmd == '!' || cmd == '+' || cmd == ' ' ||
            cmd == '\n' || cmd == '\r') {
            continue;
         } else {
            pmsg("\nUPS sent: %c\n", cmd);
         }
      }
   }
}

static void program_smart_eeprom(void)
{
   char *cmd;
   int quit = FALSE;

   pmsg("This is the EEPROM programming section of apctest.\n"
        "Please select the function you want to perform.\n");

   while (!quit) {
      pmsg("\n"
           " 1) Print EEPROM values\n"
           " 2) Change Battery date\n"
           " 3) Change UPS name\n"
           " 4) Change sensitivity\n"
           " 5) Change alarm delay\n"
           " 6) Change low battery warning delay\n"
           " 7) Change wakeup delay\n"
           " 8) Change shutdown delay\n"
           " 9) Change low transfer voltage\n"
           "10) Change high transfer voltage\n"
           "11) Change battery return threshold percent\n"
           "12) Change output voltage when on batteries\n"
           "13) Change the self test interval\n"
           "14) Set EEPROM with conf file values\n"
           " Q) Quit\n\n");

      cmd = get_cmd("Select function number: ");
      if (cmd) {
         int item = atoi(cmd);

         switch (item) {
         case 1:
            print_eeprom_values(ups);
            break;

         case 2:
            cmd = get_cmd("Enter new battery date -- DD/MM/YY: ");
            if (strlen(cmd) != 8 || cmd[2] != '/' || cmd[5] != '/') {
               pmsg("Date must be exactly DD/MM/YY\n");
               break;
            }
            apcsmart_ups_program_eeprom(ups, CI_BATTDAT, cmd);
            break;

         case 3:
            cmd = get_cmd("Enter new UPS name -- max 8 chars: ");
            if (strlen(cmd) == 0 || strlen(cmd) > 8) {
               pmsg("Name must be between 1 and 8 characters long.\n");
               break;
            }
            apcsmart_ups_program_eeprom(ups, CI_IDEN, cmd);
            break;

         case 4:
            cmd = get_cmd("Enter new sensitivity: ");
            apcsmart_ups_program_eeprom(ups, CI_SENS, cmd);
            break;

         case 5:
            cmd = get_cmd("Enter new alarm delay: ");
            apcsmart_ups_program_eeprom(ups, CI_DALARM, cmd);
            break;

         case 6:
            cmd = get_cmd("Enter new low battery delay: ");
            apcsmart_ups_program_eeprom(ups, CI_DLBATT, cmd);
            break;

         case 7:
            cmd = get_cmd("Enter new wakeup delay: ");
            apcsmart_ups_program_eeprom(ups, CI_DWAKE, cmd);
            break;

         case 8:
            cmd = get_cmd("Enter new shutdown delay: ");
            apcsmart_ups_program_eeprom(ups, CI_DSHUTD, cmd);
            break;

         case 9:
            cmd = get_cmd("Enter new low transfer voltage: ");
            apcsmart_ups_program_eeprom(ups, CI_LTRANS, cmd);
            break;

         case 10:
            cmd = get_cmd("Enter new high transfer voltage: ");
            apcsmart_ups_program_eeprom(ups, CI_HTRANS, cmd);
            break;

         case 11:
            cmd = get_cmd("Enter new battery return level: ");
            apcsmart_ups_program_eeprom(ups, CI_RETPCT, cmd);
            break;

         case 12:
            cmd = get_cmd("Enter new output voltage on batteries: ");
            apcsmart_ups_program_eeprom(ups, CI_NOMOUTV, cmd);
            break;

         case 13:
            cmd = get_cmd("Enter new self test interval: ");
            apcsmart_ups_program_eeprom(ups, CI_STESTI, cmd);
            break;

         case 14:
            pmsg("The EEPROM values to be changed will be taken from\n"
                 "the configuration directives in your apcupsd.conf file.\n");
            cmd = get_cmd("Do you want to continue? (Y/N): ");
            if (*cmd != 'y' && *cmd != 'Y') {
               pmsg("EEPROM changes NOT made.\n");
               break;
            }
            apcsmart_ups_program_eeprom(ups, -1, NULL);
            break;

         default:
            if (tolower(*cmd) == 'q')
               quit = TRUE;
            else
               pmsg("Illegal response. Please enter 1-14,Q\n");
            break;
         }
      } else {
         pmsg("Illegal response. Please enter 1-14,Q\n");
      }
   }
   ptime();
   pmsg("End EEPROM programming.\n");
}

static void smart_test1(void)
{
   char *ans, *p, *o, cmd;
   char answer[2000];
   char parts[2000];
   int stat;

#ifdef working
   char locale, locale1, locale2;
#endif

   pmsg("I am going to run through the series of queries of the UPS\n"
        "that are used in initializing apcupsd.\n\n");

   pmsg("Simulating UPSlinkCheck ...\n");
   tcflush(ups->fd, TCIOFLUSH);

   /* Start really simply */
   cmd = 'Y';
   stat = write(ups->fd, &cmd, 1);
   if (stat < 0)
      pmsg("Bad status from write: %s\n", strerror(errno));

   stat = getline(answer, sizeof(answer), ups);
   pmsg("Wrote: Y Got: %s\n", answer);
   if (stat == FAILURE) {
      pmsg("getline failed. Apparently the link is not up.\n"
           "Giving up.\n");
      return;
   }

   pmsg("Attempting to use smart_poll() ...\n");
   ans = smart_poll('Y', ups);
   pmsg("Sent: Y Got: %s  ", ans);
   if (strcmp(ans, "SM") == 0) {
      pmsg("Good -- smart_poll() works!.\n\n");
   } else {
      pmsg("Not good.\nGiving up.\n");
      return;
   }

   pmsg("Going to ask for valid commands...\n");
   cmd = 'a';
   stat = write(ups->fd, &cmd, 1);
   if (stat != 1)
      pmsg("Bad response from write: %d %s\n", stat, strerror(errno));

   *answer = 0;
   stat = getline(answer, sizeof(answer), ups);
   pmsg("Wrote: a Got: %s\n", answer);
   if (stat == FAILURE) {
      pmsg("Cannot get the list of valid commands (a very old ups perhaps ?).\n");
   }

   /* Get protocol version */
   for (p = answer, o = parts; *p && *p != '.'; p++)
      *o++ = *p;

   *o = 0;
   pmsg("Protocol version is: %s\n", parts);
   if (*p == '.')
      p++;

   /* Get alert characters */
   for (o = parts; *p && *p != '.'; p++) {
      if (*p < 0x20) {
         *o++ = '^';
         *o++ = *p + 'A' - 1;
      } else {
         *o++ = *p;
      }
   }
   *o = 0;

   pmsg("Alert characters are: %s\n", parts);
   if (*p == '.')
      p++;

   /* Get command characters */
   for (o = parts; *p; p++) {
      if (*p < 0x20) {
         *o++ = '^';
         *o++ = *p + 'A' - 1;
      } else {
         *o++ = *p;
      }
   }
   *o = 0;

   pmsg("Command characters are: %s\n", parts);
   pmsg("\nNow running through apcupsd get_UPS capabilities().\n"
        "NA  indicates that the feature is Not Available\n\n");

   pmsg("UPS Status: %s\n", smart_poll(ups->UPS_Cmd[CI_STATUS], ups));

   pmsg("Line quality: %s\n", smart_poll(ups->UPS_Cmd[CI_LQUAL], ups));

   pmsg("Reason for last transfer to batteries: %s\n",
      smart_poll(ups->UPS_Cmd[CI_WHY_BATT], ups));

   pmsg("Self-Test Status: %s\n", smart_poll(ups->UPS_Cmd[CI_ST_STAT], ups));

   pmsg("Line Voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_VLINE], ups));

   pmsg("Line Voltage Max: %s\n", smart_poll(ups->UPS_Cmd[CI_VMAX], ups));

   pmsg("Line Voltage Min: %s\n", smart_poll(ups->UPS_Cmd[CI_VMIN], ups));

   pmsg("Output Voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_VOUT], ups));

   pmsg("Batt level percent: %s\n", smart_poll(ups->UPS_Cmd[CI_BATTLEV], ups));

   pmsg("Batt voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_VBATT], ups));

   pmsg("UPS Load: %s\n", smart_poll(ups->UPS_Cmd[CI_LOAD], ups));

   pmsg("Line freq: %s\n", smart_poll(ups->UPS_Cmd[CI_FREQ], ups));

   pmsg("Runtime left: %s\n", smart_poll(ups->UPS_Cmd[CI_RUNTIM], ups));

   pmsg("UPS Internal temp: %s\n", smart_poll(ups->UPS_Cmd[CI_ITEMP], ups));

   pmsg("Dip switch settings: %s\n", smart_poll(ups->UPS_Cmd[CI_DIPSW], ups));

   pmsg("Register 1: %s\n", smart_poll(ups->UPS_Cmd[CI_REG1], ups));

   pmsg("Register 2: %s\n", smart_poll(ups->UPS_Cmd[CI_REG2], ups));

   pmsg("Register 3: %s\n", smart_poll('8', ups));

   pmsg("Sensitivity: %s\n", smart_poll(ups->UPS_Cmd[CI_SENS], ups));

   pmsg("Wakeup delay: %s\n", smart_poll(ups->UPS_Cmd[CI_DWAKE], ups));

   pmsg("Sleep delay: %s\n", smart_poll(ups->UPS_Cmd[CI_DSHUTD], ups));

   pmsg("Low transfer voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_LTRANS], ups));

   pmsg("High transfer voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_HTRANS], ups));

   pmsg("Batt charge for return: %s\n", smart_poll(ups->UPS_Cmd[CI_RETPCT], ups));

   pmsg("Alarm status: %s\n", smart_poll(ups->UPS_Cmd[CI_DALARM], ups));

   pmsg("Low battery shutdown level: %s\n", smart_poll(ups->UPS_Cmd[CI_DLBATT],
         ups));

   pmsg("UPS Name: %s\n", smart_poll(ups->UPS_Cmd[CI_IDEN], ups));

   pmsg("UPS Self test interval: %s\n", smart_poll(ups->UPS_Cmd[CI_STESTI], ups));

   pmsg("UPS manufacture date: %s\n", smart_poll(ups->UPS_Cmd[CI_MANDAT], ups));

   pmsg("UPS serial number: %s\n", smart_poll(ups->UPS_Cmd[CI_SERNO], ups));

   pmsg("Date battery replaced: %s\n", smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups));

   pmsg("Output voltage when on batteries: %s\n",
      smart_poll(ups->UPS_Cmd[CI_NOMOUTV], ups));

   pmsg("Nominal battery voltage: %s\n", smart_poll(ups->UPS_Cmd[CI_NOMBATTV], ups));

   pmsg("Percent humidity: %s\n", smart_poll(ups->UPS_Cmd[CI_HUMID], ups));

   pmsg("Ambient temperature: %s\n", smart_poll(ups->UPS_Cmd[CI_ATEMP], ups));

   pmsg("Firmware revision: %s\n", smart_poll(ups->UPS_Cmd[CI_REVNO], ups));

   pmsg("Number of external batteries installed: %s\n",
      smart_poll(ups->UPS_Cmd[CI_EXTBATTS], ups));

   pmsg("Number of bad batteries installed: %s\n",
      smart_poll(ups->UPS_Cmd[CI_BADBATTS], ups));

   pmsg("UPS model as defined by UPS: %s\n", smart_poll(ups->UPS_Cmd[CI_UPSMODEL],
         ups));

   pmsg("UPS EPROM capabilities string: %s\n", (ans =
         smart_poll(ups->UPS_Cmd[CI_EPROM], ups)));
   pmsg("The EPROM string is %d characters long!\n", strlen(ans));

   pmsg("Hours since last self test: %s\n", smart_poll(ups->UPS_Cmd[CI_ST_TIME],
         ups));

   pmsg("\nThat is all for now.\n");
   return;
}
#endif

static void do_usb_testing(void)
{
#ifdef HAVE_USB_DRIVER
   char *cmd;
   int quit = FALSE;

   pmsg("Hello, this is the apcupsd Cable Test program.\n"
        "This part of apctest is for testing USB UPSes.\n");

   pmsg("\nGetting UPS capabilities...");
   if (!usb_ups_get_capabilities(ups))
      pmsg("FAILED\nSome or all tests may not work!\n");
   else
      pmsg("SUCCESS\n");

   pmsg("\nPlease select the function you want to perform.\n");

   while (!quit) {
      pmsg("\n"
           "1)  Test kill UPS power\n"
           "2)  Perform self-test\n"
           "3)  Read last self-test result\n"
           "4)  View/Change battery date\n"
           "5)  View manufacturing date\n"
           "6)  View/Change alarm behavior\n"
           "7)  View/Change sensitivity\n"
           "8)  View/Change low transfer voltage\n"
           "9)  View/Change high transfer voltage\n"
           "10) Perform battery calibration\n"
           "11) Test alarm\n"
           "12) View/Change self-test interval\n"
           " Q) Quit\n\n");

      cmd = get_cmd("Select function number: ");
      if (cmd) {
         int item = atoi(cmd);

         switch (item) {
         case 1:
            usb_kill_power_test();
            break;
         case 2:
            usb_run_self_test();
            break;
         case 3:
            usb_get_self_test_result();
            break;
         case 4:
            usb_set_battery_date();
            break;
         case 5:
            usb_get_manf_date();
            break;
         case 6:
            usb_set_alarm();
            break;
         case 7:
            usb_set_sens();
            break;
         case 8:
            usb_set_xferv(0);
            break;
         case 9:
            usb_set_xferv(1);
            break;
         case 10:
            usb_calibration();
            break;
         case 11:
            usb_test_alarm();
            break;
         case 12:
            usb_set_self_test_interval();
            break;
         default:
            if (tolower(*cmd) == 'q')
               quit = TRUE;
            else
               pmsg("Illegal response. Please enter 1-12,Q\n");
            break;
         }
      } else {
         pmsg("Illegal response. Please enter 1-12,Q\n");
      }
   }
   ptime();
   pmsg("End apctest.\n");
#else
   pmsg("USB Driver not configured.\n");
#endif
}

#ifdef HAVE_USB_DRIVER

static void usb_set_xferv(int lowhigh)
{
   int result;
   char* cmd;
   const char *text;
   int ci;

   if (lowhigh) {
      text = "HIGH";
      ci = CI_HTRANS;
   } else {
      text = "LOW";
      ci = CI_LTRANS;
   }

   if (!usb_read_int_from_ups(ups, ci, &result)) {
      pmsg("\nI don't know how to control the %s transfer voltage "
         " settings on your UPS.\n", text);
      return;
   }

   int newval;
   do {
      pmsg("Current %s transfer voltage setting: %d Volts\n", text, result);

      pmsg("Enter new %s transfer voltage (0 to cancel): ", text);
      cmd = get_cmd("");
      newval = atoi(cmd);

      // Check for exit
      if (newval == 0)
         return;

      // Write new value
      usb_write_int_to_ups(ups, ci, newval, text);

      // Give write a chance to work, then read new value
      sleep(1);
      result = 0;
      usb_read_int_from_ups(ups, ci, &result);

      if (result != newval) {
         pmsg("FAILED to set new %s transfer voltage.\n\n"
              "This is probably because you entered a value that is out-of-range\n"
              "for your UPS. The acceptable range of values varies based on UPS\n"
              "model. The UPS has probably set the value as close as possible to\n"
              "what you requested.\n\n", text);
      }
   }
   while (result != newval);

   pmsg("New %s transfer voltage setting: %d Volts\n", text, result);
}

static void usb_set_sens(void)
{
   int result;
   char* cmd;

   if (!usb_read_int_from_ups(ups, CI_SENS, &result)) {
      pmsg("\nI don't know how to control the alarm settings on your UPS.\n");
      return;
   }
   
   pmsg("Current sensitivity setting: ");
   switch(result) {
   case 0:
      pmsg("LOW\n");
      break;
   case 1:
      pmsg("MEDIUM\n");
      break;
   case 2:
      pmsg("HIGH\n");
      break;
   default:
      pmsg("UNKNOWN\n");
      break;
   }

   while(1) {
      pmsg("Press...\n"
           " L for Low sensitivity\n"
           " M for Medium sensitivity\n"
           " H for High sensitivity\n"
           " Q to Quit with no changes\n"
           "Your choice: ");
      cmd = get_cmd("Select function: ");
      if (cmd) {
         switch(tolower(*cmd)) {
         case 'l':
            usb_write_int_to_ups(ups, CI_SENS, 0, "CI_SENS");
            break;
         case 'm':
            usb_write_int_to_ups(ups, CI_SENS, 1, "CI_SENS");
            break;
         case 'h':
            usb_write_int_to_ups(ups, CI_SENS, 2, "CI_SENS");
            break;
         case 'q':
            return;
         default:
            pmsg("Illegal response.\n");
            continue;
         }
      } else {
         pmsg("Illegal response.\n");
         continue;
      }
      
      break;
   }
   
   /* Delay needed for readback to work */
   sleep(1);

   usb_read_int_from_ups(ups, CI_SENS, &result);
   pmsg("New sensitivity setting: ");
   switch(result) {
   case 0:
      pmsg("LOW\n");
      break;
   case 1:
      pmsg("MEDIUM\n");
      break;
   case 2:
      pmsg("HIGH\n");
      break;
   default:
      pmsg("UNKNOWN\n");
      break;
   }
}

static void usb_set_alarm(void)
{
   int result;
   char* cmd;

   if (!usb_read_int_from_ups(ups, CI_DALARM, &result)) {
      pmsg("\nI don't know how to control the alarm settings on your UPS.\n");
      return;
   }
   
   pmsg("Current alarm setting: ");
   switch(result) {
   case 1:
      pmsg("DISABLED\n");
      break;
   case 2:
      pmsg("ENABLED\n");
      break;
   default:
      pmsg("UNKNOWN\n");
      break;
   }
   
   while(1) {
      pmsg("Press...\n"
           " E to Enable alarms\n"
           " D to Disable alarms\n"
           " Q to Quit with no changes\n"
           "Your choice: ");
      cmd = get_cmd("Select function: ");
      if (cmd) {
         switch(tolower(*cmd)) {
         case 'e':
            usb_write_int_to_ups(ups, CI_DALARM, 2, "CI_DALARM");
            break;
         case 'd':
            usb_write_int_to_ups(ups, CI_DALARM, 1, "CI_DALARM");
            break;
         case 'q':
            return;
         default:
            pmsg("Illegal response.\n");
            continue;
         }
      } else {
         pmsg("Illegal response.\n");
         continue;
      }
      
      break;
   }
   
   /* Delay needed for readback to work */
   sleep(1);

   usb_read_int_from_ups(ups, CI_DALARM, &result);
   pmsg("New alarm setting: ");
   switch(result) {
   case 1:
      pmsg("DISABLED\n");
      break;
   case 2:
      pmsg("ENABLED\n");
      break;
   default:
      pmsg("UNKNOWN\n");
      break;
   }
}

static void usb_kill_power_test(void)
{
   pmsg("\nThis test will attempt to power down the UPS.\n"
        "The USB cable should be plugged in to the UPS, but the\n"
        "AC power plug to the UPS should be DISCONNECTED.\n\n"
        "PLEASE DO NOT RUN THIS TEST WITH A COMPUTER CONNECTED TO YOUR UPS!!!\n\n"
        "Please enter any character when ready to continue: ");

   fgetc(stdin);
   pmsg("\n");

   ptime();
   pmsg("calling kill_power function.\n");

   make_file(ups, ups->pwrfailpath);
   initiate_hibernate(ups);
   unlink(ups->pwrfailpath);

   ptime();
   pmsg("returned from kill_power function.\n");
}

static void usb_get_self_test_result(void)
{
   int result;

   if (!usb_read_int_from_ups(ups, CI_ST_STAT, &result)) {
      pmsg("\nI don't know how to run a self test on your UPS\n"
           "or your UPS does not support self test.\n");
      return;
   }

   pmsg("Result of last self test: ");
   switch (result) {
   case 1:
      pmsg("PASSED\n");
      break;
   case 2:
      pmsg("WARNING\n");
      break;
   case 3:
      pmsg("ERROR\n");
      break;
   case 4:
      pmsg("ABORTED\n");
      break;
   case 5:
      pmsg("IN PROGRESS\n");
      break;
   case 6:
      pmsg("NO TEST PERFORMED\n");
      break;
   default:
      pmsg("UNKNOWN (%02x)\n", result);
      break;
   }
}

static bool usb_clear_test_result()
{
   int timeout, result;

   pmsg("Clearing previous self test result...");

   // abort battery calibration in case it's in progress
   usb_write_int_to_ups(ups, CI_ST_STAT, 3, "SelftestStatus");

   if (!usb_write_int_to_ups(ups, CI_ST_STAT, 0, "SelftestStatus")) {
      pmsg("FAILED\n");
      return false;
   }

   for (timeout = 0; timeout < 10; timeout++) {
      if (!usb_read_int_from_ups(ups, CI_ST_STAT, &result)) {
         pmsg("FAILED\n");
         return false;
      }

      if (result == 6) {
         pmsg("CLEARED\n");
         break;
      }

      sleep(1);
   }

   if (timeout == 10) {
      pmsg("FAILED\n");
      return false;
   }
   
   return true;
}

static void usb_run_self_test(void)
{
   int result;
   int timeout;

   pmsg("\nThis test instructs the UPS to perform a self-test\n"
        "operation and reports the result when the test completes.\n\n");

   if (!usb_read_int_from_ups(ups, CI_ST_STAT, &result)) {
      pmsg("I don't know how to run a self test on your UPS\n"
           "or your UPS does not support self test.\n");
      return;
   }

   if (!usb_clear_test_result())
      return;

   pmsg("Initiating self test...");
   if (!usb_write_int_to_ups(ups, CI_ST_STAT, 1, "SelftestStatus")) {
      pmsg("FAILED\n");
      return;
   }

   pmsg("INITIATED\n");

   pmsg("Waiting for test to complete...");

   for (timeout = 0; timeout < 40; timeout++) {
      if (!usb_read_int_from_ups(ups, CI_ST_STAT, &result)) {
         pmsg("ERROR READING STATUS\n");
         usb_write_int_to_ups(ups, CI_ST_STAT, 3, "SelftestStatus");
         return;
      }

      if (result != 6) {
         pmsg("COMPLETED\n");
         break;
      }

      sleep(1);
   }

   if (timeout == 40) {
      pmsg("TEST DID NOT COMPLETE\n");
      usb_write_int_to_ups(ups, CI_ST_STAT, 3, "SelftestStatus");
      return;
   }

   usb_get_self_test_result();
}

static int usb_get_battery_date(void)
{
   int result;

   if (!usb_read_int_from_ups(ups, CI_BATTDAT, &result)) {
      pmsg("\nI don't know how to access the battery date on your UPS\n"
           "or your UPS does not support the battery date feature.\n");
      return 0;
   }

   /*
    * Date format is:
    *   YYYY|YYYM|MMMD|DDDD
    *   bit  0-4:  Day
    *        5-8:  Month
    *        9-15: Year-1980
    */
   pmsg("Current battery date: %02u/%02u/%04u\n",
      (result & 0x1e0) >> 5, result & 0x1f, 1980 + ((result & 0xfe00) >> 9));

   return result;
}

static void usb_set_battery_date(void)
{
   char *cmd;
   int result, day, month, year, temp, max;

   if (!(result = usb_get_battery_date()))
      return;

   cmd = get_cmd("Enter new battery date (MM/DD/YYYY), blank to quit: ");
   if (!isdigit(cmd[0]) || !isdigit(cmd[1]) || cmd[2] != '/' ||
       !isdigit(cmd[3]) || !isdigit(cmd[4]) || cmd[5] != '/' ||
       !isdigit(cmd[6]) || !isdigit(cmd[7]) || !isdigit(cmd[8]) ||
       !isdigit(cmd[9]) || cmd[10] != '\0' ||
       ((month = strtoul(cmd, NULL, 10)) > 12) || (month < 1) ||
       ((day = strtoul(cmd + 3, NULL, 10)) > 31) || (day < 1) ||
       ((year = strtoul(cmd + 6, NULL, 10)) < 1980)) {
      pmsg("Invalid format.\n");
      return;
   }

   result = ((year - 1980) << 9) | (month << 5) | day;

   pmsg("Writing new date...");
   if (!usb_write_int_to_ups(ups, CI_BATTDAT, result, "ManufactureDate")) {
      pmsg("FAILED\n");
      return;
   }

   pmsg("SUCCESS\n");

   pmsg("Waiting for change to take effect...");
   for (max = 0; max < 10; max++) {
      if (!usb_read_int_from_ups(ups, CI_BATTDAT, &temp)) {
         pmsg("ERROR\n");
         return;
      }

      if (temp == result)
         break;

      sleep(1);
   }

   if (max == 10)
      pmsg("TIMEOUT\n");
   else
      pmsg("SUCCESS\n");

   usb_get_battery_date();
}

static void usb_get_manf_date(void)
{
   int result;

   if (!usb_read_int_from_ups(ups, CI_MANDAT, &result)) {
      pmsg("\nI don't know how to access the manufacturing date on your UPS\n"
           "or your UPS does not support the manufacturing date feature.\n");
      return;
   }

   /*
    * Date format is:
    *   YYYY|YYYM|MMMD|DDDD
    *   bit  0-4:  Day
    *        5-8:  Month
    *        9-15: Year-1980
    */
   pmsg("Manufacturing date: %02u/%02u/%04u\n",
      (result & 0x1e0) >> 5, result & 0x1f, 1980 + ((result & 0xfe00) >> 9));
}

static void usb_calibration()
{
   int result;
   int aborted;
   int ilastbl;

   if (!ups->UPS_Cap[CI_ST_STAT] || 
       !ups->UPS_Cap[CI_BATTLEV] ||
       !ups->UPS_Cap[CI_LOAD]) {
      pmsg("\nI don't know how to run a battery calibration on your UPS\n"
             "or your UPS does not support battery calibration\n");
      return;
   }

   pmsg("This test instructs the UPS to perform a battery calibration\n"
        "operation and reports the result when the process completes.\n"
        "The battery level must be at 100%% and the load must be at least\n"
        "10%% to begin this test.\n\n");

   if (!usb_read_int_from_ups(ups, CI_BATTLEV, &result)) {
      pmsg("Failed to read current battery level\n");
      return;
   }

   if (result == 100) {
      pmsg("Battery level is %d%% -- OK\n", result);
   }
   else {
      pmsg("Battery level %d%% is insufficient to run test.\n", result);
      return;
   }

   if (!usb_read_int_from_ups(ups, CI_LOAD, &result)) {
      pmsg("Failed to read current load level\n");
      return;
   }

   if (result >= 10) {
      pmsg("Load level is %d%% -- OK\n", result);
   }
   else {
      pmsg("Load level %d%% is insufficient to run test.\n", result);
      return;
   }

   if (!usb_clear_test_result())
      return;

   pmsg("\nThe battery calibration should automatically end\n"
          "when the battery level drops below about 25%%.\n"
          "This process can take minutes or hours, depending on\n"
          "the size of your UPS and the load attached.\n\n");

   pmsg("Initiating battery calibration...");
   if (!usb_write_int_to_ups(ups, CI_ST_STAT, 2, "SelftestStatus")) {
      pmsg("FAILED\n");
      return;
   }

   pmsg("INITIATED\n\n");

   pmsg("Waiting for calibration to complete...\n"
        "To abort the calibration, press ENTER.\n");

	ilastbl = 0;
   while (1) {
#ifndef HAVE_MINGW
      fd_set rfds;
      struct timeval tv;
      FD_ZERO(&rfds);
      FD_SET(STDIN_FILENO, &rfds);
      tv.tv_sec = 10;
      tv.tv_usec = 0;

      aborted = select(STDIN_FILENO+1, &rfds, NULL, NULL, &tv) == 1;
      if (aborted) {
         while (fgetc(stdin) != '\n')
            ;
      }
#else
      aborted = false;
      for (int i = 0; i < 100 && !aborted; i++) {
         while (kbhit() && !aborted)
            aborted = getch() == '\r';
         if (!aborted)
         {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 100000000;
            nanosleep(&ts, NULL);
         }
      }
#endif

      if (aborted) {
         pmsg("\n\nUser input detected; aborting calibration...");
         if (!usb_write_int_to_ups(ups, CI_ST_STAT, 3, "SelftestStatus")) {
            pmsg("FAILED\n");
         }
         else {
            pmsg("ABORTED\n");
         }
         return;
      }
         
      if (!usb_read_int_from_ups(ups, CI_ST_STAT, &result)) {
         pmsg("\n\nError reading status; aborting calibration...");
         if (!usb_write_int_to_ups(ups, CI_ST_STAT, 3, "SelftestStatus")) {
            pmsg("FAILED\n");
         }
         else {
            pmsg("ABORTED\n");
         }
         return;
      }

      if (result != 5) {
         pmsg("\nCALIBRATION COMPLETED\n");
         break;
      }

      // Output the battery level
      if (usb_read_int_from_ups(ups, CI_BATTLEV, &result)) {
         if (ilastbl == result)
            pmsg(".");
         else
            pmsg("\nBattery level: %d%%", result);
         ilastbl = result;
	   }
      else
         pmsg(".");
   }

   usb_get_self_test_result();
}

static void usb_test_alarm(void)
{
   int result;

   if (!usb_read_int_from_ups(ups, CI_TESTALARM, &result))
   {
      pmsg("\nI don't know how to test the alarm on your UPS.\n");
      return;
   }
   
   // Write to UPS
   pmsg("Testing alarm...");
   usb_write_int_to_ups(ups, CI_TESTALARM, 1, "CI_TESTALARM");
   sleep(1);

   pmsg("COMPLETE\n");
}

static int usb_get_self_test_interval(void)
{
   int result;

   if (!usb_read_int_from_ups(ups, CI_STESTI, &result))
   {
      pmsg("\nI don't know how to access the self-test interval on your UPS\n"
           "or your UPS does not support the self-test interval feature.\n");
      return -1;
   }

   pmsg("Current Self-test interval: ");
   switch (result)
   {
   case 0:
      pmsg("None\n");
      break;
   case 1:
      pmsg("Power On\n");
      break;
   case 2:
      pmsg("7 days\n");
      break;
   case 3:
      pmsg("14 days\n");
      break;
   default:
      pmsg("UNKNOWN (%02x)\n", result);
      break;
   }

   return result;
}

static void usb_set_self_test_interval(void)
{
   if (usb_get_self_test_interval() == -1)
      return;

   while(1)
   {
      pmsg("Press...\n"
           " 0 for None\n"
           " 1 for On Power\n"
           " 2 for 7 Days\n"
           " 3 for 14 Days\n"
           " Q to Quit with no changes\n"
           "Your choice: ");
      char *cmd = get_cmd("Select function: ");
      if (cmd)
      {
         if (*cmd >= '0' && *cmd <= '3')
         {
            usb_write_int_to_ups(ups, CI_STESTI, *cmd-'0', "CI_STESTI");
            break;
         }
         else if (tolower(*cmd) == 'q')
            return;
         else
            pmsg("Illegal response.\n");
      }
      else
      {
         pmsg("Illegal response.\n");
      }
   }
   
   /* Delay needed for readback to work */
   sleep(1);
   usb_get_self_test_interval();
}

#endif

/* Get next input command from the terminal */
static char *get_cmd(const char *prompt)
{
   static char cmd[1000];

   pmsg(prompt);

   if (fgets(cmd, sizeof(cmd), stdin) == NULL)
      return NULL;

   write_file(cmd);

   pmsg("\n");
   strip_trailing_junk(cmd);

   return cmd;
}

/* Strip any trailing junk from the command */
static void strip_trailing_junk(char *cmd)
{
   char *p;

   p = cmd + strlen(cmd) - 1;

   /* strip trailing junk from command */
   while ((p >= cmd) && (*p == '\n' || *p == '\r' || *p == ' '))
      *p-- = 0;
}

#ifdef HAVE_APCSMART_DRIVER

/*
 * EPROM commands and their values as parsed from the
 * ^Z eprom string returned by the UPS.
 */
static struct {
   char cmd;
   char size;
   char num;
   char cmdvals[50];
} cmd[15];

/* Total number of EPROM commands */
static int ncmd = 0;

static UPSINFO eeprom_ups;

/*
 * Table of the UPS command, the apcupsd configuration directive,
 * and an explanation of what the command sets in the EPROM.
 */
static struct {
   char cmd;
   const char *config_directive;
   const char *descript;
   char type;
   int *data;
} cmd_table[] = {
   {'u', "HITRANSFER",   "Upper transfer voltage",  'i', &eeprom_ups.hitrans},
   {'l', "LOTRANSFER",   "Lower transfer voltage",  'i', &eeprom_ups.lotrans},
   {'e', "RETURNCHARGE", "Return threshold",        'i', &eeprom_ups.rtnpct},
   {'o', "OUTPUTVOLTS",  "Output voltage on batts", 'i', &eeprom_ups.NomOutputVoltage},
   {'s', "SENSITIVITY",  "Sensitivity",             'c', (int *)eeprom_ups.sensitivity},
   {'q', "LOWBATT",      "Low battery warning",     'i', &eeprom_ups.dlowbatt},
   {'p', "SLEEP",        "Shutdown grace delay",    'i', &eeprom_ups.dshutd},
   {'k', "BEEPSTATE",    "Alarm delay",             'c', (int *)eeprom_ups.beepstate},
   {'r', "WAKEUP",       "Wakeup delay",            'i', &eeprom_ups.dwake},
   {'E', "SELFTEST",     "Self test interval",      'c', (int *)eeprom_ups.selftest},
   {0,   NULL,           NULL}     /* Last entry */
};


static void print_valid_eeprom_values(UPSINFO *ups)
{
   int i, j, k, l;
   char *p;
   char val[100];;

   pmsg("\nValid EEPROM values for the %s\n\n", ups->mode.long_name);

   memcpy(&eeprom_ups, ups, sizeof(UPSINFO));

   pmsg("%-24s %-12s  %-6s  %s\n", "           ", "Config", "Current", "Permitted");
   pmsg("%-24s %-12s  %-6s  %s\n", "Description", "Directive", "Value  ", "Values");
   pmsg("===================================================================\n");

   for (i = 0; i < ncmd; i++) {
      for (j = 0; cmd_table[j].cmd; j++) {
         if (cmd[i].cmd == cmd_table[j].cmd) {
            if (cmd_table[j].type == 'c')
               asnprintf(val, sizeof(val), "%s", (char *)cmd_table[j].data);
            else
               asnprintf(val, sizeof(val), "%d", *cmd_table[j].data);

            pmsg("%-24s %-12s  %-6s   ", cmd_table[j].descript,
               cmd_table[j].config_directive, val);

            p = cmd[i].cmdvals;
            for (k = cmd[i].num; k; k--) {
               for (l = cmd[i].size; l; l--)
                  pmsg("%c", *p++);

               pmsg(" ");
            }

            pmsg("\n");
            break;
         }
      }
   }

   pmsg("===================================================================\n");

   tcflush(ups->fd, TCIOFLUSH);
   smart_poll('Y', ups);
   smart_poll('Y', ups);

   pmsg("Battery date: %s\n", smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups));
   pmsg("UPS Name    : %s\n", smart_poll(ups->UPS_Cmd[CI_IDEN], ups));
   pmsg("\n");
}

/*
 * Parse EPROM command string returned by a ^Z command. We
 * pull out only entries that correspond to our UPS (locale).
 */
static void parse_eeprom_cmds(char *eprom, char locale)
{
   char *p = eprom;
   char c, l, n, s;
#ifdef debuggggggg
   int i;
#endif

   ncmd = 0;
   for (;;) {
      c = *p++;
      if (c == 0)
         break;

      if (c == '#')
         continue;

      l = *p++;                    /* get locale */
      n = *p++ - '0';              /* get number of commands */
      s = *p++ - '0';              /* get character size */

      if (l != '4' && l != locale) {    /* skip this locale */
         p += n * s;
         continue;
      }

      cmd[ncmd].cmd = c;           /* store command */
      cmd[ncmd].size = s;          /* chare length of each value */
      cmd[ncmd].num = n;           /* number of values */

      strncpy(cmd[ncmd].cmdvals, p, n * s);     /* save values */
      p += n * s;
      ncmd++;
   }
#ifdef debuggggggg
   printf("\n");
   for (i = 0; i < ncmd; i++) {
      printf("cmd=%c len=%d nvals=%d vals=%s\n", cmd[i].cmd,
         cmd[i].size, cmd[i].num, cmd[i].cmdvals);
   }
#endif
}

/*********************************************************************/
static void print_eeprom_values(UPSINFO *ups)
{
   char locale, locale1, locale2;

   pmsg("Doing prep_device() ...\n");
   prep_device(ups);
   if (!ups->UPS_Cap[CI_EPROM])
      Error_abort0("Your model does not support EPROM programming.\n");

   if (ups->UPS_Cap[CI_REVNO])
      locale1 = *(ups->firmrev + strlen(ups->firmrev) - 1);
   else
      locale1 = 0;

   if (ups->UPS_Cap[CI_UPSMODEL])
      locale2 = *(ups->upsmodel + strlen(ups->upsmodel) - 1);
   else
      locale2 = 0;

   if (locale1 == locale2 && locale1 == 0)
      Error_abort0("Your model does not support EPROM programming.\n");

   if (locale1 == locale2)
      locale = locale1;

   if (locale1 == 0)
      locale = locale2;
   else
      locale = locale1;

   parse_eeprom_cmds(ups->eprom, locale);
   print_valid_eeprom_values(ups);
}
#endif
