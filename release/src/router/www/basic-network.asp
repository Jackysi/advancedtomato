<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Basic: Network</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript' src='md5.js'></script>
<script type='text/javascript' src='wireless.jsx?_http_id=<% nv(http_id); %>'></script>
<script type='text/javascript'>
//	<% nvram("dhcp_lease,dhcp_num,dhcp_start,dhcpd_startip,dhcpd_endip,l2tp_server_ip,lan_gateway,lan_ipaddr,lan_netmask,lan_proto,mtu_enable,ppp_demand,ppp_idletime,ppp_passwd,ppp_redialperiod,ppp_service,ppp_username,ppp_custom,pptp_server_ip,pptp_dhcp,ppp_defgw,wl_security_mode,wan_dns,wan_gateway,wan_ipaddr,wan_mtu,wan_netmask,wan_proto,wan_wins,wl_wds_enable,wl_channel,wl_closed,wl_crypto,wl_key,wl_key1,wl_key2,wl_key3,wl_key4,wl_lazywds,wl_mode,wl_net_mode,wl_passphrase,wl_radio,wl_radius_ipaddr,wl_radius_port,wl_ssid,wl_wds,wl_wep_bit,wl_wpa_gtk_rekey,wl_wpa_psk,wl_radius_key,wl_auth,wl_hwaddr,wan_islan,t_features,wl_nbw_cap,wl_nctrlsb,wl_nband,wl_phytype"); %>

W('<style type=\'text/css\'>');
for (var u = 0; u < wl_ifaces.length; ++u) {
	W('#spin'+wl_unit(u)+', ');
}
W('#spin {');
W('	visibility: hidden;');
W('	vertical-align: middle;');
W('}');
W('</style>');

var xob = null;
var refresher = [];
var nphy = features('11n');

if ((!fixIP(nvram.dhcpd_startip)) || (!fixIP(nvram.dhcpd_endip))) {
	var x = nvram.lan_ipaddr.split('.').splice(0, 3).join('.') + '.';
	nvram.dhcpd_startip = x + nvram.dhcp_start;
	nvram.dhcpd_endip = x + ((nvram.dhcp_start * 1) + (nvram.dhcp_num * 1) - 1);
}

var ghz = [];
var bands = [];
var nm_loaded = [], ch_loaded = [], max_channel = [];

for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	var b;
	b = [];
	for (var i = 0; i < wl_bands[uidx].length; ++i) {
		b.push([wl_bands[uidx][i] + '', (wl_bands[uidx][i] == '1') ? '5 GHz' : '2.4 GHz']);
	}
	bands.push(b);

	b = [];
	ghz.push(b);

	nm_loaded.push(0);
	ch_loaded.push(0);
	max_channel.push(0);
	refresher.push(null);
}

function selectedBand(uidx)
{
	if (bands[uidx].length > 1) {
		var e = E('_f_wl'+u+'_nband');
		return (e.value + '' == '' ? eval('nvram.wl'+u+'_nband') : e.value);
	} else if (bands[uidx].length > 0) {
		return bands[uidx][0][0] || '0';
	} else {
		return '0';
	}
}

function refreshNetModes(uidx)
{
	var e, i, buf, val;

	if (uidx >= wl_ifaces.length) return;
	var u = wl_unit(uidx);

	var m = [['mixed','Auto']];
	if (selectedBand(uidx) == '1') {
		m.push(['a-only','A Only']);
		if (nphy) {
			m.push(['n-only','N Only']);
		}
	}
	else {
		m.push(['b-only','B Only']);
		m.push(['g-only','G Only']);
		if (nphy) {
			m.push(['bg-mixed','B/G Mixed']);
			m.push(['n-only','N Only']);
		}
	}

	e = E('_wl'+u+'_net_mode');
	buf = '';
	val = (!nm_loaded[uidx] || (e.value + '' == '')) ? eval('nvram.wl'+u+'_net_mode') : e.value;
	if (val == 'disabled') val = 'mixed';
	for (i = 0; i < m.length; ++i)
		buf += '<option value="' + m[i][0] + '"' + ((m[i][0] == val) ? ' selected' : '') + '>' + m[i][1] + '</option>';

	e = E('__wl'+u+'_net_mode');
	buf = '<select name="wl'+u+'_net_mode" onchange="verifyFields(this, 1)" id = "_wl'+u+'_net_mode">' + buf + '</select>';
	elem.setInnerHTML(e, buf);
	nm_loaded[uidx] = 1;
}

function refreshChannels(uidx)
{
	if (refresher[uidx] != null) return;
	if (u >= wl_ifaces.length) return;
	var u = wl_unit(uidx);

	refresher[uidx] = new XmlHttp();
	refresher[uidx].onCompleted = function(text, xml) {
		try {
			var e, i, buf, val;

			var wl_channels = [];
			eval(text);

			ghz[uidx] = [];
			max_channel[uidx] = 0;
			for (i = 0; i < wl_channels.length; ++i) {
				ghz[uidx].push([wl_channels[i][0] + '',
					(wl_channels[i][0]) ? ((wl_channels[i][1]) ? wl_channels[i][0] + ' - ' + (wl_channels[i][1] / 1000.0).toFixed(3) + ' GHz' : wl_channels[i][0] + '') : 'Auto']);
				max_channel[uidx] = wl_channels[i][0] * 1;
			}

			e = E('_wl'+u+'_channel');
			buf = '';
			val = (!ch_loaded[uidx] || (e.value + '' == '')) ? eval('nvram.wl'+u+'_channel') : e.value;
			for (i = 0; i < ghz[uidx].length; ++i)
				buf += '<option value="' + ghz[uidx][i][0] + '"' + ((ghz[uidx][i][0] == val) ? ' selected' : '') + '>' + ghz[uidx][i][1] + '</option>';

			e = E('__wl'+u+'_channel');
			buf = '<select name="wl'+u+'_channel" onchange="verifyFields(this, 1)" id = "_wl'+u+'_channel">' + buf + '</select>';
			elem.setInnerHTML(e, buf);
			ch_loaded[uidx] = 1;

			refresher[uidx] = null;
			verifyFields(null, 1);
		}
		catch (x) {
		}
		refresher[uidx] = null;
	}

	var bw, sb, e;

	e = E('_f_wl'+u+'_nctrlsb');
	sb = (e.value + '' == '' ? eval('nvram.wl'+u+'_nctrlsb') : e.value);
	e = E('_wl'+u+'_nbw_cap');
	bw = (e.value + '' == '' ? eval('nvram.wl'+u+'_nbw_cap') : e.value) == '0' ? '20' : '40';

	refresher[uidx].onError = function(ex) { alert(ex); refresher[uidx] = null; reloadPage(); }
	refresher[uidx].post('update.cgi', 'exec=wlchannels&arg0=' + u + '&arg1=' + (nphy ? '1' : '0') +
		'&arg2=' + bw + '&arg3=' + selectedBand(uidx) + '&arg4=' + sb);
}

