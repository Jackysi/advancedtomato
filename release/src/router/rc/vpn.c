/*

	Copyright (C) 2008 Keith Moyer, tomato@keithmoyer.com

	No part of this file may be used without permission.

*/

#include "rc.h"
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#define CLIENT_IF_START 10
#define SERVER_IF_START 20

#define DEBUG(x) if(nvram_get_int("vpn_debug")!=0) syslog(LOG_INFO, "VPN DEBUG: %d: %s\n", __LINE__, x)

void start_vpnclient(int clientNum)
{
	FILE *fp;
	char buffer[64];
	char iface[6];
	char *nv;
	char *argv[5];
	int argc = 0;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	enum { TAP, TUN } ifType = TUN;
	enum { BRIDGE, NAT, NONE } routeMode = NONE;
	int nvi, ip[4], nm[4];

	// Determine interface and encryption mode
	sprintf(&buffer[0], "vpn_client%d_if", clientNum);
	sprintf(&iface[0], "%s%d", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "tap", clientNum+CLIENT_IF_START);
	if ( strncmp(&iface[0], "tap", 3) == 0)
		ifType = TAP;
	sprintf(&buffer[0], "vpn_client%d_crypt", clientNum);
	if ( strcmp(((nv = nvram_get(&buffer[0])) != NULL)? nv: "", "tls") == 0 )
		cryptMode = TLS;
	else if ( strcmp(((nv = nvram_get(&buffer[0])) != NULL)? nv: "", "secret") == 0 )
		cryptMode = SECRET;
	sprintf(&buffer[0], "vpn_client%d_bridge", clientNum);
	if ( ifType == TAP && nvram_get_int(&buffer[0]) == 1 )
		routeMode = BRIDGE;
	sprintf(&buffer[0], "vpn_client%d_nat", clientNum);
	if ( (ifType == TUN || routeMode != BRIDGE) && nvram_get_int(&buffer[0]) == 1 )
		routeMode = NAT;

	// Make sure openvpn directory exists
	mkdir("/etc/openvpn", 0700);

	// Make sure symbolic link exists
	sprintf(&buffer[0], "ln -sf /usr/sbin/openvpn /etc/openvpn/vpnclient%d", clientNum);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		DEBUG("Creating symlink failed...");
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
		DEBUG("Creating tunnel interface failed...");
		stop_vpnclient(clientNum);
		return;
	}

	// Bring interface up (TAP only)
	if( ifType == TAP )
	{
		if ( routeMode == BRIDGE )
		{
			sprintf(&buffer[0], "brctl addif %s %s", ((nv = nvram_get("lan_ifname")) != NULL)? nv: "br0", &iface[0]);
			for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
			if ( _eval(argv, NULL, 0, NULL) )
			{
				DEBUG("Adding tunnel interface to bridge failed...");
				stop_vpnclient(clientNum);
				return;
			}
		}

		sprintf(&buffer[0], "ifconfig %s promisc up", ((nv = nvram_get("lan_ifname")) != NULL)? nv: "br0");
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if ( _eval(argv, NULL, 0, NULL) )
		{
			DEBUG("Bringing interface up failed...");
			stop_vpnclient(clientNum);
			return;
		}
	}

	// Build and write config file
	sprintf(&buffer[0], "/etc/openvpn/client%d.ovpn", clientNum);
	fp = fopen(&buffer[0], "w");
	chmod(&buffer[0], S_IRUSR|S_IWUSR);
	fprintf(fp, "# Automatically generated configuration\n");
	fprintf(fp, "daemon\n");
	if ( cryptMode == TLS )
		fprintf(fp, "client\n");
	fprintf(fp, "dev %s\n", &iface[0]);
	sprintf(&buffer[0], "vpn_client%d_proto", clientNum);
	fprintf(fp, "proto %s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	sprintf(&buffer[0], "vpn_client%d_addr", clientNum);
	fprintf(fp, "remote %s ", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	sprintf(&buffer[0], "vpn_client%d_port", clientNum);
	fprintf(fp, "%d\n", nvram_get_int(&buffer[0]));
	if ( cryptMode == SECRET )
	{
		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_client%d_local", clientNum);
			fprintf(fp, "ifconfig %s ", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
			sprintf(&buffer[0], "vpn_client%d_remote", clientNum);
			fprintf(fp, "%s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		}
		else if ( ifType == TAP )
		{
			sprintf(&buffer[0], "vpn_client%d_local", clientNum);
			fprintf(fp, "ifconfig %s ", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
			sprintf(&buffer[0], "vpn_client%d_nm", clientNum);
			fprintf(fp, "%s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
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
	fprintf(fp, "comp-lzo %s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	sprintf(&buffer[0], "vpn_client%d_cipher", clientNum);
	if ( strcmp(((nv = nvram_get(&buffer[0])) != NULL)? nv: "", "default") != 0 )
		fprintf(fp, "cipher %s\n", nv);
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
	fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	fclose(fp);

	// Write certification and key files
	if ( cryptMode == TLS )
	{
		sprintf(&buffer[0], "/etc/openvpn/client%d-ca.crt", clientNum);
			fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_ca", clientNum);
			fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/client%d.key", clientNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_key", clientNum);
		fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/client%d.crt", clientNum);
			fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_crt", clientNum);
			fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);
	}
	sprintf(&buffer[0], "vpn_client%d_hmac", clientNum);
	if ( cryptMode == SECRET || (cryptMode == TLS && nvram_get_int(&buffer[0]) >= 0) )
	{
		sprintf(&buffer[0], "/etc/openvpn/client%d-static.key", clientNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_client%d_static", clientNum);
		fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);
	}

	// Start the VPN client
	sprintf(&buffer[0], "/etc/openvpn/vpnclient%d --cd /etc/openvpn --config client%d.ovpn", clientNum, clientNum);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		DEBUG("Starting OpenVPN failed...");
		stop_vpnclient(clientNum);
		return;
	}

	// Create firewall rules
	sprintf(&buffer[0], "/etc/openvpn/client%d-fw.sh", clientNum);
	fp = fopen(&buffer[0], "w");
	chmod(&buffer[0], S_IRUSR|S_IWUSR|S_IXUSR);
	fprintf(fp, "#!/bin/sh\n");
	fprintf(fp, "iptables -A INPUT -i %s -j ACCEPT\n", &iface[0]);
	fprintf(fp, "iptables -A FORWARD -i %s -j ACCEPT\n", &iface[0]);
	if ( routeMode == NAT )
	{
		sscanf(((nv = nvram_get("lan_ipaddr")) != NULL)? nv: "", "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
		sscanf(((nv = nvram_get("lan_netmask")) != NULL)? nv: "", "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
		fprintf(fp, "iptables -t nat -A POSTROUTING -s %d.%d.%d.%d/%s -o %s -j MASQUERADE\n",
		        ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3],
		        ((nv = nvram_get("lan_netmask")) != NULL)? nv: "", &iface[0]);
	}
	fclose(fp);

	// Run the firewall rules
	sprintf(&buffer[0], "/etc/openvpn/client%d-fw.sh", clientNum);
	argv[0] = &buffer[0];
	argv[1] = NULL;
	if ( _eval(argv, NULL, 0, NULL) )
	{
		DEBUG("Adding firewall rules failed...");
		stop_vpnclient(clientNum);
		return;
	}
}

void stop_vpnclient(int clientNum)
{
	int argc;
	char *argv[7];
	char buffer[200];

	// Remove firewall rules
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

	// Stop the VPN client
	sprintf(&buffer[0], "vpnclient%d", clientNum);
	killall(&buffer[0], SIGTERM);

	// NVRAM setting for device type could have changed, just try to remove both
	sprintf(&buffer[0], "openvpn --rmtun --dev tap%d", clientNum+CLIENT_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	sprintf(&buffer[0], "openvpn --rmtun --dev tun%d", clientNum+CLIENT_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	modprobe_r("tun");

	if ( nvram_get_int("vpn_debug") != 1 )
	{
		sprintf(&buffer[0], "/etc/openvpn/vpnclient%d /etc/openvpn/client%d.ovpn /etc/openvpn/client%d.key "
		                    "/etc/openvpn/client%d-static.key /etc/openvpn/client%d.crt /etc/openvpn/client%d-ca.crt "
		                    "/etc/openvpn/client%d-fw.sh",
		                    clientNum, clientNum, clientNum, clientNum, clientNum, clientNum, clientNum);
		argv[0] = "rm";
		argv[2] = NULL;
		for(argv[1] = strtok(&buffer[0], " "); argv[1] != NULL; argv[1] = strtok(NULL, " "))
			_eval(argv, NULL, 0, NULL);
	}
}

void start_vpnserver(int serverNum)
{
	FILE *fp;
	char buffer[64];
	char iface[6];
	char *nv;
	char *argv[5];
	int argc = 0;
	enum { TAP, TUN } ifType = TUN;
	enum { TLS, SECRET, CUSTOM } cryptMode = CUSTOM;
	int nvi, ip[4], nm[4];

	// Determine interface type
	sprintf(&buffer[0], "vpn_server%d_if", serverNum);
	sprintf(&iface[0], "%s%d", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "tap", serverNum+SERVER_IF_START);
	if ( strncmp(&iface[0], "tap", 3) == 0 )
		ifType = TAP;

	// Determine encryption mode
	sprintf(&buffer[0], "vpn_server%d_crypt", serverNum);
	if ( strcmp(((nv = nvram_get(&buffer[0])) != NULL)? nv: "", "tls") == 0 )
		cryptMode = TLS;
	if ( strcmp(((nv = nvram_get(&buffer[0])) != NULL)? nv: "", "secret") == 0 )
		cryptMode = SECRET;	

	// Make sure openvpn directory exists
	mkdir("/etc/openvpn", 0700);

	// Make sure symbolic link exists
	sprintf(&buffer[0], "ln -sf /usr/sbin/openvpn /etc/openvpn/vpnserver%d", serverNum);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		DEBUG("Creating symlink failed...");
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
		DEBUG("Creating tunnel interface failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Add interface to LAN bridge (TAP only)
	if( ifType == TAP )
	{
		sprintf(&buffer[0], "brctl addif %s %s", ((nv = nvram_get("lan_ifname")) != NULL)? nv: "br0", &iface[0]);
		for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
		if ( _eval(argv, NULL, 0, NULL) )
		{
			DEBUG("Adding tunnel interface to bridge failed...");
			stop_vpnserver(serverNum);
			return;
		}
	}

	// Bring interface up
	sprintf(&buffer[0], "ifconfig %s 0.0.0.0 promisc up", &iface[0]);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		DEBUG("Bringing up tunnel interface failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Build and write config file
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
			fprintf(fp, "server %s ", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
			sprintf(&buffer[0], "vpn_server%d_nm", serverNum);
			fprintf(fp, "%s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		}
		else if ( ifType == TAP )
		{
			fprintf(fp, "server-bridge %s ", ((nv = nvram_get("lan_ipaddr")) != NULL)? nv: "");
			fprintf(fp, "%s ", ((nv = nvram_get("lan_netmask")) != NULL)? nv: "");
			sprintf(&buffer[0], "vpn_server%d_r1", serverNum);
			fprintf(fp, "%s ", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
			sprintf(&buffer[0], "vpn_server%d_r2", serverNum);
			fprintf(fp, "%s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		}
	}
	else if ( cryptMode == SECRET )
	{
		if ( ifType == TUN )
		{
			sprintf(&buffer[0], "vpn_server%d_local", serverNum);
			fprintf(fp, "ifconfig %s ", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
			sprintf(&buffer[0], "vpn_server%d_remote", serverNum);
			fprintf(fp, "%s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		}
	}
	sprintf(&buffer[0], "vpn_server%d_proto", serverNum);
	fprintf(fp, "proto %s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	sprintf(&buffer[0], "vpn_server%d_port", serverNum);
	fprintf(fp, "port %d\n", nvram_get_int(&buffer[0]));
	fprintf(fp, "dev %s\n", &iface[0]);
	sprintf(&buffer[0], "vpn_server%d_cipher", serverNum);
	if ( strcmp(((nv = nvram_get(&buffer[0])) != NULL)? nv: "", "default") != 0 )
		fprintf(fp, "cipher %s\n", nv);
	sprintf(&buffer[0], "vpn_server%d_comp", serverNum);
	fprintf(fp, "comp-lzo %s\n", ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	fprintf(fp, "keepalive 15 60\n");
	fprintf(fp, "verb 3\n");
	if ( ifType == TUN )
	{
		sscanf(((nv = nvram_get("lan_ipaddr")) != NULL)? nv: "", "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
		sscanf(((nv = nvram_get("lan_netmask")) != NULL)? nv: "", "%d.%d.%d.%d", &nm[0], &nm[1], &nm[2], &nm[3]);
		fprintf(fp, "push \"route %d.%d.%d.%d %s\"\n", ip[0]&nm[0], ip[1]&nm[1], ip[2]&nm[2], ip[3]&nm[3],
		        ((nv = nvram_get("lan_netmask")) != NULL)? nv: "");
	}
	if ( cryptMode == TLS )
	{
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
	fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	fclose(fp);

	// Write certification and key files
	if ( cryptMode == TLS )
	{
		sprintf(&buffer[0], "/etc/openvpn/server%d-ca.crt", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_ca", serverNum);
		fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/server%d.key", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_key", serverNum);
		fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/server%d.crt", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_crt", serverNum);
		fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);

		sprintf(&buffer[0], "/etc/openvpn/server%d-dh.pem", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_dh", serverNum);
		fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);
	}
	sprintf(&buffer[0], "vpn_server%d_hmac", serverNum);
	if ( cryptMode == SECRET || (cryptMode == TLS && nvram_get_int(&buffer[0]) >= 0) )
	{
		sprintf(&buffer[0], "/etc/openvpn/server%d-static.key", serverNum);
		fp = fopen(&buffer[0], "w");
		chmod(&buffer[0], S_IRUSR|S_IWUSR);
		sprintf(&buffer[0], "vpn_server%d_static", serverNum);
		fprintf(fp, ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
		fclose(fp);
	}

	sprintf(&buffer[0], "/etc/openvpn/vpnserver%d --cd /etc/openvpn --config server%d.ovpn", serverNum, serverNum);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	if ( _eval(argv, NULL, 0, NULL) )
	{
		DEBUG("Starting VPN instance failed...");
		stop_vpnserver(serverNum);
		return;
	}

	// Create firewall rules
	sprintf(&buffer[0], "/etc/openvpn/server%d-fw.sh", serverNum);
	fp = fopen(&buffer[0], "w");
	chmod(&buffer[0], S_IRUSR|S_IWUSR|S_IXUSR);
	fprintf(fp, "#!/bin/sh\n");
	sprintf(&buffer[0], "vpn_server%d_proto", serverNum);
	strcpy(&buffer[0], ((nv = nvram_get(&buffer[0])) != NULL)? nv: "");
	fprintf(fp, "iptables -I INPUT -p %s ", strtok(&buffer[0], "-"));
	sprintf(&buffer[0], "vpn_server%d_port", serverNum);
	fprintf(fp, "--dport %d -j ACCEPT\n", nvram_get_int(&buffer[0]));
	fprintf(fp, "iptables -A INPUT -i %s -j ACCEPT\n", &iface[0]);
	fprintf(fp, "iptables -A FORWARD -i %s -j ACCEPT\n", &iface[0]);
	fclose(fp);

	// Run the firewall rules
	sprintf(&buffer[0], "/etc/openvpn/server%d-fw.sh", serverNum);
	argv[0] = &buffer[0];
	argv[1] = NULL;
	if ( _eval(argv, NULL, 0, NULL) )
	{
		DEBUG("Adding firewall rules failed...");
		stop_vpnserver(serverNum);
		return;
	}
}

void stop_vpnserver(int serverNum)
{
	int argc;
	char *argv[9];
	char buffer[256];

	// Remove firewall rules
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

	// Stop the VPN server
	sprintf(&buffer[0], "vpnserver%d", serverNum);
	killall(&buffer[0], SIGTERM);

	// NVRAM setting for device type could have changed, just try to remove both
	sprintf(&buffer[0], "openvpn --rmtun --dev tap%d", serverNum+SERVER_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);

	sprintf(&buffer[0], "openvpn --rmtun --dev tun%d", serverNum+SERVER_IF_START);
	for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
	_eval(argv, NULL, 0, NULL);	

	modprobe_r("tun");

	if ( nvram_get_int("vpn_debug") != 1 )
	{
		sprintf(&buffer[0], "/etc/openvpn/vpnserver%d /etc/openvpn/server%d.ovpn /etc/openvpn/server%d.key "
		                    "/etc/openvpn/server%d.crt /etc/openvpn/server%d-ca.crt /etc/openvpn/server%d-dh.pem "
		                    "/etc/openvpn/server%d-static.key /etc/openvpn/server%d.status /etc/openvpn/server%d-fw.sh",
		                    serverNum, serverNum, serverNum, serverNum, serverNum, serverNum, serverNum, serverNum, serverNum);
		argv[0] = "rm";
		argv[2] = NULL;
		for(argv[1] = strtok(&buffer[0], " "); argv[1] != NULL; argv[1] = strtok(NULL, " "))
			_eval(argv, NULL, 0, NULL);
	}
}

void run_vpn_firewall_scripts()
{
	DIR *dir;
	struct dirent *file;
	char *fn;
	char *match = "-fw.sh";
	char *argv[2];

	dir = opendir("/etc/openvpn");

	while ( (file = readdir(dir)) != NULL )
	{
		fn = file->d_name;
		if ( strlen(fn) >= strlen(match) && !strcmp(&fn[strlen(fn)-strlen(match)],match) )
		{
			DEBUG("Running firewall script:");
			DEBUG(fn);
			argv[0] = fn;
			argv[1] = NULL;
			_eval(argv, NULL, 0, NULL);
		}
	}

	closedir(dir);
}
