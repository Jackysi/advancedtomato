/* PWCX module glue code. */

#include <linux/config.h>

#include "pwc.h"
#include "pwcx.h"
#include "pwc-uncompress.h"

#define PWCX_MAJOR	9
#define PWCX_MINOR	0

#ifdef CONFIG_USB_PWCX_MODULE
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>

MODULE_DESCRIPTION("Philips webcam decompressor routines");
MODULE_AUTHOR("Nemosoft Unv. <webcam@smcc.demon.nl>");
MODULE_LICENSE("Proprietary. See http://www.smcc.demon.nl/webcam/tainting.html");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
EXPORT_NO_SYMBOLS;
#endif

static void lock_pwcx(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	MOD_INC_USE_COUNT;
#endif	
}

static void unlock_pwcx(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	MOD_DEC_USE_COUNT;
#endif	
}

#else

static void lock_pwcx(void)
{
}

static void unlock_pwcx(void)
{
}

#endif

static struct pwc_decompressor Nala[2] = 
{
   {
      645,
      5000,
      pwcx_init_decompress_Nala,
      pwcx_exit_decompress_Nala,
      pwcx_decompress_Nala,
      lock_pwcx,
      unlock_pwcx,
   },
   {
      646,
      5000,
      pwcx_init_decompress_Nala,
      pwcx_exit_decompress_Nala,
      pwcx_decompress_Nala,
      lock_pwcx,
      unlock_pwcx,
   },
};

static struct pwc_decompressor Timon[3] =
{
   {
      675,
      60000,
      pwcx_init_decompress_Timon,
      pwcx_exit_decompress_Timon,
      pwcx_decompress_Timon,
      lock_pwcx,
      unlock_pwcx,
   },
   {
      680,
      60000,
      pwcx_init_decompress_Timon,
      pwcx_exit_decompress_Timon,
      pwcx_decompress_Timon,
      lock_pwcx,
      unlock_pwcx,
   },
   {
      690,
      60000,
      pwcx_init_decompress_Timon,
      pwcx_exit_decompress_Timon,
      pwcx_decompress_Timon,
      lock_pwcx,
      unlock_pwcx,
   },
};

static struct pwc_decompressor Kiara[4] =
{
   {
      720,
      60000,
      pwcx_init_decompress_Kiara,
      pwcx_exit_decompress_Kiara,
      pwcx_decompress_Kiara,
      lock_pwcx,
      unlock_pwcx,
   },
   {
      730,
      60000,
      pwcx_init_decompress_Kiara,
      pwcx_exit_decompress_Kiara,
      pwcx_decompress_Kiara,
      lock_pwcx,
      unlock_pwcx,
   },
   {
      740,
      60000,
      pwcx_init_decompress_Kiara,
      pwcx_exit_decompress_Kiara,
      pwcx_decompress_Kiara,
      lock_pwcx,
      unlock_pwcx,
   },
   {
      750,
      60000,
      pwcx_init_decompress_Kiara,
      pwcx_exit_decompress_Kiara,
      pwcx_decompress_Kiara,
      lock_pwcx,
      unlock_pwcx,
   },
};

#ifdef CONFIG_USB_PWCX_MODULE
static int __init usb_pwcx_init(void)
#else
int usb_pwcx_init(void)
#endif
{
	Info("Philips webcam decompressor routines version %d.%d-BETA-2\n", PWCX_MAJOR, PWCX_MINOR);
	Info("Supports all cameras supported by the main module (pwc).\n");

	/* register decompression modules */
	if (pwc_decompressor_version != PWCX_MAJOR) {
		Err("Version mismatch! These decompression routines are version %d.*, while the\n"
		    "main module expects version %d.*. Please consult the Philips webcam Linux\n"
                    "driver page for the correct version and downloads.\n", PWCX_MAJOR, pwc_decompressor_version);
		    return -EINVAL;
	}
	pwc_register_decompressor(&Nala[0]);
	pwc_register_decompressor(&Nala[1]);
	pwc_register_decompressor(&Timon[0]);
	pwc_register_decompressor(&Timon[1]);
	pwc_register_decompressor(&Timon[2]);
	pwc_register_decompressor(&Kiara[0]);
	pwc_register_decompressor(&Kiara[1]);
	pwc_register_decompressor(&Kiara[2]);
	pwc_register_decompressor(&Kiara[3]);
	return 0;
}

#ifdef CONFIG_USB_PWCX_MODULE
static void __exit usb_pwcx_exit(void)
#else
void usb_pwcx_exit(void)
#endif
{
	pwc_unregister_decompressor(Nala[0].type);
	pwc_unregister_decompressor(Nala[1].type);
	pwc_unregister_decompressor(Timon[0].type);
	pwc_unregister_decompressor(Timon[1].type);
	pwc_unregister_decompressor(Timon[2].type);
	pwc_unregister_decompressor(Kiara[0].type);
	pwc_unregister_decompressor(Kiara[1].type);
	pwc_unregister_decompressor(Kiara[2].type);
	pwc_unregister_decompressor(Kiara[3].type);
	Info("Philips webcam decompressor routines removed.\n");
}

#ifdef CONFIG_USB_PWCX_MODULE
module_init(usb_pwcx_init);
module_exit(usb_pwcx_exit);
#endif