function spin(x, unit)
{
	for (var u = 0; u < wl_ifaces.length; ++u) {
		E('_f_wl'+wl_unit(u)+'_scan').disabled = x;
	}
	var e = E('_f_wl'+unit+'_scan');
	if (x) e.value = 'Scan ' + (wscan.tries + 1);
		else e.value = 'Scan';
	E('spin'+unit).style.visibility = x ? 'visible' : 'hidden';
}

function scan()
{
	if (xob) return;

	var unit = wscan.unit;
	var uidx = wl_uidx(unit);

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		try {
			var i;

			wlscandata = [];
			eval(text);

			for (i = 0; i < wlscandata.length; ++i) {
				var data = wlscandata[i];
				var ch = data[2];
				var mac = data[0];

				if (!wscan.inuse[ch]) {
					wscan.inuse[ch] = {
						count: 0,
						rssi: -999,
						ssid: ''
					};
				}

				if (!wscan.seen[mac]) {
					wscan.seen[mac] = 1;
					++wscan.inuse[ch].count;
				}

				if (data[4] > wscan.inuse[ch].rssi) {
					wscan.inuse[ch].rssi = data[4];
					wscan.inuse[ch].ssid = data[1];
				}
			}
			var e = E('_wl'+unit+'_channel');
			for (i = 1; i < ghz[uidx].length; ++i) {
				var s = ghz[uidx][i][1];
				var u = wscan.inuse[ghz[uidx][i][0]];
				if (u) s += ' (' + u.count + ' AP' + (u.count == 1 ? '' : 's') + ' / strongest: "' + ellipsis(u.ssid, 15) + '" ' + u.rssi + ' dBm)';
				e.options[i].innerHTML = s;
			}
			e.style.width = '400px';

			xob = null;

			if (wscan.tries < 4) {
				++wscan.tries;
				setTimeout(scan, 1000);
				return;
			}
		}
		catch (x) {
		}
		spin(0, unit);
	}
	xob.onError = function(x) {
		alert('error: ' + x);
		spin(0, unit);
		xob = null;
	}

	spin(1, unit);
	xob.post('update.cgi', 'exec=wlscan&arg0='+unit);
}

function scanButton(u)
{
	if (xob) return;

	wscan = {
		unit: u,
		seen: [],
		inuse: [],
		tries: 0
	};

	scan();
}

function joinAddr(a) {
	var r, i, s;

	r = [];
	for (i = 0; i < a.length; ++i) {
		s = a[i];
		if ((s != '00:00:00:00:00:00') && (s != '0.0.0.0')) r.push(s);
	}
	return r.join(' ');
}

function random_x(max)
{
	var c = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
	var s = '';
	while (max-- > 0) s += c.substr(Math.floor(c.length * Math.random()), 1);
	return s;
}

function random_psk(id)
{
	var e = E(id);
	e.value = random_x(63);
	verifyFields(null, 1);
}

function random_wep(u)
{
	E('_wl'+u+'_passphrase').value = random_x(16);
	generate_wep(u);
}

function v_wep(e, quiet)
{
	var s = e.value;
	
	if (((s.length == 5) || (s.length == 13)) && (s.length == (e.maxLength >> 1))) {
		// no checking
	}
	else {
		s = s.toUpperCase().replace(/[^0-9A-F]/g, '');
		if (s.length != e.maxLength) {
			ferror.set(e, 'Invalid WEP key. Expecting ' + e.maxLength + ' hex or ' + (e.maxLength >> 1) + ' ASCII characters.', quiet);
			return 0;
		}
	}

	e.value = s;
	ferror.clear(e);
	return 1;
}

// compatible w/ Linksys' and Netgear's (key 1) method for 128-bits
function generate_wep(u)
{
	function _wepgen(pass, i)
	{
		while (pass.length < 64) pass += pass;
		return hex_md5(pass.substr(0, 64)).substr(i, (E('_wl'+u+'_wep_bit').value == 128) ? 26 : 10);
	}

	var e = E('_wl'+u+'_passphrase');
	var pass = e.value;
	if (!v_length(e, false, 3)) return;
	E('_wl'+u+'_key1').value = _wepgen(pass, 0);
	pass += '#$%';
	E('_wl'+u+'_key2').value = _wepgen(pass, 2);
	pass += '!@#';
	E('_wl'+u+'_key3').value = _wepgen(pass, 4);
	pass += '%&^';
	E('_wl'+u+'_key4').value = _wepgen(pass, 6);
	verifyFields(null, 1);
}

