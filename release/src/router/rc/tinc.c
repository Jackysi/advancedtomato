/*

	Copyright (C) 2014 Lance Fredrickson
	lancethepants@gmail.com

*/

#include "rc.h"

#define BUF_SIZE 256

void start_tinc(void)
{

	char *nv, *nvp, *b;
	const char *connecto, *name, *address, *port, *compression, *subnet, *rsa, *ed25519, *custom, *tinc_tmp_value;
	char buffer[BUF_SIZE];
	FILE *fp, *hp;


	// create tinc directories
	mkdir("/etc/tinc", 0700);
	mkdir("/etc/tinc/hosts", 0700);


	// write private rsa key
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_private_rsa"), "") != 0 ){
		if ( !( fp = fopen( "/etc/tinc/rsa_key.priv", "w" ))){
			perror( "/etc/tinc/rsa_key.priv" );
			return;
		}
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod("/etc/tinc/rsa_key.priv", 0600);
	}


	// write private ed25519 key
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_private_ed25519"), "") != 0 ){
		if ( !( fp = fopen( "/etc/tinc/ed25519_key.priv", "w" ))){
			perror( "/etc/tinc/ed25519_key.priv" );
			return;
		}
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod("/etc/tinc/ed25519_key.priv", 0600);
	}


	// create tinc.conf
	if ( !( fp = fopen( "/etc/tinc/tinc.conf", "w" ))){
		perror( "/etc/tinc/tinc.conf" );
		return;
	}


	fprintf(fp, "Name = %s\n", nvram_safe_get( "tinc_name" ));

	fprintf(fp, "Interface = tinc\n" );

	fprintf(fp, "DeviceType = %s\n", nvram_safe_get( "tinc_devicetype" ));


	if (nvram_match("tinc_devicetype", "tun")){
		fprintf(fp, "Mode = router\n");
	}
	else if (nvram_match("tinc_devicetype", "tap")){
		fprintf(fp, "Mode = %s\n", nvram_safe_get( "tinc_mode" ));
	}


	// create tinc host files
	nvp = nv = strdup(nvram_safe_get("tinc_hosts"));
	if (!nv) return;
	while ((b = strsep(&nvp, ">")) != NULL) {

		if (vstrsep(b, "<", &connecto, &name, &address, &port, &compression, &subnet, &rsa, &ed25519, &custom) != 9) continue;

		sprintf(&buffer[0], "/etc/tinc/hosts/%s", name);
		if ( !( hp = fopen( &buffer[0], "w" ))){
			perror( &buffer[0] );
			return;
		}

		// write Connecto's to tinc.conf, excluding the host system if connecto is enabled
		if ( (strcmp( connecto, "1") == 0 ) && (strcmp( nvram_safe_get("tinc_name"), name) != 0 ) ){
			fprintf(fp, "ConnectTo = %s\n", name );
		}

		if ( strcmp( rsa, "" ) != 0 )
			fprintf(hp, "%s\n", rsa );

		if ( strcmp( ed25519, "" ) != 0 )
			fprintf(hp, "%s\n", ed25519 );

		if ( strcmp( address, "" ) != 0 )
			fprintf(hp, "Address = %s\n", address );

		if ( strcmp( subnet, "" ) != 0 )
			fprintf(hp, "Subnet = %s\n", subnet );

		if ( strcmp( compression, "" ) != 0 )
			fprintf(hp, "Compression = %s\n", compression );

		if ( strcmp( port, "") != 0 )
			fprintf(hp, "Port = %s\n", port );

		if ( strcmp( custom, "") != 0 )
			fprintf(hp, "%s\n", custom );

		fclose(hp);

		// generate tinc-up and firewall scripts
		if ( strcmp( nvram_safe_get("tinc_name"), name)  == 0 ){

			// create tinc-up script if this is the host system.

			if ( !( hp = fopen( "/etc/tinc/tinc-up", "w" ))){
				perror( "/etc/tinc/tinc-up" );
				return;
			}

			fprintf(hp, "#!/bin/sh\n" );

			// Determine whether automatically generate tinc-up, or use manually supplied script.
			if ( !nvram_match("tinc_manual_tinc_up", "1") ){

				if (nvram_match("tinc_devicetype", "tun")){
					fprintf(hp, "ifconfig $INTERFACE %s netmask %s\n", nvram_safe_get("lan_ipaddr"), nvram_safe_get("tinc_vpn_netmask") );
				}
				else if (nvram_match("tinc_devicetype", "tap")){
					fprintf(hp, "brctl addif %s $INTERFACE\n", nvram_safe_get("lan_ifname") );
					fprintf(hp, "ifconfig $INTERFACE 0.0.0.0 promisc up\n" );
				}
			}
			else {
				fprintf(hp, "%s\n", nvram_safe_get("tinc_tinc_up") );
			}

			fclose(hp);
			chmod("/etc/tinc/tinc-up", 0744);

			// Create firewall script.
			if ( !( hp = fopen( "/etc/tinc/tinc-fw.sh", "w" ))){
				perror( "/etc/tinc/tinc-fw.sh" );
				return;
			}

			fprintf(hp, "#!/bin/sh\n" );

			if ( !nvram_match("tinc_manual_firewall", "2") ){

				if ( strcmp( port, "") == 0 )
					port = "655";

				fprintf(hp, "iptables -I INPUT -p udp --dport %s -j ACCEPT\n", port );
				fprintf(hp, "iptables -I INPUT -p tcp --dport %s -j ACCEPT\n", port );


				fprintf(hp, "iptables -I INPUT -i tinc -j ACCEPT\n" );
				fprintf(hp, "iptables -I FORWARD -i tinc -j ACCEPT\n" );

#ifdef TCONFIG_IPV6
				if (ipv6_enabled()){

					fprintf(hp, "\n" );
					fprintf(hp, "ip6tables -I INPUT -p udp --dport %s -j ACCEPT\n", port );
					fprintf(hp, "ip6tables -I INPUT -p tcp --dport %s -j ACCEPT\n", port );

					fprintf(hp, "ip6tables -I INPUT -i tinc -j ACCEPT\n" );
					fprintf(hp, "ip6tables -I FORWARD -i tinc -j ACCEPT\n" );
				}
#endif
			}

			if ( !nvram_match("tinc_manual_firewall", "0") ){

				fprintf(hp, "\n" );
				fprintf(hp, "%s\n", nvram_safe_get("tinc_firewall") );

			}

			fclose(hp);
			chmod("/etc/tinc/tinc-fw.sh", 0744);
		}
	}

	// Write tinc.conf custom configuration
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_custom"), "") != 0 )
		fprintf(fp, "%s\n", tinc_tmp_value );

	fclose(fp);
	free(nv);

	// write tinc-down
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_tinc_down"), "") != 0 ){
		if ( !( fp = fopen( "/etc/tinc/tinc-down", "w" ))){
			perror( "/etc/tinc/tinc-down" );
			return;
		}
		fprintf(fp, "#!/bin/sh\n" );
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod("/etc/tinc/tinc-down", 0744);
        }

	// write host-up
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_host_up"), "") != 0 ){
		if ( !( fp = fopen( "/etc/tinc/host-up", "w" ))){
			perror( "/etc/tinc/host-up" );
			return;
		}
		fprintf(fp, "#!/bin/sh\n" );
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod("/etc/tinc/host-up", 0744);
	}

	// write host-down
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_host_down"), "") != 0 ){
		if ( !( fp = fopen( "/etc/tinc/host-down", "w" ))){
			perror( "/etc/tinc/host-down" );
			return;
		}
		fprintf(fp, "#!/bin/sh\n" );
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod("/etc/tinc/host-down", 0744);
	}

	// write subnet-up
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_subnet_up"), "") != 0 ){
		if ( !( fp = fopen( "/etc/tinc/subnet-up", "w" ))){
			perror( "/etc/tinc/subnet-up" );
			return;
		}
		fprintf(fp, "#!/bin/sh\n" );
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod("/etc/tinc/subnet-up", 0744);
	}

	// write subnet-down
	if ( strcmp( tinc_tmp_value = nvram_safe_get("tinc_subnet_down"), "") != 0 ){
		if ( !( fp = fopen( "/etc/tinc/subnet-down", "w" ))){
			perror( "/etc/tinc/subnet-down" );
			return;
		}
		fprintf(fp, "#!/bin/sh\n" );
		fprintf(fp, "%s\n", tinc_tmp_value );
		fclose(fp);
		chmod("/etc/tinc/subnet-down", 0744);
	}


	// Make sure module is loaded
	modprobe("tun");
	f_wait_exists("/dev/net/tun", 5);

	run_tinc_firewall_script();
	xstart( "/usr/sbin/tinc", "start" );
	return;
}

void stop_tinc(void)
{
	killall("tincd", SIGTERM);
	system( "/bin/sed -i \'s/-A/-D/g;s/-I/-D/g\' /etc/tinc/tinc-fw.sh\n");
	run_tinc_firewall_script();
	system( "/bin/rm -rf /etc/tinc\n" );
	return;
}

void run_tinc_firewall_script(void){

	FILE *fp;

	if ((fp = fopen( "/etc/tinc/tinc-fw.sh", "r" ))){

		fclose(fp);
		system( "/etc/tinc/tinc-fw.sh" );
	}

	return;
}

void start_tinc_wanup(void){

	if ( nvram_match("tinc_wanup", "1") )
		start_tinc();

	return;
}
