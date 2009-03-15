/*

	Copyright (C) 2008-2009 Keith Moyer, tomatovpn@keithmoyer.com

	No part of this file may be used without permission.

*/

#include "rc.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

// Line number as text string
#define __LINE_T__ __LINE_T_(__LINE__)
#define __LINE_T_(x) __LINE_T(x)
#define __LINE_T(x) # x

#define VPN_LOG_ERROR -1
#define VPN_LOG_NOTE 0
#define VPN_LOG_INFO 1
#define VPN_LOG_EXTRA 2
#define vpnlog(level,x...) if(nvram_get_int("vpn_debug")>=level) syslog(LOG_INFO, "VPN DEBUG: " __LINE_T__ ": " x)

#define CLIENT_IF_START 10
#define SERVER_IF_START 20

#define BUF_SIZE 96
#define IF_SIZE 8

void start_vpnclient(int clientNum)
{
	FILE *fp;
	char buffer[BUF_SIZE];
	char iface[IF_SIZE];
	char *argv[5];
	int argc = 0;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	enum { TAP, TUN } ifType = TUN;
	enum { BRIDGE, NAT, NONE } routeMode = NONE;
	int nvi, ip[4], nm[4];

	vpnlog(VPN_LOG_INFO,"VPN GUI client backend starting...");

	sprintf(&buffer[0], "vpnclient%d", clientNum);
	if ( pidof(&buffer[0]) >= 0 )
	{
		vpnlog(VPN_LOG_NOTE, "VPN Client %d already running...", clientNum);
		vpnlog(VPN_LOG_INFO,"PID: %d", pidof(&buffer[0]));
		return;
	}

	// Determine interface
	sprintf(&buffer[0], "vpn_client%d_if", clientNum);
	if ( nvram_contains_word(&buffer[0], "tap") )
		ifType = TAP;
	else if ( nvram_contains_word(&buffer[0], "tun") )
		ifType = TUN;
	else
	{
		vpnlog(VPN_LOG_ERROR, "Invalid interface type, %.3s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Build interface name
	snprintf(&iface[0], IF_SIZE, "%s%d", nvram_safe_get(&buffer[0]), clientNum+CLIENT_IF_START);

	// Determine encryption mode
	sprintf(&buffer[0], "vpn_client%d_crypt", clientNum);
	if ( nvram_contains_word(&buffer[0], "tls") )
		cryptMode = TLS;
	else if ( nvram_contains_word(&buffer[0], "secret") )
		cryptMode = SECRET;
	else if ( nvram_contains_word(&buffer[0], "custom") )
		cryptMode = CUSTOM;
	else
	{
		vpnlog(VPN_LOG_ERROR,"Invalid encryption mode, %.6s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Determine if we should bridge the tunnel
	sprintf(&buffer[0], "vpn_client%d_bridge", clientNum);
	if ( ifType == TAP && nvram_get_int(&buffer[0]) == 1 )
		routeMode = BRIDGE;

	// Determine if we should NAT the tunnel
	sprintf(&buffer[0], "vpn_client%d_nat", clientNum);
	if ( (ifType == TUN || routeMode != BRIDGE) && nvram_get_int(&buffer[0]) == 1 )
		routeMode = NAT;

	// Make sure openvpn directory exists
	mkdir("/etc/openvpn", 0700);

	// Make sure symbolic link exists
	sprintf(&buffer[0], "/etc/openvpn/vpnclient%d", clientNum);
	unlink(&buffer[0]);
	if ( symlink("/usr/sbin/openvpn", &buffer[0]) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating symlink failed...");
		stop_vpnclient(clientNum);
		return;
	}

	// Make sure module is loaded
	modprobe("tun");

	// Create tap/tun interface
	sprintf(&buffer[0], "openvpn --mktun --dev %s", &iface[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating tunnel interface failed...");
		stop_vpnclient(clientNum);
		return;
	}

	// Bring interface up (TAP only)
	if( ifType == TAP )
	{
		if ( routeMode == BRIDGE )
		{
			snprintf(&buffer[0], BUF_SIZE, "brctl addif %s %s", nvram_safe_get("lan_ifname"), &iface[0]);
			for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
			if ( _eval(argv, NULL, 0, NULL) )
			{
				vpnlog(VPN_LOG_ERROR,"Adding tunnel interface to bridge failed...");
				stop_vpnclient(clientNum);
				return;
			}
		}

		snprintf(&buffer[0], BUF_SIZE, "ifconfig %s promisc up", nvram_safe_get("lan_ifname"));
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if ( _eval(argv, NULL, 0, NULL) )
		{
			vpnlog(VPN_LOG_ERROR,"Bringing interface up failed...");
			stop_vpnclient(clientNum);
			return;
		}
	}

	// Build and write config file
	vpnlog(VPN_LOG_EXTRA,"Writing config file");
	sprintf(&buffer[0], "/etc/openvpn/client%d.ovpn", clientNum);
	fp = fopen(&buffer[0], "w");
	chmod(&buffer[0], S_IRUSR|S_IWUSR);
	fprintf(fp, "# Automatically generated configuration\n");
	fprintf(fp, "daemon\n");
	if ( cryptMode == TLS )
		fprintf(fp, "client\n");
	fprintf(fp, "dev %s\n", &iface[0]);
	sprintf(&buffer[0], "vpn_client%d_proto", clientNum);
	fprintf(fp, "proto %s\n", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_client%d_addr", clientNum);
	fprintf(fp, "remote %s ", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_client%d_port", clientNum);
	fprintf(fp, "%d\n", nvram_get_int(&buffer[0]));
	if ( cryptMode == SECRET )
	{
		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_client%d_local", clientNum);
			fprintf(fp, "ifconfig %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_client%d_remote", clientNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
		}
		else if ( ifType == TAP )
		{
			sprintf(&buffer[0], "vpn_client%d_local", clientNum);
			fprintf(fp, "ifconfig %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_client%d_nm", clientNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
		}
	}
	sprintf(&buffer[0], "vpn_client%d_retry", clientNum);
	if ( (nvi = nvram_get_int(&buffer[0])) >= 0 )
		fprintf(fp, "resolv-retry %d\n", nvi);
	else
		fprintf(fp, "resolv-retry infinite\n");
	fprintf(fp, "nobind\n");
	fprintf(fp, "persist-key\n");
	fprintf(fp, "persist-tun\n");
	sprintf(&buffer[0], "vpn_client%d_comp", clientNum);
	fprintf(fp, "comp-lzo %s\n", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_client%d_cipher", clientNum);
	if ( !nvram_contains_word(&buffer[0], "default") )
		fprintf(fp, "cipher %s\n", nvram_safe_get(&buffer[0]));
	fprintf(fp, "verb 3\n");
	if ( cryptMode == TLS )
	{
		sprintf(&buffer[0], "vpn_client%d_hmac", clientNum);
		nvi = nvram_get_int(&buffer[0]);
		if ( nvi >= 0 )
		{
			fprintf(fp, "tls-auth client%d-static.key", clientNum);
			if ( nvi < 2 )
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}
			
		fprintf(fp, "ca client%d-ca.crt\n", clientNum);
		fprintf(fp, "cert client%d.crt\n", clientNum);
		fprintf(fp, "key client%d.key\n", clientNum);
	}
	else if ( cryptMode == SECRET )
	{
		fprintf(fp, "secret client%d-static.key\n", clientNum);
	}
	fprintf(fp, "\n# Custom Configuration\n");
	sprintf(&buffer[0], "vpn_client%d_custom", clientNum);
	fprintf(fp, nvram_safe_get(&buffer[0]));
	fclose(fp);
	vpnlog(VPN_LOG_EXTRA,"Done writing config file");

	// Write certification and key files
	vpnlog(VPN_LOG_EXTRA,"Writing certs/keys");
	if ( cryptMode == TLS )
	{
		sprintf(&buffer[0], "/etc/openvpn/client%d-ca.crt", clientNum);
			fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_ca", clientNum);
			fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/client%d.key", clientNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_key", clientNum);
		fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/client%d.crt", clientNum);
			fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_crt", clientNum);
			fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);
	}
	sprintf(&buffer[0], "vpn_client%d_hmac", clientNum);
	if ( cryptMode == SECRET || (cryptMode == TLS && nvram_get_int(&buffer[0]) >= 0) )
	{
		sprintf(&buffer[0], "/etc/openvpn/client%d-static.key", clientNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_static", clientNum);
		fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);
	}
	vpnlog(VPN_LOG_EXTRA,"Done writing certs/keys");

	// Start the VPN client
	sprintf(&buffer[0], "/etc/openvpn/vpnclient%d --cd /etc/openvpn --config client%d.ovpn", clientNum, clientNum);
	vpnlog(VPN_LOG_INFO,"Starting OpenVPN: %s",&buffer[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Starting OpenVPN failed...");
		stop_vpnclient(clientNum);
		return;
	}
	vpnlog(VPN_LOG_EXTRA,"Done starting openvpn");

	// Handle firewall rules if appropriate
	sprintf(&buffer[0], "vpn_client%d_firewall", clientNum);
	if ( !nvram_contains_word(&buffer[0], "custom") )
	{
		// Create firewall rules
		vpnlog(VPN_LOG_EXTRA,"Creating firewall rules");
		sprintf(&buffer[0], "/etc/openvpn/client%d-fw.sh", clientNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR|S_IXUSR);
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "iptables -A INPUT -i %s -j ACCEPT\n", &iface[0]);
		fprintf(fp, "iptables -A FORWARD -i %s -j ACCEPT\n", &iface[0]);
		if ( routeMode == NAT )
		{
			sscanf(nvram_safe_get("lan_ipaddr"), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
			sscanf(nvram_safe_get("lan_netmask"), "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
			fprintf(fp, "iptables -t nat -A POSTROUTING -s %d.%d.%d.%d/%s -o %s -j MASQUERADE\n",
			        ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3], nvram_safe_get("lan_netmask"), &iface[0]);
		}
		fclose(fp);
		vpnlog(VPN_LOG_EXTRA,"Done creating firewall rules");

		// Run the firewall rules
		vpnlog(VPN_LOG_EXTRA,"Running firewall rules");
		sprintf(&buffer[0], "/etc/openvpn/client%d-fw.sh", clientNum);
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
		vpnlog(VPN_LOG_EXTRA,"Done running firewall rules");
	}

	vpnlog(VPN_LOG_INFO,"VPN GUI server backend complete.");
}

void stop_vpnclient(int clientNum)
{
	DIR *dir;
	struct dirent *file;
	char *fn;
	int argc;
	char *argv[7];
	char buffer[32];

	vpnlog(VPN_LOG_INFO,"Stopping VPN GUI client backend.");

	// Remove firewall rules
	vpnlog(VPN_LOG_EXTRA,"Removing firewall rules.");
	sprintf(&buffer[0], "/etc/openvpn/client%d-fw.sh", clientNum);
	argv[0] = "sed";
	argv[1] = "-i";
	argv[2] = "s/-A/-D/g;s/-I/-D/g";
	argv[3] = &buffer[0];
	argv[4] = NULL;
	if (!_eval(argv, NULL, 0, NULL))
	{
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}
	vpnlog(VPN_LOG_EXTRA,"Done removing firewall rules.");

	// Stop the VPN client
	vpnlog(VPN_LOG_EXTRA,"Stopping OpenVPN client.");
	sprintf(&buffer[0], "vpnclient%d", clientNum);
	killall(&buffer[0], SIGTERM);
	vpnlog(VPN_LOG_EXTRA,"OpenVPN client stopped.");

	// NVRAM setting for device type could have changed, just try to remove both
	vpnlog(VPN_LOG_EXTRA,"Removing VPN device.");
	sprintf(&buffer[0], "openvpn --rmtun --dev tap%d", clientNum+CLIENT_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	sprintf(&buffer[0], "openvpn --rmtun --dev tun%d", clientNum+CLIENT_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);
	vpnlog(VPN_LOG_EXTRA,"VPN device removed.");

	modprobe_r("tun");

	if ( nvram_get_int("vpn_debug") != 1 )
	{
		// Delete all files in /etc/openvpn that match client#
		vpnlog(VPN_LOG_EXTRA,"Removing generated files.");
		sprintf(&buffer[0], "client%d", clientNum);
		dir = opendir("/etc/openvpn");
		chdir("/etc/openvpn");
		while ( (file = readdir(dir)) != NULL )
		{
			fn = file->d_name;
			if ( strstr(fn, &buffer[0]) )
				unlink(fn);
		}
		vpnlog(VPN_LOG_EXTRA,"Done removing generated files.");
	}

	vpnlog(VPN_LOG_INFO,"VPN GUI client backend stopped.");
}

void start_vpnserver(int serverNum)
{
	FILE *fp, *ccd;
	char buffer[BUF_SIZE];
	char iface[IF_SIZE];
	char *argv[6], *chp, *route;
	int argc = 0;
	int c2c = 0;
	enum { TAP, TUN } ifType = TUN;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	int nvi, ip[4], nm[4];

	vpnlog(VPN_LOG_INFO,"VPN GUI server backend starting...");

	sprintf(&buffer[0], "vpnserver%d", serverNum);
	if ( pidof(&buffer[0]) >= 0 )
	{
		vpnlog(VPN_LOG_NOTE, "VPN Server %d already running...", serverNum);
		vpnlog(VPN_LOG_INFO,"PID: %d", pidof(&buffer[0]));
		return;
	}

	// Determine interface type
	sprintf(&buffer[0], "vpn_server%d_if", serverNum);
	if ( nvram_contains_word(&buffer[0], "tap") )
		ifType = TAP;
	else if ( nvram_contains_word(&buffer[0], "tun") )
		ifType = TUN;
	else
	{
		vpnlog(VPN_LOG_ERROR,"Invalid interface type, %.3s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Build interface name
	snprintf(&iface[0], IF_SIZE, "%s%d", nvram_safe_get(&buffer[0]), serverNum+SERVER_IF_START);

	// Determine encryption mode
	sprintf(&buffer[0], "vpn_server%d_crypt", serverNum);
	if ( nvram_contains_word(&buffer[0], "tls") )
		cryptMode = TLS;
	else if ( nvram_contains_word(&buffer[0], "secret") )
		cryptMode = SECRET;
	else if ( nvram_contains_word(&buffer[0], "custom") )
		cryptMode = CUSTOM;
	else
	{
		vpnlog(VPN_LOG_ERROR,"Invalid encryption mode, %.6s", nvram_safe_get(&buffer[0]));
		return;
	}

	// Make sure openvpn directory exists
	mkdir("/etc/openvpn", 0700);

	// Make sure symbolic link exists
	sprintf(&buffer[0], "/etc/openvpn/vpnserver%d", serverNum);
	unlink(&buffer[0]);
	if ( symlink("/usr/sbin/openvpn", &buffer[0]) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating symlink failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Make sure module is loaded
	modprobe("tun");

	// Create tap/tun interface
	sprintf(&buffer[0], "openvpn --mktun --dev %s", &iface[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Creating tunnel interface failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Add interface to LAN bridge (TAP only)
	if( ifType == TAP )
	{
		snprintf(&buffer[0], BUF_SIZE, "brctl addif %s %s", nvram_safe_get("lan_ifname"), &iface[0]);
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if ( _eval(argv, NULL, 0, NULL) )
		{
			vpnlog(VPN_LOG_ERROR,"Adding tunnel interface to bridge failed...");
			stop_vpnserver(serverNum);
			return;
		}
	}

	// Bring interface up
	sprintf(&buffer[0], "ifconfig %s 0.0.0.0 promisc up", &iface[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Bringing up tunnel interface failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Build and write config files
	vpnlog(VPN_LOG_EXTRA,"Writing config file");
	sprintf(&buffer[0], "/etc/openvpn/server%d.ovpn", serverNum);
	fp = fopen(&buffer[0], "w");
	chmod(&buffer[0], S_IRUSR|S_IWUSR);
	fprintf(fp, "# Automatically generated configuration\n");
	fprintf(fp, "daemon\n");
	if ( cryptMode == TLS )
	{
		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_server%d_sn", serverNum);
			fprintf(fp, "server %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_server%d_nm", serverNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
		}
		else if ( ifType == TAP )
		{
			fprintf(fp, "server-bridge");
			sprintf(&buffer[0], "vpn_server%d_dhcp", serverNum);
			if ( nvram_get_int(&buffer[0]) == 0 )
			{
				fprintf(fp, " %s ", nvram_safe_get("lan_ipaddr"));
				fprintf(fp, "%s ", nvram_safe_get("lan_netmask"));
				sprintf(&buffer[0], "vpn_server%d_r1", serverNum);
				fprintf(fp, "%s ", nvram_safe_get(&buffer[0]));
				sprintf(&buffer[0], "vpn_server%d_r2", serverNum);
				fprintf(fp, "%s", nvram_safe_get(&buffer[0]));
			}
			fprintf(fp, "\n");
		}
	}
	else if ( cryptMode == SECRET )
	{
		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_server%d_local", serverNum);
			fprintf(fp, "ifconfig %s ", nvram_safe_get(&buffer[0]));
			sprintf(&buffer[0], "vpn_server%d_remote", serverNum);
			fprintf(fp, "%s\n", nvram_safe_get(&buffer[0]));
		}
	}
	sprintf(&buffer[0], "vpn_server%d_proto", serverNum);
	fprintf(fp, "proto %s\n", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_server%d_port", serverNum);
	fprintf(fp, "port %d\n", nvram_get_int(&buffer[0]));
	fprintf(fp, "dev %s\n", &iface[0]);
	sprintf(&buffer[0], "vpn_server%d_cipher", serverNum);
	if ( !nvram_contains_word(&buffer[0], "default") )
		fprintf(fp, "cipher %s\n", nvram_safe_get(&buffer[0]));
	sprintf(&buffer[0], "vpn_server%d_comp", serverNum);
	fprintf(fp, "comp-lzo %s\n", nvram_safe_get(&buffer[0]));
	fprintf(fp, "keepalive 15 60\n");
	fprintf(fp, "verb 3\n");
	if ( cryptMode == TLS )
	{
		if ( ifType == TUN )
		{
			sscanf(nvram_safe_get("lan_ipaddr"), "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
			sscanf(nvram_safe_get("lan_netmask"), "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
			fprintf(fp, "push \"route %d.%d.%d.%d %s\"\n", ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3],
			        nvram_safe_get("lan_netmask"));
		}

		sprintf(&buffer[0], "vpn_server%d_ccd", serverNum);
		if ( nvram_get_int(&buffer[0]) )
		{
			fprintf(fp, "client-config-dir server%d-ccd\n", serverNum);

			sprintf(&buffer[0], "vpn_server%d_c2c", serverNum);
			if ( (c2c = nvram_get_int(&buffer[0])) )
				fprintf(fp, "client-to-client\n");

			sprintf(&buffer[0], "vpn_server%d_ccd_excl", serverNum);
			if ( nvram_get_int(&buffer[0]) )
				fprintf(fp, "ccd-exclusive\n");

			sprintf(&buffer[0], "/etc/openvpn/server%d-ccd", serverNum);
			mkdir(&buffer[0], 0700);
			chdir(&buffer[0]);

			sprintf(&buffer[0], "vpn_server%d_ccd_val", serverNum);
			strcpy(&buffer[0], nvram_safe_get(&buffer[0]));
			chp = strtok(&buffer[0],">");
			while ( chp != NULL )
			{
				nvi = strlen(chp);

				chp[strcspn(chp,"<")] = '\0';
				vpnlog(VPN_LOG_EXTRA,"CCD: enabled: %d", atoi(chp));
				if ( atoi(chp) == 1 )
				{
					nvi -= strlen(chp)+1;
					chp += strlen(chp)+1;

					ccd = NULL;
					route = NULL;
					if ( nvi > 0 )
					{
						chp[strcspn(chp,"<")] = '\0';
						vpnlog(VPN_LOG_EXTRA,"CCD: Common name: %s", chp);
						ccd = fopen(chp, "w");
						chmod(chp, S_IRUSR|S_IWUSR);

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}
					if ( nvi > 0 && ccd != NULL && strcspn(chp,"<") != strlen(chp) )
					{
						chp[strcspn(chp,"<")] = ' ';
						chp[strcspn(chp,"<")] = '\0';
						route = chp;
						vpnlog(VPN_LOG_EXTRA,"CCD: Route: %s", chp);
						if ( strlen(route) > 1 )
						{
							fprintf(ccd, "iroute %s\n", route);
							fprintf(fp, "route %s\n", route);
						}	

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}
					if ( ccd != NULL )
						fclose(ccd);
					if ( nvi > 0 && route != NULL )
					{
						chp[strcspn(chp,"<")] = '\0';
						vpnlog(VPN_LOG_EXTRA,"CCD: Push: %d", atoi(chp));
						if ( c2c && atoi(chp) == 1 && strlen(route) > 1 )
							fprintf(fp, "push \"route %s\"\n", route);

						nvi -= strlen(chp)+1;
						chp += strlen(chp)+1;
					}

					vpnlog(VPN_LOG_EXTRA,"CCD leftover: %d", nvi+1);
				}
				// Advance to next entry
				chp = strtok(NULL, ">");
			}
			vpnlog(VPN_LOG_EXTRA,"CCD processing complete");
		}

		sprintf(&buffer[0], "vpn_server%d_hmac", serverNum);
		nvi = nvram_get_int(&buffer[0]);
		if ( nvi >= 0 )
		{
			fprintf(fp, "tls-auth server%d-static.key", serverNum);
			if ( nvi < 2 )
				fprintf(fp, " %d", nvi);
			fprintf(fp, "\n");
		}

		fprintf(fp, "ca server%d-ca.crt\n", serverNum);
		fprintf(fp, "dh server%d-dh.pem\n", serverNum);
		fprintf(fp, "cert server%d.crt\n", serverNum);
		fprintf(fp, "key server%d.key\n", serverNum);
	}
	else if ( cryptMode == SECRET )
	{
		fprintf(fp, "secret server%d-static.key\n", serverNum);
	}
	fprintf(fp, "status-version 2\n");
	fprintf(fp, "status server%d.status\n", serverNum);
	fprintf(fp, "\n# Custom Configuration\n");
	sprintf(&buffer[0], "vpn_server%d_custom", serverNum);
	fprintf(fp, nvram_safe_get(&buffer[0]));
	fclose(fp);
	vpnlog(VPN_LOG_EXTRA,"Done writing config file");

	// Write certification and key files
	vpnlog(VPN_LOG_EXTRA,"Writing certs/keys");
	if ( cryptMode == TLS )
	{
		sprintf(&buffer[0], "/etc/openvpn/server%d-ca.crt", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_ca", serverNum);
		fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/server%d.key", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_key", serverNum);
		fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/server%d.crt", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_crt", serverNum);
		fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/server%d-dh.pem", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_dh", serverNum);
		fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);
	}
	sprintf(&buffer[0], "vpn_server%d_hmac", serverNum);
	if ( cryptMode == SECRET || (cryptMode == TLS && nvram_get_int(&buffer[0]) >= 0) )
	{
		sprintf(&buffer[0], "/etc/openvpn/server%d-static.key", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_static", serverNum);
		fprintf(fp, nvram_safe_get(&buffer[0]));
		fclose(fp);
	}
	vpnlog(VPN_LOG_EXTRA,"Done writing certs/keys");

	sprintf(&buffer[0], "/etc/openvpn/vpnserver%d --cd /etc/openvpn --config server%d.ovpn", serverNum, serverNum);
	vpnlog(VPN_LOG_INFO,"Starting OpenVPN: %s",&buffer[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		vpnlog(VPN_LOG_ERROR,"Starting VPN instance failed...");
		stop_vpnserver(serverNum);
		return;
	}
	vpnlog(VPN_LOG_EXTRA,"Done starting openvpn");

	// Handle firewall rules if appropriate
	sprintf(&buffer[0], "vpn_server%d_firewall", serverNum);
	if ( !nvram_contains_word(&buffer[0], "custom") )
	{
		// Create firewall rules
		vpnlog(VPN_LOG_EXTRA,"Creating firewall rules");
		sprintf(&buffer[0], "/etc/openvpn/server%d-fw.sh", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR|S_IXUSR);
		fprintf(fp, "#!/bin/sh\n");
		sprintf(&buffer[0], "vpn_server%d_proto", serverNum);
		strncpy(&buffer[0], nvram_safe_get(&buffer[0]), BUF_SIZE);
		fprintf(fp, "iptables -I INPUT -p %s ", strtok(&buffer[0], "-"));
		sprintf(&buffer[0], "vpn_server%d_port", serverNum);
		fprintf(fp, "--dport %d -j ACCEPT\n", nvram_get_int(&buffer[0]));
		sprintf(&buffer[0], "vpn_server%d_firewall", serverNum);
		if ( !nvram_contains_word(&buffer[0], "external") )
		{
			fprintf(fp, "#!/bin/sh\n");
			fprintf(fp, "iptables -A INPUT -i %s -j ACCEPT\n", &iface[0]);
			fprintf(fp, "iptables -A FORWARD -i %s -j ACCEPT\n", &iface[0]);
		}
		fclose(fp);
		vpnlog(VPN_LOG_EXTRA,"Done creating firewall rules");

		// Run the firewall rules
		vpnlog(VPN_LOG_EXTRA,"Running firewall rules");
		sprintf(&buffer[0], "/etc/openvpn/server%d-fw.sh", serverNum);
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
		vpnlog(VPN_LOG_EXTRA,"Done running firewall rules");
	}

	vpnlog(VPN_LOG_INFO,"VPN GUI server backend complete.");
}

void stop_vpnserver(int serverNum)
{
	DIR *dir;
	struct dirent *file;
	char *fn;
	int argc;
	char *argv[9];
	char buffer[32];

	vpnlog(VPN_LOG_INFO,"Stopping VPN GUI server backend.");

	// Remove firewall rules
	vpnlog(VPN_LOG_EXTRA,"Removing firewall rules.");
	sprintf(&buffer[0], "/etc/openvpn/server%d-fw.sh", serverNum);
	argv[0] = "sed";
	argv[1] = "-i";
	argv[2] = "s/-A/-D/g;s/-I/-D/g";
	argv[3] = &buffer[0];
	argv[4] = NULL;
	if (!_eval(argv, NULL, 0, NULL))
	{
		argv[0] = &buffer[0];
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
	}
	vpnlog(VPN_LOG_EXTRA,"Done removing firewall rules.");

	// Stop the VPN server
	vpnlog(VPN_LOG_EXTRA,"Stopping OpenVPN server.");
	sprintf(&buffer[0], "vpnserver%d", serverNum);
	killall(&buffer[0], SIGTERM);
	vpnlog(VPN_LOG_EXTRA,"OpenVPN server stopped.");

	// NVRAM setting for device type could have changed, just try to remove both
	vpnlog(VPN_LOG_EXTRA,"Removing VPN device.");
	sprintf(&buffer[0], "openvpn --rmtun --dev tap%d", serverNum+SERVER_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	sprintf(&buffer[0], "openvpn --rmtun --dev tun%d", serverNum+SERVER_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);
	vpnlog(VPN_LOG_EXTRA,"VPN device removed.");

	modprobe_r("tun");

	if ( nvram_get_int("vpn_debug") != 1 )
	{
		// Delete all files in /etc/openvpn that match server#
		vpnlog(VPN_LOG_EXTRA,"Removing generated files.");
		sprintf(&buffer[0], "server%d", serverNum);
		dir = opendir("/etc/openvpn");
		chdir("/etc/openvpn");
		while ( (file = readdir(dir)) != NULL )
		{
			fn = file->d_name;
			if ( strstr(fn, &buffer[0]) )
				unlink(fn);
		}
		closedir(dir);
		vpnlog(VPN_LOG_EXTRA,"Done removing generated files.");
	}

	vpnlog(VPN_LOG_INFO,"VPN GUI server backend stopped.");
}

void run_vpn_firewall_scripts()
{
	DIR *dir;
	struct dirent *file;
	char *fn;
	char *match = "-fw.sh";
	char *argv[3];

	if ( chdir("/etc/openvpn") )
		return;

	dir = opendir("/etc/openvpn");

	vpnlog(VPN_LOG_EXTRA,"Beginning all firewall scripts...");
	while ( (file = readdir(dir)) != NULL )
	{
		fn = file->d_name;
		if ( strlen(fn) >= strlen(match) && !strcmp(&fn[strlen(fn)-strlen(match)],match) )
		{
			vpnlog(VPN_LOG_INFO,"Running firewall script: %s", fn);
			argv[0] = "/bin/sh";
			argv[1] = fn;
			argv[2] = NULL;
			_eval(argv, NULL, 0, NULL);
		}
	}
	vpnlog(VPN_LOG_EXTRA,"Done with all firewall scripts...");

	closedir(dir);
}