function verifyFields(focused, quiet)
{
	var i;
	var ok = 1;
	var a, b, c, d, e;
	var u, uidx;
	var wmode, sm2;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);
		if (focused == E('_f_wl'+u+'_nband')) {
			refreshNetModes(uidx);
			refreshChannels(uidx);
		}
		else if (focused == E('_f_wl'+u+'_nctrlsb') || focused == E('_wl'+u+'_nbw_cap')) {
			refreshChannels(uidx);
		}
	}

	// --- visibility ---

	var vis = {
		_wan_proto: 1,
		_ppp_username: 1,
		_ppp_passwd: 1,
		_ppp_service: 1,
		_ppp_custom: 1,
		_l2tp_server_ip: 1,
		_wan_ipaddr: 1,
		_wan_netmask: 1,
		_wan_gateway: 1,
		_pptp_server_ip: 1,
		_f_pptp_dhcp: 1,
		_f_ppp_defgw: 1,
		_ppp_demand: 1,
		_ppp_idletime: 1,
		_ppp_redialperiod: 1,
		_mtu_enable: 1,
		_f_wan_mtu: 1,
		_f_wan_islan: 0,

		_dhcp_lease: 1,
		_f_dhcpd_enable: 1,
		_dhcpd_startip: 1,
		_dhcpd_endip: 1,
		_f_dns_1: 1,
		_f_dns_2: 1,
		_f_dns_3: 1,
		_lan_gateway: 1,
		_lan_ipaddr: 1,
		_lan_netmask: 1,
		_wan_wins: 1
	};

	var wl_vis = [];
	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		a = {
		_f_wl_radio: 1,
		_f_wl_mode: 1,
		_f_wl_nband: (bands[uidx].length > 1) ? 1 : 0,
		_wl_net_mode: 1,
		_wl_ssid: 1,
		_f_wl_bcast: 1,
		_wl_channel: 1,
		_wl_nbw_cap: nphy ? 1 : 0,
		_f_wl_nctrlsb: nphy ? 1 : 0,
		_f_wl_scan: 1,

		_wl_security_mode: 1,
		_wl_crypto: 1,
		_wl_wpa_psk: 1,
		_f_wl_psk_random1: 1,
		_f_wl_psk_random2: 1,
		_wl_wpa_gtk_rekey: 1,
		_wl_radius_key: 1,
		_wl_radius_ipaddr: 1,
		_wl_radius_port: 1,
		_wl_wep_bit: 1,
		_wl_passphrase: 1,
		_f_wl_wep_gen: 1,
		_f_wl_wep_random: 1,
		_wl_key1: 1,
		_wl_key2: 1,
		_wl_key3: 1,
		_wl_key4: 1,

		_f_wl_lazywds: 1,
		_f_wl_wds_0: 1
		};
		wl_vis.push(a);
	}

	var wan = E('_wan_proto').value;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		wmode = E('_f_wl'+wl_unit(uidx)+'_mode').value;

		if (wmode == 'wet') {
			wan = 'disabled';
			vis._wan_proto = 0;
			vis._f_dhcpd_enable = 0;
			vis._dhcp_lease = 0;
		}

		if ((wan == 'disabled') || (wmode == 'sta') || (wmode == 'wet')) {
			vis._f_wan_islan = 1;
		}
	}

	switch (wan) {
	case 'disabled':
		vis._ppp_username = 0;
		vis._ppp_service = 0;
		vis._ppp_custom = 0;
		vis._l2tp_server_ip = 0;
		vis._wan_ipaddr = 0;
		vis._wan_netmask = 0;
		vis._wan_gateway = 0;
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;
		vis._f_ppp_defgw = 0;
		vis._ppp_demand = 0;
		vis._mtu_enable = 0;
		vis._f_wan_mtu = 0;
		break;
	case 'dhcp':
		vis._l2tp_server_ip = 0;
		vis._ppp_demand = 0;
		vis._ppp_service = 0;
		vis._ppp_username = 0;
		vis._ppp_custom = 0;
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;
		vis._f_ppp_defgw = 0;
		vis._wan_gateway = 0;
		vis._wan_ipaddr = 0;
		vis._wan_netmask = 0;

		vis._lan_gateway = 0;
		break;
	case 'pppoe':
		vis._l2tp_server_ip = 0;
		vis._pptp_server_ip = 0;
		vis._ppp_custom = 0;
		vis._f_pptp_dhcp = 0;
		vis._f_ppp_defgw = 0;
		vis._wan_gateway = 0;
		vis._wan_ipaddr = 0;
		vis._wan_netmask = 0;

		vis._lan_gateway = 0;
		break;
	case 'static':
		vis._l2tp_server_ip = 0;
		vis._ppp_demand = 0;
		vis._ppp_service = 0;
		vis._ppp_username = 0;
		vis._ppp_custom = 0;
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;
		vis._f_ppp_defgw = 0;

		vis._lan_gateway = 0;
		break;
	case 'pptp':
		vis._l2tp_server_ip = 0;
		vis._ppp_service = 0;
		vis._wan_gateway = (!E('_f_pptp_dhcp').checked);
		vis._wan_ipaddr = (!E('_f_pptp_dhcp').checked);
		vis._f_ppp_defgw = (E('_f_pptp_dhcp').checked);

		vis._lan_gateway = 0;
		break;
	case 'l2tp':
		vis._pptp_server_ip = 0;
		vis._ppp_service = 0;
		vis._wan_gateway = (!E('_f_pptp_dhcp').checked);
		vis._wan_ipaddr = (!E('_f_pptp_dhcp').checked);
		vis._f_ppp_defgw = (E('_f_pptp_dhcp').checked);

		vis._lan_gateway = 0;
		break;
	}

	vis._ppp_idletime = (E('_ppp_demand').value == 1) && vis._ppp_demand
	vis._ppp_redialperiod = !vis._ppp_idletime && vis._ppp_demand;

	if (vis._mtu_enable) {
		if (E('_mtu_enable').value == 0) {
			vis._f_wan_mtu = 2;
			a = E('_f_wan_mtu');
			switch (E('_wan_proto').value) {
			case 'pppoe':
				a.value = 1492;
				break;
			case 'pptp':
			case 'l2tp':
				a.value = 1460;
				break;
			 default:
				a.value = 1500;
				break;
			}
		}
	}

	if (!E('_f_dhcpd_enable').checked) vis._dhcp_lease = 0;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);
		wmode = E('_f_wl'+u+'_mode').value;

		if (!E('_f_wl'+u+'_radio').checked) {
			for (a in wl_vis[uidx]) {
				wl_vis[uidx][a] = 2;
			}
			wl_vis[uidx]._f_wl_radio = 1;
			wl_vis[uidx]._wl_nbw_cap = nphy ? 2 : 0;
			wl_vis[uidx]._f_wl_nband = (bands[uidx].length > 1) ? 2 : 0;
		}

		switch (wmode) {
		case 'apwds':
		case 'wds':
			break;
		case 'wet':
		case 'sta':
			wl_vis[uidx]._f_wl_bcast = 0;
			wl_vis[uidx]._wl_channel = 0;
			wl_vis[uidx]._wl_nbw_cap = 0;
		default:
			wl_vis[uidx]._f_wl_lazywds = 0;
			wl_vis[uidx]._f_wl_wds_0 = 0;
			break;
		}

		sm2 = E('_wl'+u+'_security_mode').value;
		switch (sm2) {
		case 'disabled':
			wl_vis[uidx]._wl_crypto = 0;
			wl_vis[uidx]._wl_wep_bit = 0;
			wl_vis[uidx]._wl_wpa_psk = 0;
			wl_vis[uidx]._wl_radius_key = 0;
			wl_vis[uidx]._wl_radius_ipaddr = 0;
			wl_vis[uidx]._wl_wpa_gtk_rekey = 0;
			break;
		case 'wep':
			wl_vis[uidx]._wl_crypto = 0;
			wl_vis[uidx]._wl_wpa_psk = 0;
			wl_vis[uidx]._wl_radius_key = 0;
			wl_vis[uidx]._wl_radius_ipaddr = 0;
			wl_vis[uidx]._wl_wpa_gtk_rekey = 0;
			break;
		case 'radius':
			wl_vis[uidx]._wl_crypto = 0;
			wl_vis[uidx]._wl_wpa_psk = 0;
			break;
		default:	// wpa*
			wl_vis[uidx]._wl_wep_bit = 0;
			if (sm2.indexOf('personal') != -1) {
				wl_vis[uidx]._wl_radius_key = 0;
				wl_vis[uidx]._wl_radius_ipaddr = 0;
			}
			else {
				wl_vis[uidx]._wl_wpa_psk = 0;
			}
			break;
		}

		if ((E('_f_wl'+u+'_lazywds').value == 1) && (wl_vis[uidx]._f_wl_wds_0 == 1)) {
			wl_vis[uidx]._f_wl_wds_0 = 2;
		}

		if (wl_vis[uidx]._wl_nbw_cap != 0) {
			switch (E('_wl'+u+'_net_mode').value) {
			case 'b-only':
			case 'g-only':
			case 'a-only':
			case 'bg-mixed':
				wl_vis[uidx]._wl_nbw_cap = 2;
				if (E('_wl'+u+'_nbw_cap').value != '0') {
					E('_wl'+u+'_nbw_cap').value = 0;
					refreshChannels(uidx);
				}
				break;
			}
			// avoid Enterprise-TKIP with 40MHz
			if ((sm2 == 'wpa_enterprise') && (E('_wl'+u+'_crypto').value == 'tkip')) {
				wl_vis[uidx]._wl_nbw_cap = 2;
				if (E('_wl'+u+'_nbw_cap').value != '0') {
					E('_wl'+u+'_nbw_cap').value = 0;
					refreshChannels(uidx);
				}
			}
		}

		wl_vis[uidx]._f_wl_nctrlsb = (E('_wl'+u+'_nbw_cap').value == 0) ? 0 : wl_vis[uidx]._wl_nbw_cap;

