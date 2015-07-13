//<% nvram("at_update,ppp_get_ip,pptp_server_ip,router_name,t_model_name,wan_ipaddr_buf,wan_domain,wan_gateway,wan_gateway_get,wan_get_domain,wan_hostname,wan_hwaddr,wan_ipaddr,wan_netmask,wan_proto,wan_run_mtu,et0macaddr,lan_proto,lan_ipaddr,dhcp_start,dhcp_num,dhcpd_startip,dhcpd_endip,lan_netmask,wl_security_mode,wl_crypto,wl_mode,wl_wds_enable,wl_hwaddr,wl_net_mode,wl_radio,wl_channel,lan_gateway,wl_ssid,wl_closed,t_model_name,t_features,pptp_dhcp,dhcp1_start,dhcp1_num,dhcpd1_startip,dhcpd1_endip,dhcp2_start,dhcp2_num,dhcpd2_startip,dhcpd2_endip,dhcp3_start,dhcp3_num,dhcpd3_startip,dhcpd3_endip,lan1_proto,lan1_ipaddr,lan1_netmask,lan2_proto,lan2_ipaddr,lan2_netmask,lan3_proto,lan3_ipaddr,lan3_netmask,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan_ifnames,lan1_ifnames,lan2_ifnames,lan3_ifnames,wan_ifnames,tomatoanon_enable,tomatoanon_answer,lan_desc"); %>
//<% uptime(); %>
//<% sysinfo(); %>
//<% wlstats(1); %>
stats = { };
do {
	var a, b, i;
	var xifs = ['wan', 'lan', 'lan1', 'lan2', 'lan3'];
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
	stats.systemtype = sysinfo.systemtype;
	stats.cpuload = ((sysinfo.loads[0] / 65536.0).toFixed(2) + '<small> / </small> ' +
		(sysinfo.loads[1] / 65536.0).toFixed(2) + '<small> / </small>' +
		(sysinfo.loads[2] / 65536.0).toFixed(2));
	stats.uptime = sysinfo.uptime_s;

	stats.wlsense = sysinfo.wlsense;
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
	stats.wanup = '<% wanup(); %>' == '1';
	stats.wanprebuf = nvram.wan_ipaddr_buf;
	stats.wanuptime = '<% link_uptime(); %>';
	stats.wanlease = '<% dhcpc_time(); %>';
	stats.routermodel = nvram.t_model_name;
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
	
/* IPV6-BEGIN */
	stats.ip6_wan = ((typeof(sysinfo.ip6_wan) != 'undefined') ? sysinfo.ip6_wan : '') + '';
	stats.ip6_lan = ((typeof(sysinfo.ip6_lan) != 'undefined') ? sysinfo.ip6_lan : '') + '';
	stats.ip6_lan_ll = ((typeof(sysinfo.ip6_lan_ll) != 'undefined') ? sysinfo.ip6_lan_ll : '') + '';
/* IPV6-END */

	stats.wanstatus = '<% wanstatus(); %>';
	if (stats.wanstatus != 'Connected') stats.wanstatus = '<b>' + stats.wanstatus + '</b>';
	stats.channel = [];
	stats.interference = [];
	stats.qual = [];

	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);
		a = i = wlstats[uidx].channel * 1;
		if (i < 0) i = -i;
		stats.channel.push('<a class="ajaxload" href="#tools-survey.asp">' + ((i) ? i + '' : 'Auto') +
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
