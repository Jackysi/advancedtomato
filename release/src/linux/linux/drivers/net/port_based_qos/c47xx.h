#ifndef _C47XX_H_
#define _C47XX_H_

extern void	*bcm947xx_sbh;
#define sbh bcm947xx_sbh

#define GPIO0	0
#define GPIO1	1
#define GPIO2	2
#define GPIO3	3
#define GPIO4	4
#define GPIO5	5
#define GPIO6	6
#define GPIO7	7
#define GPIO8	8

#define B_RESET		1<<GPIO0
#define B_ECS		1<<GPIO2
#define B_ECK		1<<GPIO3
#define B_EDO		1<<GPIO4
#define B_EDI		1<<GPIO5

#endif
