/* $Id: natpmp.h,v 1.5 2009/11/06 20:18:20 nanard Exp $ */
/* MiniUPnP project
 * author : Thomas Bernard
 * website : http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 */
#ifndef __NATPMP_H__
#define __NATPMP_H__

#define NATPMP_PORT (5351)
#define NATPMP_NOTIF_ADDR	("224.0.0.1")

int OpenAndConfNATPMPSocket();

void ProcessIncomingNATPMPPacket(int s);

int ScanNATPMPforExpiration();

int CleanExpiredNATPMP();

void SendNATPMPPublicAddressChangeNotification(int * sockets, int n_sockets);

#endif