/* REMOVE-BEGIN
	This is ugly...
	Special case - 2.4GHz band, currently running in B/G-only mode,
	with N/Auto and 40MHz selected in the GUI.
	Channel list is not filtered in this case by the wl driver,
	and includes all channels available with 20MHz channel width.
REMOVE-END */
		b = selectedBand(uidx);
		if (wl_vis[uidx]._wl_channel == 1 && wl_vis[uidx]._f_wl_nctrlsb != 0 &&
		   ((b == '2') || (wl_vis[uidx]._f_wl_nband == 0 && b == '0'))) {
			switch (eval('nvram.wl'+u+'_net_mode')) {
			case 'b-only':
			case 'g-only':
			case 'bg-mixed':
				i = E('_wl'+u+'_channel').value * 1;
				if (i > 0 && i < 5) {
					E('_f_wl'+u+'_nctrlsb').value = 'lower';
					wl_vis[uidx]._f_wl_nctrlsb = 2;
				}
				else if (i > max_channel[uidx] - 4) {
					E('_f_wl'+u+'_nctrlsb').value = 'upper';
					wl_vis[uidx]._f_wl_nctrlsb = 2;
				}
				break;
			}
		}

		wl_vis[uidx]._f_wl_scan = wl_vis[uidx]._wl_channel;
		wl_vis[uidx]._f_wl_psk_random1 = wl_vis[uidx]._wl_wpa_psk;
		wl_vis[uidx]._f_wl_psk_random2 = wl_vis[uidx]._wl_radius_key;
		wl_vis[uidx]._wl_radius_port = wl_vis[uidx]._wl_radius_ipaddr;
		wl_vis[uidx]._wl_key1 = wl_vis[uidx]._wl_key2 = wl_vis[uidx]._wl_key3 = wl_vis[uidx]._wl_key4 = wl_vis[uidx]._f_wl_wep_gen = wl_vis[uidx]._f_wl_wep_random = wl_vis[uidx]._wl_passphrase = wl_vis[uidx]._wl_wep_bit;

		for (i = 1; i < 10; ++i) {
			wl_vis[uidx]['_f_wl_wds_' + i] = wl_vis[uidx]._f_wl_wds_0;
		}
	} // for each wl_iface

	//

	vis._ppp_passwd = vis._ppp_username;
	vis._dhcpd_startip = vis._dhcpd_endip = vis._wan_wins = vis._dhcp_lease;

	for (a in vis) {
		b = E(a);
		c = vis[a];
		b.disabled = (c != 1);
		PR(b).style.display = c ? '' : 'none';
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		for (a in wl_vis[uidx]) {
			i = 3;
			if (a.substr(0, 6) == '_f_wl_') i = 5;
			b = E(a.substr(0, i) + wl_unit(uidx) + a.substr(i, a.length));
			c = wl_vis[uidx][a];
			b.disabled = (c != 1);
			PR(b).style.display = c ? '' : 'none';
		}
	}

	// --- verify ---

	ferror.clear('_wan_proto');

	var wlclnt = 0;
	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);
		wmode = E('_f_wl'+u+'_mode').value;
		sm2 = E('_wl'+u+'_security_mode').value;

