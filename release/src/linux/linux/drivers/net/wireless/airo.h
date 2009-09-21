#ifndef _AIRO_H_
#define _AIRO_H_

struct net_device *init_airo_card(unsigned short irq, int port, int is_pcmcia);
void stop_airo_card(struct net_device *dev, int freeres);
int reset_airo_card(struct net_device *dev);

#endif  /*  _AIRO_H_  */
