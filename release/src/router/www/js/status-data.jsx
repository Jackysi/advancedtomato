/*
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
*/

//<% nvram("router_name,wan_domain,wan_hostname,et0macaddr,lan_proto,lan_ipaddr,dhcp_start,dhcp_num,dhcpd_startip,dhcpd_endip,lan_netmask,wl_security_mode,wl_crypto,wl_mode,wl_wds_enable,wl_hwaddr,wl_net_mode,wl_radio,wl_channel,lan_gateway,wl_ssid,wl_closed,t_model_name,t_features,dhcp1_start,dhcp1_num,dhcpd1_startip,dhcpd1_endip,dhcp2_start,dhcp2_num,dhcpd2_startip,dhcpd2_endip,dhcp3_start,dhcp3_num,dhcpd3_startip,dhcpd3_endip,lan1_proto,lan1_ipaddr,lan1_netmask,lan2_proto,lan2_ipaddr,lan2_netmask,lan3_proto,lan3_ipaddr,lan3_netmask,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan_ifnames,lan1_ifnames,lan2_ifnames,lan3_ifnames,wan_ifnames,tomatoanon_enable,tomatoanon_answer,lan_desc,wan_ppp_get_ip,wan_pptp_dhcp,wan_pptp_server_ip,wan_ipaddr_buf,wan_gateway,wan_gateway_get,wan_get_domain,wan_hwaddr,wan_ipaddr,wan_netmask,wan_proto,wan_run_mtu,wan_sta,wan2_ppp_get_ip,wan2_pptp_dhcp,wan2_pptp_server_ip,wan2_ipaddr_buf,wan2_gateway,wan2_gateway_get,wan2_get_domain,wan2_hwaddr,wan2_ipaddr,wan2_netmask,wan2_proto,wan2_run_mtu,wan2_sta,wan3_ppp_get_ip,wan3_pptp_dhcp,wan3_pptp_server_ip,wan3_ipaddr_buf,wan3_gateway,wan3_gateway_get,wan3_get_domain,wan3_hwaddr,wan3_ipaddr,wan3_netmask,wan3_proto,wan3_run_mtu,wan3_sta,wan4_ppp_get_ip,wan4_pptp_dhcp,wan4_pptp_server_ip,wan4_ipaddr_buf,wan4_gateway,wan4_gateway_get,wan4_get_domain,wan4_hwaddr,wan4_ipaddr,wan4_netmask,wan4_proto,wan4_run_mtu,wan4_sta,mwan_num,pptp_client_enable,pptp_client_ipaddr,pptp_client_netmask,pptp_client_gateway,pptp_client_get_dns,pptp_client_srvsub,pptp_client_srvsubmsk"); %>
//<% uptime(); %>
//<% sysinfo(); %>
//<% wlstats(1); %>

stats = { };