/* REMOVE-BEGIN
		if ((wl_vis[uidx]._f_wl_mode == 1) && (wmode != 'ap') && (sm2.substr(0, 4) == 'wpa2')) {
			ferror.set('_wl'+u+'_security_mode', 'WPA2 is supported only in AP mode.', quiet || !ok);
			ok = 0;
		}
		else ferror.clear('_wl'+u+'_security_mode');
REMOVE-END */

		// --- N standard does not support WPA+TKIP ---
		a = E('_wl'+u+'_crypto');
		switch (E('_wl'+u+'_net_mode').value) {
		case 'mixed':
		case 'n-only':
			if (nphy && (a.value == 'tkip') && (sm2.indexOf('wpa') != -1)) {
				ferror.set(a, 'TKIP encryption is not supported with WPA / WPA2 in N mode.', quiet || !ok);
				ok = 0;
			}
			else ferror.clear(a);
			break;
		}

		a = E('_wl'+u+'_net_mode');
		ferror.clear(a);
		b = E('_f_wl'+u+'_mode');
		ferror.clear(b);
		if ((wmode == 'sta') || (wmode == 'wet')) {
			++wlclnt;
			if (wlclnt > 1) {
				ferror.set(b, 'Only one wireless interface can be configured in client mode.', quiet || !ok);
				ok = 0;
			}
			else if (a.value == 'n-only') {
				ferror.set(a, 'N-only is not supported in wireless client modes, use Auto.', quiet || !ok);
				ok = 0;
			}
		}

		a = E('_wl'+u+'_wpa_psk');
		ferror.clear(a);
		if (wl_vis[uidx]._wl_wpa_psk == 1) {
			if ((a.value.length < 8) || ((a.value.length == 64) && (a.value.search(/[^0-9A-Fa-f]/) != -1))) {
				ferror.set('_wl'+u+'_wpa_psk', 'Invalid pre-shared key. Please enter at least 8 characters or 64 hexadecimal digits.', quiet || !ok);
				ok = 0;
			}
		}

		// wl channel
		if (((wmode == 'wds') || (wmode == 'apwds')) && (wl_vis[uidx]._wl_channel == 1) && (E('_wl'+u+'_channel').value == '0')) {
			ferror.set('_wl'+u+'_channel', 'Fixed wireless channel required in WDS mode.', quiet || !ok);
			ok = 0;
		}
		else ferror.clear('_wl'+u+'_channel');

		if (E('_f_wl'+u+'_mode').value == 'sta') {
			if ((wan == 'disabled') && (E('_f_wl'+u+'_radio').checked)) {
				ferror.set('_wan_proto', 'Wireless Client mode requires a valid WAN setting (usually DHCP).', quiet || !ok);
				ok = 0;
			}
		}
	}

	// domain name or IP address
	a = ['_l2tp_server_ip', '_pptp_server_ip'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && ((!v_length(a[i], 1, 1)) || ((!v_ip(a[i], 1)) && (!v_domain(a[i], 1))))) {
			if (!quiet && ok) ferror.show(a[i]);
			ok = 0;
		}

	// IP address
	a = ['_wan_gateway','_wan_ipaddr','_lan_ipaddr', '_dhcpd_startip', '_dhcpd_endip'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_ip(a[i], quiet || !ok))) ok = 0;

	// IP address, blank -> 0.0.0.0
	a = ['_f_dns_1', '_f_dns_2', '_f_dns_3','_wan_wins','_lan_gateway'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_dns(a[i], quiet || !ok))) ok = 0;

	// netmask
	a = ['_wan_netmask','_lan_netmask'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_netmask(a[i], quiet || !ok))) ok = 0;

	// range
	a = [['_ppp_idletime', 3, 1440],['_ppp_redialperiod', 1, 86400],['_f_wan_mtu', 576, 1500],
		 ['_dhcp_lease', 1, 10080]];
	for (i = a.length - 1; i >= 0; --i) {
		v = a[i];
		if ((vis[v[0]]) && (!v_range(v[0], quiet || !ok, v[1], v[2]))) ok = 0;
	}

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);

		// IP address
		a = ['_wl'+u+'_radius_ipaddr'];
		for (i = a.length - 1; i >= 0; --i) {
			if (elem.isVisible(E(a[i])) && (!v_ip(a[i], quiet || !ok))) ok = 0;
		}

		// range
		a = [['_wl'+u+'_wpa_gtk_rekey', 60, 7200], ['_wl'+u+'_radius_port', 1, 65535]];
		for (i = a.length - 1; i >= 0; --i) {
			v = a[i];
			if (elem.isVisible(E(v[0])) && (!v_range(v[0], quiet || !ok, v[1], v[2]))) ok = 0;
		}

		// length
		a = [['_wl'+u+'_ssid', 1], ['_wl'+u+'_radius_key', 1]];
		for (i = a.length - 1; i >= 0; --i) {
			v = a[i];
			if (elem.isVisible(E(v[0])) && (!v_length(v[0], quiet || !ok, v[1], E(v[0]).maxlength))) ok = 0;
		}

		if (wl_vis[uidx]._wl_key1) {
			a = (E('_wl'+u+'_wep_bit').value == 128) ? 26 : 10;
			for (i = 1; i <= 4; ++i) {
				b = E('_wl'+u+'_key' + i);
				b.maxLength = a;
				if ((b.value.length > 0) || (E('_f_wl'+u+'_wepidx_' + i).checked)) {
					if (!v_wep(b, quiet || !ok)) ok = 0;
				}
				else ferror.clear(b);
			}
		}

		ferror.clear('_f_wl'+u+'_wds_0');
		if (wl_vis[uidx]._f_wl_wds_0 == 1) {
			b = 0;
			for (i = 0; i < 10; ++i) {
				a = E('_f_wl'+u+'_wds_' + i);
				if (!v_macz(a, quiet || !ok)) ok = 0;
					else if (!isMAC0(a.value)) b = 1;
			}
			if (!b) {
				ferror.set('_f_wl'+u+'_wds_0', 'WDS MAC address required.', quiet || !ok);
				ok = 0;
			}
		}
	}

	a = E('_dhcpd_startip');
	b = E('_dhcpd_endip');
	ferror.clear(a);
	ferror.clear(b);

	if ((!a._error_msg) && (!b._error_msg)) {
		c = aton(E('_lan_netmask').value);
		d = aton(E('_lan_ipaddr').value) & c;
		e = 'Invalid IP address or subnet mask';
		if ((aton(a.value) & c) != d) {
			ferror.set(a, e, quiet || !ok);
			ok = 0;
		}
		if ((aton(b.value) & c) != d) {
			ferror.set(b, e, quiet || !ok);
			ok = 0;
		}
	}

	if ((!a._error_msg) && (!b._error_msg)) {
		if (aton(a.value) > aton(b.value)) {
			c = a.value;
			a.value = b.value;
			b.value = c;
		}

		elem.setInnerHTML('dhcp_count', '(' + ((aton(b.value) - aton(a.value)) + 1) + ')');
	}

	return ok;
}

function earlyInit()
{
	verifyFields(null, 1);
}

