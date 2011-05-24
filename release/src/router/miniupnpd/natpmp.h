/* $Id: natpmp.h,v 1.7 2011/05/14 13:43:35 nanard Exp $ */
/* MiniUPnP project
 * author : Thomas Bernard
 * website : http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 */
#ifndef __NATPMP_H__
#define __NATPMP_H__

#define NATPMP_PORT (5351)
#define NATPMP_NOTIF_ADDR	("224.0.0.1")

int OpenAndConfNATPMPSockets(int * sockets);

void ProcessIncomingNATPMPPacket(int s);

int ScanNATPMPforExpiration(void);

int CleanExpiredNATPMP(void);

void SendNATPMPPublicAddressChangeNotification(int * sockets, int n_sockets);

#endif

