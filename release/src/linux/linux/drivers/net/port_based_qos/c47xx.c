#include <typedefs.h>
#include <sbutils.h>
#include <bcmdevs.h>
#include <osl.h>
#include <bcmnvram.h>
#include <bcmutils.h>

#include <sbpci.h>
#include <sbchipc.h>
#include <sbconfig.h>
#include <sbextif.h>
#include <sbmips.h>
#include "c47xx.h"
extern uint32 sb_gpioouten(void *sbh, uint32 mask, uint32 val);
extern uint32 sb_gpioout(void *sbh, uint32 mask, uint32 val);
extern uint32 sb_gpioin(void *sbh);
extern uint32 sb_gpiointmask(void *sbh, uint32 mask, uint32 val);

#define OUTENMASK	B_RESET|B_ECS|B_ECK|B_EDI
#define CFGMASK		B_ECS|B_ECK|B_EDI
#define BIT(x)	(1 << (x))
#define	ASSERT(exp)		do {} while (0)

void
conf_gpio(int x)
{
	ASSERT(sbh);

	/* Enable all of output pins */
	sb_gpioouten(sbh, OUTENMASK, OUTENMASK);

	/* We don't want the B_RESET pin changed, unless
	 * it tries to set the B_RESET pin.
	 */
	if (x & B_RESET)
		sb_gpioout(sbh, OUTENMASK, x);
	else
		sb_gpioout(sbh, CFGMASK, x);
	
}
	
void
gpio_line_set(int x, unsigned int value)
{
	ASSERT(sbh);

	if (value == 1)
		sb_gpioout(sbh, BIT(x), BIT(x));
	else if (value == 0)
		sb_gpioout(sbh, BIT(x), 0);
}

void
gpio_line_get(int x, int *value)
{
	ASSERT(sbh);

	*value = (sb_gpioin(sbh) >> x) & 0x1;
}

void
gpio_line_config_in(int x)
{
	ASSERT(sbh);
	
	sb_gpioouten(sbh, BIT(x), 0);
	sb_gpiointmask(sbh, BIT(x), BIT(x));
}

void
gpio_line_config_out(int x)
{
	ASSERT(sbh);

	sb_gpiointmask(sbh, BIT(x), 0);
	sb_gpioouten(sbh, BIT(x), BIT(x));
}

void
gpio_line_config_out_all(int x)
{
	ASSERT(sbh);

	sb_gpioouten(sbh, OUTENMASK, OUTENMASK);
}