function save()
{
	var a, b, c;
	var i;
	var u, uidx, wmode, sm2, wradio;

	if (!verifyFields(null, false)) return;

	var fom = E('_fom');

	fom.lan_proto.value = fom.f_dhcpd_enable.checked ? 'dhcp' : 'static';

	fom.wan_mtu.value = fom.f_wan_mtu.value;
	fom.wan_mtu.disabled = fom.f_wan_mtu.disabled;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);
		wmode = E('_f_wl'+u+'_mode').value;
		sm2 = E('_wl'+u+'_security_mode').value;
		wradio = E('_f_wl'+u+'_radio').checked;

		E('_wl'+u+'_nband').value = selectedBand(uidx);

		if (wmode == 'apwds') E('_wl'+u+'_mode').value = 'ap';
		else E('_wl'+u+'_mode').value = wmode;

		if (wmode == 'wet') {
			fom.wan_proto.value = 'disabled';
			fom.wan_proto.disabled = 0;
			fom.lan_proto.value = 'static';
		}

		a = [];
		for (i = 0; i < 10; ++i) a.push(E('_f_wl'+u+'_wds_' + i).value);
		E('_wl'+u+'_wds').value = joinAddr(a);

		if (wmode.indexOf('wds') != -1) {
			E('_wl'+u+'_wds_enable').value = 1;
			E('_wl'+u+'_lazywds').value = E('_f_wl'+u+'_lazywds').value;
			if (E('_wl'+u+'_lazywds').value == 1) E('_wl'+u+'_wds').value = '';
		}
		else {
			E('_wl'+u+'_wds_enable').value = 0;
			E('_wl'+u+'_wds').value = '';
			E('_wl'+u+'_lazywds').value = 0;
		}

		E('_wl'+u+'_radio').value = wradio ? 1 : 0;
		E('_wl'+u+'_auth').value = eval('nvram.wl'+u+'_auth');

		e = E('_wl'+u+'_akm');
		switch (sm2) {
		case 'disabled':
		case 'radius':
		case 'wep':
			e.value = '';
			break;
		default:
			c = [];

			if (sm2.indexOf('personal') != -1) {
				if (sm2.indexOf('wpa2_') == -1) c.push('psk');
				if (sm2.indexOf('wpa_') == -1) c.push('psk2');
			}
			else {
				if (sm2.indexOf('wpa2_') == -1) c.push('wpa');
				if (sm2.indexOf('wpa_') == -1) c.push('wpa2');
			}
			c = c.join(' ');
			e.value = c;
			break;
		}
		E('_wl'+u+'_auth_mode').value = (sm2 == 'radius') ? 'radius' : 'none';
		E('_wl'+u+'_wep').value = ((sm2 == 'radius') || (sm2 == 'wep')) ? 'enabled': 'disabled';

		if (sm2.indexOf('wpa') != -1) E('_wl'+u+'_auth').value = 0;

		E('_wl'+u+'_nreqd').value = 0;
		E('_wl'+u+'_gmode').value = 1;
		E('_wl'+u+'_nmode').value = 0;
		E('_wl'+u+'_nmcsidx').value = -2; // Legacy Rate
		E('_wl'+u+'_nbw').value = 0;
		switch (E('_wl'+u+'_net_mode').value) {
		case 'b-only':
			E('_wl'+u+'_gmode').value = 0;
			break;
		case 'g-only':
			E('_wl'+u+'_gmode').value = 4;
			break;
		case 'bg-mixed':
			break;
		case 'a-only':
			E('_wl'+u+'_nmcsidx').value = -1; // Auto
			break;
		case 'n-only':
			if (selectedBand(uidx) == '1') { // 5 GHz
				E('_wl'+u+'_nmode').value = -1;
				E('_wl'+u+'_nmcsidx').value = -1;
			} else {
				E('_wl'+u+'_nmode').value = 1;
				E('_wl'+u+'_nmcsidx').value = 32;
			}
			E('_wl'+u+'_nreqd').value = 1;
			break;
		default: // Auto
			E('_wl'+u+'_nmode').value = -1;
			E('_wl'+u+'_nmcsidx').value = -1;
			break;
		}

		E('_wl'+u+'_nctrlsb').value = eval('nvram.wl'+u+'_nctrlsb');
		if (E('_wl'+u+'_nmode').value != 0) {
			E('_wl'+u+'_nctrlsb').value = E('_f_wl'+u+'_nctrlsb').value;
			E('_wl'+u+'_nbw').value = (E('_wl'+u+'_nbw_cap').value == 0) ? 20 : 40;
		}

		E('_wl'+u+'_closed').value = E('_f_wl'+u+'_bcast').checked ? 0 : 1;

		a = fields.radio.selected(eval('fom.f_wl'+u+'_wepidx'));
		if (a) E('_wl'+u+'_key').value = a.value;
	}
	
	fom.wan_islan.value = fom.f_wan_islan.checked ? 1 : 0;
	fom.pptp_dhcp.value = fom.f_pptp_dhcp.checked ? 1 : 0;
	fom.ppp_defgw.value = fom.f_pptp_dhcp.checked ? (fom.f_ppp_defgw.checked ? 1 : 0) : 1;

	fom.wan_dns.value = joinAddr([fom.f_dns_1.value, fom.f_dns_2.value, fom.f_dns_3.value])

	if (nvram.lan_ipaddr != fom.lan_ipaddr.value) {
		fom._moveip.value = 1;
		form.submit(fom);
	}
	else {
		form.submit(fom, 1);
	}
}

function init()
{
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		refreshNetModes(uidx);
		refreshChannels(uidx);
	}
}
</script>

</head>
<body onload='init()'>
<form id='_fom' method='post' action='tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_nextpage' value='basic-network.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_service' value='*'>
<input type='hidden' name='_moveip' value='0'>

<input type='hidden' name='wan_mtu'>
<input type='hidden' name='wan_islan'>
<input type='hidden' name='pptp_dhcp'>
<input type='hidden' name='ppp_defgw'>
<input type='hidden' name='lan_proto'>
<input type='hidden' name='wan_dns'>


