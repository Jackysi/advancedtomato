/*
	Tomato GUI
	Copyright (C) 2006-2009 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
*/

//<% nvram("l2tp_get_ip,pptp_get_ip,pptp_server_ip,router_name,wan_domain,wan_gateway,wan_get_domain,wan_hostname,wan_hwaddr,wan_ipaddr,wan_netmask,wan_proto,wan_run_mtu,et0macaddr,lan_proto,lan_ipaddr,dhcp_start,dhcp_num,dhcpd_startip,dhcpd_endip,lan_netmask,security_mode2,wl_crypto,wl_mode,wds_enable,wl0_hwaddr,wl_net_mode,wl_radio,wl_channel,lan_gateway,wl_ssid,t_model_name,t_features"); %>
//<% uptime(); %>
//<% sysinfo(); %>
//<% wlradio(); %>

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
	stats.wangateway = nvram.wan_gateway;

	switch (nvram.wan_proto) {
	case 'pptp':
		if (stats.wanup) {
			stats.wanip = nvram.pptp_get_ip;
			stats.wannetmask = '255.255.255.255';
		}
		else {
			stats.wangateway = nvram.pptp_server_ip;
		}
		break;
	case 'l2tp':
		if (stats.wanup) {
			stats.wanip = nvram.l2tp_get_ip;
			stats.wannetmask = '255.255.255.255';
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

	// <% wlchannel(); %>
	a = i = wlchaninfo[0] * 1;
	if (i < 0) i = -i;
	stats.channel = '<a href="tools-survey.asp">' + ((i) ? i + '' : 'Auto') +
		((wlchaninfo[1]) ? ' - ' + (wlchaninfo[1] / 1000.0).toFixed(3) + ' <small>GHz</small>' : '') + '</a>' +
		((a < 0) ? ' <small>(scanning...)</small>' : '');
	stats.interference = (wlchaninfo[2] >= 0) ? ((wlchaninfo[2]) ? 'Severe' : 'Acceptable') : '';

	a = '<% wlnbw(); %>' * 1;
	if (a > 0)
		stats.nbw = a + ' <small>MHz</small>';
	else
		stats.nbw = 'Auto';

	wlcrssi = wlnoise = stats.qual = stats.rate = '';
	isClient = ((nvram.wl_mode == 'wet') || (nvram.wl_mode == 'sta'));
	if (wlradio) {
		a = '<% wlrate(); %>' * 1;
		if (a > 0)
			stats.rate = Math.floor(a / 2) + ((a & 1) ? '.5' : '') + ' <small>Mbps</small>';
		if (isClient) {
			//<% wlnoise(); %>
			//<% wlcrssi(); %>
			if (wlcrssi == 0) a = 0;
				else a = MAX(wlcrssi - wlnoise, 0);
			stats.qual = a + ' <img src="bar' + MIN(MAX(Math.floor(a / 10), 1), 6) + '.gif">';
		}
		wlnoise += ' <small>dBm</small>';
		wlcrssi += ' <small>dBm</small>';
	}
} while (0);