do {
	var a, b, i;
/* MULTIWAN-BEGIN */
	var xifs = ['wan', 'lan', 'lan1', 'lan2', 'lan3', 'wan2', 'wan3', 'wan4'];
/* MULTIWAN-END */

/* DUALWAN-BEGIN */
	var xifs = ['wan', 'lan', 'lan1', 'lan2', 'lan3', 'wan2'];
/* DUALWAN-END */

	stats.anon_enable = nvram.tomatoanon_enable;
	stats.anon_answer = nvram.tomatoanon_answer;

	stats.lan_desc = nvram.lan_desc;

	if (typeof(last_wan_proto) == 'undefined') {
		last_wan_proto = nvram.wan_proto;
	}
	else if (last_wan_proto != nvram.wan_proto) {
		reloadPage();
	}
	stats.flashsize = sysinfo.flashsize+'MB';
	stats.cpumhz = sysinfo.cpuclk+'MHz';
	stats.cputemp = sysinfo.cputemp+'°';
	stats.systemtype = sysinfo.systemtype;
	stats.cpuload = ((sysinfo.loads[0] / 65536.0).toFixed(2) + '<small> / </small> ' +
		(sysinfo.loads[1] / 65536.0).toFixed(2) + '<small> / </small>' +
		(sysinfo.loads[2] / 65536.0).toFixed(2));
	stats.freqcpu = nvram.clkfreq;
	stats.uptime = sysinfo.uptime_s;

	a = sysinfo.totalram;
	b = sysinfo.totalfreeram;
	stats.memory = scaleSize(a - b) + ' <small>/</small> ' + scaleSize(a) + ' (' + ((a - b) / a * 100.0).toFixed(2) + '%)';
	stats.memoryperc = ((a-b) / a * 100.0).toFixed(2) + '%';

	if (sysinfo.totalswap > 0) {
		a = sysinfo.totalswap;
		b = sysinfo.freeswap;
		stats.swap = scaleSize(a - b) + ' <small>/</small> ' + scaleSize(a) + ' (' + ((a - b) / a * 100.0).toFixed(2) + '%)';
		stats.swapperc = ((a - b) / a * 100.0).toFixed(2) + '%';

	} else
		stats.swap = '';

	stats.time = '<% time(); %>';
/* MULTIWAN-BEGIN */
	stats.wanup = [<% wanup("wan"); %>,<% wanup("wan2"); %>,<% wanup("wan3"); %>,<% wanup("wan4"); %>];
	stats.wanuptime = ['<% link_uptime("wan"); %>','<% link_uptime("wan2"); %>','<% link_uptime("wan3"); %>','<% link_uptime("wan4"); %>'];
	stats.wanlease = ['<% dhcpc_time("wan"); %>','<% dhcpc_time("wan2"); %>','<% dhcpc_time("wan3"); %>','<% dhcpc_time("wan4"); %>'];
	stats.dns = [<% dns("wan"); %>,<% dns("wan2"); %>,<% dns("wan3"); %>,<% dns("wan4"); %>];
/* MULTIWAN-END */

/* DUALWAN-BEGIN */
	stats.wanup = [<% wanup("wan"); %>,<% wanup("wan2"); %>];
	stats.wanuptime = ['<% link_uptime("wan"); %>','<% link_uptime("wan2"); %>'];
	stats.wanlease = ['<% dhcpc_time("wan"); %>','<% dhcpc_time("wan2"); %>'];
	stats.dns = [<% dns("wan"); %>,<% dns("wan2"); %>];
/* DUALWAN-END */

	stats.wanip = [];
	stats.wannetmask = [];
	stats.wangateway = [];

	for(var uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		var u = (uidx>1) ? uidx : '';
		stats.wanip[uidx-1] = nvram['wan'+u+'_ipaddr'];
		stats.wannetmask[uidx-1] = nvram['wan'+u+'_netmask'];
		stats.wangateway[uidx-1] = nvram['wan'+u+'_gateway_get'];
		if (stats.wangateway[uidx-1] == '0.0.0.0' || stats.wangateway[uidx-1] == '')
		stats.wangateway[uidx-1] = nvram['wan'+u+'_gateway'];

		switch (nvram['wan'+u+'_proto']) {
		case 'pptp':
		case 'l2tp':
		case 'pppoe':
			if (stats.wanup[uidx-1]) {
				stats.wanip[uidx-1] = nvram['wan'+u+'_ppp_get_ip'];
				if (nvram['wan'+u+'_pptp_dhcp'] == '1') {
					if (nvram['wan'+u+'_ipaddr'] != '' && nvram['wan'+u+'_ipaddr'] != '0.0.0.0' && nvram['wan'+u+'_ipaddr'] != stats.wanip[uidx-1])
						stats.wanip[uidx-1] += '&nbsp;&nbsp;<small>(DHCP: ' + nvram['wan'+u+'_ipaddr'] + ')</small>';
					if (nvram['wan'+u+'_gateway'] != '' && nvram['wan'+u+'_gateway']  != '0.0.0.0' && nvram['wan'+u+'_gateway']  != stats.wangateway[uidx-1])
						stats.wangateway[uidx-1] += '&nbsp;&nbsp;<small>(DHCP: ' + nvram['wan'+u+'_gateway']  + ')</small>';
				}
				if (stats.wannetmask[uidx-1] == '0.0.0.0')
					stats.wannetmask[uidx-1] = '255.255.255.255';
			} else {
				if (nvram['wan'+u+'_proto'] == 'pptp')
					stats.wangateway[uidx-1] = nvram['wan'+u+'_pptp_server_ip'];
			}
			break;
		default:
			if (!stats.wanup[uidx-1]) {
				stats.wanip[uidx-1] = '0.0.0.0';
				stats.wannetmask[uidx-1] = '0.0.0.0';
				stats.wangateway[uidx-1] = '0.0.0.0';
			}
		}
	}

/* IPV6-BEGIN */
	stats.ip6_wan = ((typeof(sysinfo.ip6_wan) != 'undefined') ? sysinfo.ip6_wan : '') + '';
	stats.ip6_lan = ((typeof(sysinfo.ip6_lan) != 'undefined') ? sysinfo.ip6_lan : '') + '';
	stats.ip6_lan_ll = ((typeof(sysinfo.ip6_lan_ll) != 'undefined') ? sysinfo.ip6_lan_ll : '') + '';
/* IPV6-END */

/* MULTIWAN-BEGIN */
	stats.wanstatus = ['<% wanstatus("wan"); %>','<% wanstatus("wan2"); %>','<% wanstatus("wan3"); %>','<% wanstatus("wan4"); %>'];
/* MULTIWAN-END */
/* DUALWAN-BEGIN */
	stats.wanstatus = ['<% wanstatus("wan"); %>','<% wanstatus("wan2"); %>'];
/* DUALWAN-END */

	for(var uidx = 1; uidx <= nvram.mwan_num; ++uidx) {
		if (stats.wanstatus[uidx-1] != 'Connected') stats.wanstatus[uidx-1] = '<b>' + stats.wanstatus[uidx-1] + '</b>';
	}

	stats.channel = [];
	stats.interference = [];
	stats.qual = [];

	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);

		a = i = wlstats[uidx].channel * 1;
		if (i < 0) i = -i;
		stats.channel.push('<a href="#tools-survey.asp">' + ((i) ? i + '' : 'Auto') +
			((wlstats[uidx].mhz) ? ' - ' + (wlstats[uidx].mhz / 1000.0).toFixed(3) + ' <small>GHz</small>' : '') + '</a>' +
			((a < 0) ? ' <small>(scanning...)</small>' : ''));
		stats.interference.push((wlstats[uidx].intf >= 0) ? ((wlstats[uidx].intf) ? 'Severe' : 'Acceptable') : '');

		a = wlstats[uidx].nbw * 1;
		wlstats[uidx].nbw = (a > 0) ? (a + ' <small>MHz</small>') : 'Auto';

		if (wlstats[uidx].radio) {
			a = wlstats[uidx].rate * 1;
			if (a > 0)
				wlstats[uidx].rate = Math.floor(a / 2) + ((a & 1) ? '.5' : '') + ' <small>Mbps</small>';
			else
				wlstats[uidx].rate = '-';

			if (wlstats[uidx].client) {
				if (wlstats[uidx].rssi == 0) a = 0;
				else a = MAX(wlstats[uidx].rssi - wlstats[uidx].noise, 0);
				stats.qual.push(a + ' <i class="icon-wifi-' + MIN(MAX(Math.floor(a / 10), 1), 3) + '"></i>');
			}
			else {
				stats.qual.push('');
			}
			wlstats[uidx].noise += ' <small>dBm</small>';
			wlstats[uidx].rssi += ' <small>dBm</small>';
		}
		else {
			wlstats[uidx].rate = '';
			wlstats[uidx].noise = '';
			wlstats[uidx].rssi = '';
			stats.qual.push('');
		}

		if (wl_ifaces[uidx][6] != 1) {
			wlstats[uidx].ifstatus = '<b>Down</b>';
		} else {
			wlstats[uidx].ifstatus = 'Up';
			for (i = 0; i < xifs.length ; ++i) {
				if ((nvram[xifs[i] + '_ifnames']).indexOf(wl_ifaces[uidx][0]) >= 0) {
					wlstats[uidx].ifstatus = wlstats[uidx].ifstatus + ' (' + xifs[i].toUpperCase() + ')';
					break;
				}
			}
		}
	}
	
} while (0);
