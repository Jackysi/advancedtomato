/*
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
*/

//<% nvram("ppp_get_ip,pptp_server_ip,router_name,wan_domain,wan_gateway,wan_gateway_get,wan_get_domain,wan_hostname,wan_hwaddr,wan_ipaddr,wan_netmask,wan_proto,wan_run_mtu,et0macaddr,lan_proto,lan_ipaddr,dhcp_start,dhcp_num,dhcpd_startip,dhcpd_endip,lan_netmask,wl_security_mode,wl_crypto,wl_mode,wl_wds_enable,wl_hwaddr,wl_net_mode,wl_radio,wl_channel,lan_gateway,wl_ssid,t_model_name,t_features,pptp_dhcp"); %>
//<% uptime(); %>
//<% sysinfo(); %>
//<% wlstats(); %>

stats = { };

do {
	var a, b, i;

	if (typeof(last_wan_proto) == 'undefined') {
		last_wan_proto = nvram.wan_proto;
	}
	else if (last_wan_proto != nvram.wan_proto) {
		reloadPage();
	}

	stats.cpuload = ((sysinfo.loads[0] / 65536.0).toFixed(2) + '<small> / </small> ' +
		(sysinfo.loads[1] / 65536.0).toFixed(2) + '<small> / </small>' +
		(sysinfo.loads[2] / 65536.0).toFixed(2));
	stats.uptime = sysinfo.uptime_s;

	a = sysinfo.totalram;
	b = sysinfo.totalfreeram;
	stats.memory = scaleSize(a) + ' / ' + scaleSize(b) + ' <small>(' + (b / a * 100.0).toFixed(2) + '%)</small>';
	if (sysinfo.totalswap > 0) {
		a = sysinfo.totalswap;
		b = sysinfo.freeswap;
		stats.swap = scaleSize(a) + ' / ' + scaleSize(b) + ' <small>(' + (b / a * 100.0).toFixed(2) + '%)</small>';
	} else
		stats.swap = '';

	stats.time = '<% time(); %>';
	stats.wanup = '<% wanup(); %>' == '1';
	stats.wanuptime = '<% link_uptime(); %>';
	stats.wanlease = '<% dhcpc_time(); %>';

	//<% dns(); %>
	stats.dns = dns.join(', ');

	stats.wanip = nvram.wan_ipaddr;
	stats.wannetmask = nvram.wan_netmask;
	stats.wangateway = nvram.wan_gateway_get;
	if (stats.wangateway == '0.0.0.0' || stats.wangateway == '')
		stats.wangateway = nvram.wan_gateway;

	switch (nvram.wan_proto) {
	case 'pptp':
	case 'l2tp':
		if (stats.wanup) {
			stats.wanip = nvram.ppp_get_ip;
			if (nvram.pptp_dhcp == '1') {
				if (nvram.wan_ipaddr != '' && nvram.wan_ipaddr != '0.0.0.0' && nvram.wan_ipaddr != stats.wanip)
					stats.wanip += '&nbsp;&nbsp;<small>(DHCP: ' + nvram.wan_ipaddr + ')</small>';
				if (nvram.wan_gateway != '' && nvram.wan_gateway != '0.0.0.0' && nvram.wan_gateway != stats.wangateway)
					stats.wangateway += '&nbsp;&nbsp;<small>(DHCP: ' + nvram.wan_gateway + ')</small>';
			}
			if (stats.wannetmask == '0.0.0.0')
				stats.wannetmask = '255.255.255.255';
		}
		else {
			if (nvram.wan_proto == 'pptp')
				stats.wangateway = nvram.pptp_server_ip;
		}
		break;
	default:
		if (!stats.wanup) {
			stats.wanip = '0.0.0.0';
			stats.wannetmask = '0.0.0.0';
			stats.wangateway = '0.0.0.0';
		}
	}

	stats.wanstatus = '<% wanstatus(); %>';
	if (stats.wanstatus != 'Connected') stats.wanstatus = '<b>' + stats.wanstatus + '</b>';

	stats.channel = [];
	stats.interference = [];
	stats.qual = [];

	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);

		a = i = wlstats[uidx].channel * 1;
		if (i < 0) i = -i;
		stats.channel.push('<a href="tools-survey.asp">' + ((i) ? i + '' : 'Auto') +
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
				stats.qual.push(a + ' <img src="bar' + MIN(MAX(Math.floor(a / 10), 1), 6) + '.gif">');
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
	}
} while (0);