<div class='section-title'>WAN / Internet</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Type', name: 'wan_proto', type: 'select', options: [['dhcp','DHCP'],['pppoe','PPPoE'],['static','Static'],['pptp','PPTP'],['l2tp','L2TP'],['disabled','Disabled']],
		value: nvram.wan_proto },
	{ title: 'Username', name: 'ppp_username', type: 'text', maxlen: 60, size: 64, value: nvram.ppp_username },
	{ title: 'Password', name: 'ppp_passwd', type: 'password', maxlen: 60, size: 64, peekaboo: 1, value: nvram.ppp_passwd },
	{ title: 'Service Name', name: 'ppp_service', type: 'text', maxlen: 50, size: 64, value: nvram.ppp_service },
	{ title: 'L2TP Server', name: 'l2tp_server_ip', type: 'text', maxlen: 128, size: 64, value: nvram.l2tp_server_ip },
	{ title: 'Use DHCP', name: 'f_pptp_dhcp', type: 'checkbox', value: (nvram.pptp_dhcp == 1) },
	{ title: 'Use Default Gateway on Remote Network', name: 'f_ppp_defgw', type: 'checkbox', value: (nvram.ppp_defgw == 1) },
	{ title: 'IP Address', name: 'wan_ipaddr', type: 'text', maxlen: 15, size: 17, value: nvram.wan_ipaddr },
	{ title: 'Subnet Mask', name: 'wan_netmask', type: 'text', maxlen: 15, size: 17, value: nvram.wan_netmask },
	{ title: 'Gateway', name: 'wan_gateway', type: 'text', maxlen: 15, size: 17, value: nvram.wan_gateway },
	{ title: 'PPTP Gateway', name: 'pptp_server_ip', type: 'text', maxlen: 128, size: 64, value: nvram.pptp_server_ip },
	{ title: 'Options', name: 'ppp_custom', type: 'text', maxlen: 256, size: 64, value: nvram.ppp_custom },
	{ title: 'Connect Mode', name: 'ppp_demand', type: 'select', options: [['1', 'Connect On Demand'],['0', 'Keep Alive']],
		value: nvram.ppp_demand },
	{ title: 'Max Idle Time', indent: 2, name: 'ppp_idletime', type: 'text', maxlen: 5, size: 7, suffix: ' <i>(minutes)</i>',
		value: nvram.ppp_idletime },
	{ title: 'Check Interval', indent: 2, name: 'ppp_redialperiod', type: 'text', maxlen: 5, size: 7, suffix: ' <i>(seconds)</i>',
		value: nvram.ppp_redialperiod },
	{ title: 'MTU', multi: [
		{ name: 'mtu_enable', type: 'select', options: [['0', 'Default'],['1','Manual']], value: nvram.mtu_enable },
		{ name: 'f_wan_mtu', type: 'text', maxlen: 4, size: 6, value: nvram.wan_mtu } ] },
	{ title: 'Use WAN port for LAN', name: 'f_wan_islan', type: 'checkbox', value: (nvram.wan_islan == 1) }
]);
</script>
</div>

<div class='section-title'>LAN</div>
<div class='section'>
<script type='text/javascript'>
dns = nvram.wan_dns.split(/\s+/);
ipp = nvram.lan_ipaddr.split('.').splice(0, 3).join('.');

createFieldTable('', [
	{ title: 'Router IP Address', name: 'lan_ipaddr', type: 'text', maxlen: 15, size: 17, value: nvram.lan_ipaddr },
	{ title: 'Subnet Mask', name: 'lan_netmask', type: 'text', maxlen: 15, size: 17, value: nvram.lan_netmask },
	{ title: 'Default Gateway', name: 'lan_gateway', type: 'text', maxlen: 15, size: 17, value: nvram.lan_gateway },
	{ title: 'Static DNS', suffix: '&nbsp; <i>(IP:port)</i>', name: 'f_dns_1', type: 'text', maxlen: 21, size: 25, value: dns[0] || '0.0.0.0' },
	{ title: '', name: 'f_dns_2', type: 'text', maxlen: 21, size: 25, value: dns[1] || '0.0.0.0' },
	{ title: '', name: 'f_dns_3', type: 'text', maxlen: 21, size: 25, value: dns[2] || '0.0.0.0' },
	{ title: 'DHCP Server', name: 'f_dhcpd_enable', type: 'checkbox', value: (nvram.lan_proto == 'dhcp') },
	{ title: 'IP Address Range', indent: 2, multi: [
		{ name: 'dhcpd_startip', type: 'text', maxlen: 15, size: 17, value: nvram.dhcpd_startip, suffix: ' - ' },
		{ name: 'dhcpd_endip', type: 'text', maxlen: 15, size: 17, value: nvram.dhcpd_endip, suffix: ' <i id="dhcp_count"></i>' }
	] },

	{ title: 'Lease Time', indent: 2, name: 'dhcp_lease', type: 'text', maxlen: 6, size: 8, suffix: ' <i>(minutes)</i>',
		value: (nvram.dhcp_lease > 0) ? nvram.dhcp_lease : 1440 },
	{ title: 'WINS', indent: 2, name: 'wan_wins', type: 'text', maxlen: 15, size: 17, value: nvram.wan_wins }
]);
</script>
</div>

<script type='text/javascript'>

