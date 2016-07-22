/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include "rc.h"

#include <sys/reboot.h>
#include <wlutils.h>
#include <wlioctl.h>

//	#define DEBUG_TEST

static int gf;

static int get_btn(const char *name, uint32_t *bit, uint32_t *pushed)
{
	int gpio;
	int inv;
	
	if (nvget_gpio(name, &gpio, &inv)) {
		*bit = 1 << gpio;
		*pushed = inv ? 0 : *bit;
		return 1;
	}
	return 0;
}

int buttons_main(int argc, char *argv[])
{
	uint32_t gpio;
	uint32_t mask;
	uint32_t last;
	uint32_t ses_mask;
	uint32_t ses_pushed;
	uint32_t reset_mask;
	uint32_t reset_pushed;
	uint32_t brau_mask;
	uint32_t brau_state;
	int brau_count_stable;
	int brau_flag;
	int count;
	char s[16];
	char *p;
	int n;
	int ses_led;

	ses_mask = ses_pushed = 0;
	reset_pushed = 0;
	brau_mask = 0;
	brau_state = ~0;
	ses_led = LED_DIAG;

	// moveme
	switch (nvram_get_int("btn_override") ? MODEL_UNKNOWN : get_model()) {
	case MODEL_WRT54G:
	case MODEL_WRTSL54GS:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		ses_led = LED_DMZ;
		break;
/*		
	case MODEL_WRH54G:
		reset_mask = 1 << 6;
		break;
*/
	case MODEL_WTR54GS:
		reset_mask = 1 << 3;
		ses_mask = 1 << 2;
		break;
	case MODEL_WHRG54S:
	case MODEL_WHRHPG54:
	case MODEL_WHR2A54G54:
	case MODEL_WHR3AG54:
	case MODEL_WHRG125:
		reset_mask = reset_pushed = 1 << 4;
		ses_mask = 1 << 0;
		brau_mask = 1 << 5;
		break;
	case MODEL_WBRG54:
		reset_mask = reset_pushed = 1 << 4;
		break;
	case MODEL_WBR2G54:
		reset_mask = reset_pushed = 1 << 7;
		ses_mask = ses_pushed = 1 << 4;
		ses_led = LED_AOSS;
		break;
	case MODEL_WZRG54:
	case MODEL_WZRHPG54:
	case MODEL_WZRRSG54:
	case MODEL_WZRRSG54HP:
	case MODEL_WVRG54NF:
		reset_mask = reset_pushed = 1 << 4;
		ses_mask = 1 << 0;
		ses_led = LED_AOSS;
		break;
	case MODEL_WZRG108:
		reset_mask = reset_pushed = 1 << 7;
		ses_mask = 1 << 0;
		ses_led = LED_AOSS;
		break;
	case MODEL_WR850GV1:
		reset_mask = 1 << 0;
		break;
	case MODEL_WR850GV2:
	case MODEL_WR100:
		reset_mask = 1 << 5;
		break;
	case MODEL_WL500GP:
		reset_mask = reset_pushed = 1 << 0;
		ses_mask = ses_pushed = 1 << 4;
		break;
	case MODEL_WL500W:
		reset_mask = reset_pushed = 1 << 6;
		ses_mask = ses_pushed = 1 << 7;
		break;		
	case MODEL_DIR320:
	case MODEL_H618B:
		reset_mask = 1 << 7;
		ses_mask = 1 << 6;	// WLAN button on H618B
		break;		
	case MODEL_WL500GPv2:
	case MODEL_WL520GU:
	case MODEL_WL330GE:
		reset_mask = 1 << 2;
		ses_mask = 1 << 3;
		break;		
//	case MODEL_MN700:
//?		reset_mask = reset_pushed = 1 << 7;
//		break;
	case MODEL_WLA2G54L:
		reset_mask = reset_pushed = 1 << 7;
		break;
	case MODEL_WL1600GL:
		reset_mask = 1 << 3;
		ses_mask = 1 << 4;
		ses_led = LED_AOSS;
		break;
#ifdef CONFIG_BCMWL5
	case MODEL_RTN10:
		reset_mask = 1 << 3;
		ses_mask = 1 << 2;
		break;
	case MODEL_RTN10U:
		reset_mask = 1 << 21;
		ses_mask = 1 << 20;
		ses_led = LED_AOSS;
		break;
	case MODEL_RTN10P:
		reset_mask = 1 << 20;
		ses_mask = 1 << 21;
		ses_led = LED_AOSS;
		break;
	case MODEL_RTN12:
		reset_mask = 1 << 1;
		ses_mask = 1 << 0;
		brau_mask = (1 << 4) | (1 << 5) | (1 << 6);
		break;
	case MODEL_RTN15U:
		reset_mask = 1 << 5;
		ses_mask = 1 << 8;
		break;
	case MODEL_RTN16:
		reset_mask = 1 << 6;
		ses_mask = 1 << 8;
		break;
	case MODEL_RTN53:
		reset_mask = 1 << 3;
		ses_mask = 1 << 7;
		break;
	case MODEL_RTN53A1:
		reset_mask = 1 << 7;
		ses_mask = 1 << 3;
		break;
	case MODEL_RTN66U:
		reset_mask = 1 << 9;
		ses_mask = 1 << 4;
		break;
	case MODEL_RTN18U:
		reset_mask = 1 << 7;
		ses_mask = 1 << 11;
		break;
	case MODEL_RTAC56U:
		reset_mask = 1 << 11;
		ses_mask = 1 << 15;
		ses_led = LED_AOSS;
		break;
	case MODEL_RTAC68U:
	case MODEL_DIR868L:
		reset_mask = 1 << 11;
		ses_mask = 1 << 7;
		ses_led = LED_AOSS;
		break;
	case MODEL_WS880:
		reset_mask = 1 << 2;
		ses_mask = 1 << 3;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6500V1:
		reset_mask = 1 << 3;
		ses_mask = 1 << 4;
		break;
	case MODEL_EA6700:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_EA6900:
		ses_mask = 1 << 7;
		reset_mask = 1 << 11;
		ses_led = LED_AOSS;
		break;
	case MODEL_R1D:
		reset_mask = 1 << 17;
 		ses_led = LED_AOSS;
 		break;
	case MODEL_W1800R:
		reset_mask = 1 << 14;
		break;
	case MODEL_D1800H:
		reset_mask = 1 << 5;
		break;
	case MODEL_R6400:
		reset_mask = 1 << 5;
		ses_mask = 1 << 4;
		ses_led = LED_AOSS;
		break;
	case MODEL_R6250:
	case MODEL_R6300v2:
	case MODEL_R7000:
		reset_mask = 1 << 6;
		ses_mask = 1 << 5;
		ses_led = LED_AOSS;
		break;
	case MODEL_WZR1750:
//		reset_mask = 1 << 6;
		ses_mask = 1 << 12;
		ses_led = LED_AOSS;
		break;
	case MODEL_WNR3500L:
	case MODEL_WNR3500LV2:
		reset_mask = 1 << 4;
		ses_mask = 1 << 6;
		ses_led = LED_AOSS;
		break;
	case MODEL_WNR2000v2:
		reset_mask = 1 << 1;
		ses_mask = 1 << 0;
		ses_led = LED_AOSS;
		break;
	case MODEL_F7D3301:
	case MODEL_F7D3302:
	case MODEL_F7D4301:
	case MODEL_F7D4302:
	case MODEL_F5D8235v3:
		reset_mask = 1 << 6;
		ses_mask = 1 << 8;
		ses_led = LED_AOSS;
		break;
	case MODEL_E900:
	case MODEL_E1000v2:
	case MODEL_E1500:
	case MODEL_E1550:
	case MODEL_E2500:
		reset_mask = 1 << 10;
		ses_mask = 1 << 9;
		break;
	case MODEL_E3200:
		reset_mask = 1 << 5;
		ses_mask = 1 << 8;
		break;
	case MODEL_WRT160Nv3:
		reset_mask = 1 << 6;
		ses_mask = 1 << 5;
		break;
	case MODEL_WRT320N:
		reset_mask = 1 << 8;
		ses_mask = 1 << 5;
		ses_led = LED_AMBER;
		break;
	case MODEL_WRT610Nv2:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		ses_led = LED_AMBER;
		break;
	case MODEL_E4200:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		ses_led = LED_WHITE;
		break;
	case MODEL_L600N:
		reset_mask = 1 << 21;
		ses_mask = 1 << 20;
		//wlan button = 1 >> 10
		break;
	case MODEL_DIR620C1:
		reset_mask = 1 << 21;
		ses_mask = 1 << 20;
		break;
#endif
	case MODEL_WRT160Nv1:
	case MODEL_WRT300N:
		reset_mask = 1 << 6;
		ses_mask = 1 << 4;
		break;
	case MODEL_WRT310Nv1:
		reset_mask = 1 << 6;
		ses_mask = 1 << 8;
		break;
	// Added by BWQ
	case MODEL_RG200E_CA:
	case MODEL_H218N:
		reset_mask = 1 << 30;
		ses_mask = 1 << 28;
		break;
	case MODEL_HG320:
		reset_mask = 1 << 30;
		ses_mask = 1 << 29;
		break;
	case MODEL_TDN60:
		reset_mask = 1 << 8;
		break;
	case MODEL_TDN6:
		reset_mask = 1 << 20;
		break;
	// BWQ end
	default:
		get_btn("btn_ses", &ses_mask, &ses_pushed);
		if (!get_btn("btn_reset", &reset_mask, &reset_pushed)) {
//			fprintf(stderr, "Not supported.\n");
			return 1;
		}
		break;
	}
	mask = reset_mask | ses_mask | brau_mask;

#ifdef DEBUG_TEST
	cprintf("reset_mask=0x%X reset_pushed=0x%X\n", reset_mask, reset_pushed);
	cprintf("ses_mask=0x%X\n", ses_mask);
	cprintf("brau_mask=0x%X\n", brau_mask);
	cprintf("ses_led=%d\n", ses_led);
#else
	if (fork() != 0) return 0;
	setsid();
#endif

	signal(SIGCHLD, chld_reap);

	if ((gf = gpio_open(mask)) < 0) return 1;

	last = 0;
	brau_count_stable = 0;
	brau_flag = 0;
	while (1) {
		if (((gpio = _gpio_read(gf)) == ~0) || (last == (gpio &= mask) && !brau_flag) || (check_action() != ACT_IDLE)) {
#ifdef DEBUG_TEST
			cprintf("gpio = %X\n", gpio);
#endif
			sleep(1);
			continue;
		}

		if ((gpio & reset_mask) == reset_pushed) {
#ifdef DEBUG_TEST
			cprintf("reset down\n");
#endif

			led(LED_DIAG, 0);

			count = 0;
			do {
				sleep(1);
				if (++count == 3) led(LED_DIAG, 1);
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & reset_mask) == reset_pushed));

#ifdef DEBUG_TEST
			cprintf("reset count = %d\n", count);
#else
			if (count >= 3) {
				eval("mtd-erase2", "nvram");
				//nvram_set("restore_defaults", "1");
				//nvram_commit();
				sync();
				reboot(RB_AUTOBOOT);
			}
			else {
				led(LED_DIAG, 1);
				set_action(ACT_REBOOT);
				kill(1, SIGTERM);
			}
			exit(0);
#endif
		}

		if ((ses_mask) && ((gpio & ses_mask) == ses_pushed)) {
			count = 0;
			do {
				//	syslog(LOG_DEBUG, "ses-pushed: gpio=x%X, pushed=x%X, mask=x%X, count=%d", gpio, ses_pushed, ses_mask, count);

				led(ses_led, LED_ON);
				usleep(500000);
				led(ses_led, LED_OFF);
				usleep(500000);
				++count;
			} while (((gpio = _gpio_read(gf)) != ~0) && ((gpio & ses_mask) == ses_pushed));
			gpio &= mask;

			if ((ses_led == LED_DMZ) && (nvram_get_int("dmz_enable") > 0)) led(LED_DMZ, 1);

			//	syslog(LOG_DEBUG, "ses-released: gpio=x%X, pushed=x%X, mask=x%X, count=%d", gpio, ses_pushed, ses_mask, count);
			syslog(LOG_INFO, "SES pushed. Count was %d.", count);

			if ((count != 3) && (count != 7) && (count != 11)) {
				n = count >> 2;
				if (n > 3) n = 3;
				/*
					0-2  = func0
					4-6  = func1
					8-10 = func2
					12+  = func3
				*/

#ifdef DEBUG_TEST
				cprintf("ses func=%d\n", n);
#else
				sprintf(s, "sesx_b%d", n);
				//	syslog(LOG_DEBUG, "ses-func: count=%d %s='%s'", count, s, nvram_safe_get(s));
				if ((p = nvram_get(s)) != NULL) {
					switch (*p) {
					case '1':	// toggle wl
						nvram_set("rrules_radio", "-1");
						eval("radio", "toggle");
						break;
					case '2':	// reboot
						kill(1, SIGTERM);
						break;
					case '3':	// shutdown
						kill(1, SIGQUIT);
						break;
					case '4':	// run a script
						sprintf(s, "%d", count);
						run_nvscript("sesx_script", s, 2);
						break;
#ifdef TCONFIG_USB
					case '5':	// !!TB: unmount all USB drives
						add_remove_usbhost("-2", 0);
						break;
#endif
					}
				}
#endif

			}
		}

		if (brau_mask) {
			if (last == gpio)
				sleep(1);
			last = (gpio & brau_mask);
			if (brau_state != last) {
				brau_flag = (brau_state != ~0); // set to 1 to run at startup
				brau_state = last;
				brau_count_stable = 0;
			}
			else if (brau_flag && ++brau_count_stable > 2) { // stable for 2+ seconds
				brau_flag = 0;
				switch (nvram_get_int("btn_override") ? MODEL_UNKNOWN : get_model()) {
#ifdef CONFIG_BCMWL5
				case MODEL_RTN12:
					p = (brau_state & (1 << 4)) ? "ap" : (brau_state & (1 << 5)) ? "repeater" : "router";
					break;
#endif
				default:
					p = brau_state ? "auto" : "bridge";
					break;
				}
				nvram_set("brau_state", p);
#ifdef DEBUG_TEST
				cprintf("bridge/auto state = %s\n", p);
#else
				run_nvscript("script_brau", p, 2);
#endif
			}
		}

		last = gpio;
	}

	return 0;
}