for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {

var u = wl_unit(uidx);

W('<input type=\'hidden\' id=\'_wl'+u+'_mode\' name=\'wl'+u+'_mode\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_nband\' name=\'wl'+u+'_nband\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_wds_enable\' name=\'wl'+u+'_wds_enable\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_wds\' name=\'wl'+u+'_wds\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_radio\' name=\'wl'+u+'_radio\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_closed\' name=\'wl'+u+'_closed\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_key\' name=\'wl'+u+'_key\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_gmode\' name=\'wl'+u+'_gmode\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_akm\' name=\'wl'+u+'_akm\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_auth\' name=\'wl'+u+'_auth\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_auth_mode\' name=\'wl'+u+'_auth_mode\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_wep\' name=\'wl'+u+'_wep\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_lazywds\' name=\'wl'+u+'_lazywds\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_nmode\' name=\'wl'+u+'_nmode\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_nmcsidx\' name=\'wl'+u+'_nmcsidx\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_nreqd\' name=\'wl'+u+'_nreqd\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_nctrlsb\' name=\'wl'+u+'_nctrlsb\'>');
W('<input type=\'hidden\' id=\'_wl'+u+'_nbw\' name=\'wl'+u+'_nbw\'>');

W('<div class=\'section-title\'>Wireless');
if (wl_ifaces.length > 1)
	W(' (' + wl_display_ifname(uidx) + ')');
W('</div>');

W('<div class=\'section\'>');

f = [
	{ title: 'Enable Wireless', name: 'f_wl'+u+'_radio', type: 'checkbox',
		value: (eval('nvram.wl'+u+'_radio') == '1') && (eval('nvram.wl'+u+'_net_mode') != 'disabled') },
	{ title: 'MAC Address', text: '<a href="advanced-mac.asp">' + eval('nvram.wl'+u+'_hwaddr') + '</a>' },
	{ title: 'Wireless Mode', name: 'f_wl'+u+'_mode', type: 'select',
		options: [['ap', 'Access Point'],['apwds', 'Access Point + WDS'],['sta', 'Wireless Client'],['wet', 'Wireless Ethernet Bridge'],['wds', 'WDS']],
		value: ((eval('nvram.wl'+u+'_mode') == 'ap') && (eval('nvram.wl'+u+'_wds_enable') == '1')) ? 'apwds' : eval('nvram.wl'+u+'_mode') },
	{ title: 'Radio Band', name: 'f_wl'+u+'_nband', type: 'select', options: bands[uidx],
		value: eval('nvram.wl'+u+'_nband') || '0' == '0' ? bands[uidx][0][0] : eval('nvram.wl'+u+'_nband') },
	{ title: 'Wireless Network Mode', name: 'wl'+u+'_net_mode', type: 'select',
		value: (eval('nvram.wl'+u+'_net_mode') == 'disabled') ? 'mixed' : eval('nvram.wl'+u+'_net_mode'),
		options: [], prefix: '<span id="__wl'+u+'_net_mode">', suffix: '</span>' },
	{ title: 'SSID', name: 'wl'+u+'_ssid', type: 'text', maxlen: 32, size: 34, value: eval('nvram.wl'+u+'_ssid') },
	{ title: 'Broadcast', indent: 2, name: 'f_wl'+u+'_bcast', type: 'checkbox', value: (eval('nvram.wl'+u+'_closed') == '0') },
	{ title: 'Channel', name: 'wl'+u+'_channel', type: 'select', options: ghz[uidx], prefix: '<span id="__wl'+u+'_channel">', suffix: '</span> <input type="button" id="_f_wl'+u+'_scan" value="Scan" onclick="scanButton('+u+')"> <img src="spin.gif" id="spin'+u+'">',
		value: eval('nvram.wl'+u+'_channel') },
	{ title: 'Channel Width', name: 'wl'+u+'_nbw_cap', type: 'select', options: [['0','20 MHz'],['1','40 MHz']],
		value: eval('nvram.wl'+u+'_nbw_cap') },
	{ title: 'Control Sideband', name: 'f_wl'+u+'_nctrlsb', type: 'select', options: [['lower','Lower'],['upper','Upper']],
		value: eval('nvram.wl'+u+'_nctrlsb') == 'none' ? 'lower' : eval('nvram.wl'+u+'_nctrlsb') },
	null,
	{ title: 'Security', name: 'wl'+u+'_security_mode', type: 'select',
		options: [['disabled','Disabled'],['wep','WEP'],['wpa_personal','WPA Personal'],['wpa_enterprise','WPA Enterprise'],['wpa2_personal','WPA2 Personal'],['wpa2_enterprise','WPA2 Enterprise'],['wpaX_personal','WPA / WPA2 Personal'],['wpaX_enterprise','WPA / WPA2 Enterprise'],['radius','Radius']],
		value: eval('nvram.wl'+u+'_security_mode') },
	{ title: 'Encryption', indent: 2, name: 'wl'+u+'_crypto', type: 'select',
		options: [['tkip','TKIP'],['aes','AES'],['tkip+aes','TKIP / AES']], value: eval('nvram.wl'+u+'_crypto') },
	{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_wpa_psk', type: 'password', maxlen: 64, size: 66, peekaboo: 1,
		suffix: ' <input type="button" id="_f_wl'+u+'_psk_random1" value="Random" onclick="random_psk(\'_wl'+u+'_wpa_psk\')">',
		value: eval('nvram.wl'+u+'_wpa_psk') },
	{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_radius_key', type: 'password', maxlen: 80, size: 32, peekaboo: 1,
		suffix: ' <input type="button" id="_f_wl'+u+'_psk_random2" value="Random" onclick="random_psk(\'_wl'+u+'_radius_key\')">',
		value: eval('nvram.wl'+u+'_radius_key') },
	{ title: 'Group Key Renewal', indent: 2, name: 'wl'+u+'_wpa_gtk_rekey', type: 'text', maxlen: 4, size: 6, suffix: ' <i>(seconds)</i>',
		value: eval('nvram.wl'+u+'_wpa_gtk_rekey') },
	{ title: 'Radius Server', indent: 2, multi: [
		{ name: 'wl'+u+'_radius_ipaddr', type: 'text', maxlen: 15, size: 17, value: eval('nvram.wl'+u+'_radius_ipaddr') },
		{ name: 'wl'+u+'_radius_port', type: 'text', maxlen: 5, size: 7, prefix: ' : ', value: eval('nvram.wl'+u+'_radius_port') } ] },
	{ title: 'Encryption', indent: 2, name: 'wl'+u+'_wep_bit', type: 'select', options: [['128','128-bits'],['64','64-bits']],
		value: eval('nvram.wl'+u+'_wep_bit') },
	{ title: 'Passphrase', indent: 2, name: 'wl'+u+'_passphrase', type: 'text', maxlen: 16, size: 20,
		suffix: ' <input type="button" id="_f_wl'+u+'_wep_gen" value="Generate" onclick="generate_wep('+u+')"> <input type="button" id="_f_wl'+u+'_wep_random" value="Random" onclick="random_wep('+u+')">',
		value: eval('nvram.wl'+u+'_passphrase') }
];

for (i = 1; i <= 4; ++i)	{
	f.push(
		{ title: ('Key ' + i), indent: 2, name: ('wl'+u+'_key' + i), type: 'text', maxlen: 26, size: 34,
			suffix: '<input type="radio" onchange="verifyFields(this,1)" onclick="verifyFields(this,1)" name="f_wl'+u+'_wepidx" id="_f_wl'+u+'_wepidx_' + i + '" value="' + i + '"' + ((eval('nvram.wl'+u+'_key') == i) ? ' checked>' : '>'),
			value: nvram['wl'+u+'_key' + i] });
}

f.push(null,
	{ title: 'WDS', name: 'f_wl'+u+'_lazywds', type: 'select',
		 options: [['0','Link With...'],['1','Automatic']], value: nvram['wl'+u+'_lazywds'] } );
wds = eval('nvram.wl'+u+'_wds').split(/\s+/);
for (i = 0; i < 10; i += 2)	{
	f.push({ title: (i ? '' : 'MAC Address'), indent: 2, multi: [
		{ name: 'f_wl'+u+'_wds_' + i, type: 'text', maxlen: 17, size: 20, value: wds[i] || '00:00:00:00:00:00' },
		{ name: 'f_wl'+u+'_wds_' + (i + 1), type: 'text', maxlen: 17, size: 20, value: wds[i + 1] || '00:00:00:00:00:00' } ] } );
}

createFieldTable('', f);
W('</div>');

}
// for each wlif
</script>


<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>earlyInit()</script>
<div style='height:100px'></div>
</body>
</html>
