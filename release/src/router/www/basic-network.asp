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

<style type='text/css'>
#spin {
	visibility: hidden;
	vertical-align: middle;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript' src='md5.js'></script>
<script type='text/javascript'>
//	<% nvram("dhcp_lease,dhcp_num,dhcp_start,dhcpd_startip,dhcpd_endip,l2tp_server_ip,lan_gateway,lan_ipaddr,lan_netmask,lan_proto,mtu_enable,ppp_demand,ppp_idletime,ppp_passwd,ppp_redialperiod,ppp_service,ppp_username,pptp_server_ip,pptp_dhcp,security_mode2,wan_dns,wan_gateway,wan_ipaddr,wan_mtu,wan_netmask,wan_proto,wan_wins,wds_enable,wl_channel,wl_closed,wl_crypto,wl_key,wl_key1,wl_key2,wl_key3,wl_key4,wl_lazywds,wl_mode,wl_net_mode,wl_passphrase,wl_radio,wl_radius_ipaddr,wl_radius_port,wl_ssid,wl_wds,wl_wep_bit,wl_wpa_gtk_rekey,wl_wpa_psk,wl_radius_key,wds_save,wl_auth,wl0_hwaddr,wan_islan,t_features,wl_nbw_cap,wl_nctrlsb,wl_nband"); %>

xob = null;

wl_channels = [];
ghz = [];

if ((!fixIP(nvram.dhcpd_startip)) || (!fixIP(nvram.dhcpd_endip))) {
	var x = nvram.lan_ipaddr.split('.').splice(0, 3).join('.') + '.';
	nvram.dhcpd_startip = x + nvram.dhcp_start;
	nvram.dhcpd_endip = x + ((nvram.dhcp_start * 1) + (nvram.dhcp_num * 1) - 1);
}

var nphy = features('11n');
var dualband = features('2g5g');
var modes = [];
var nm_loaded = 0;
var ch_loaded = 0;
var max_channel = 0;

function refreshNetModes()
{
	var e, i, buf, val, band5;

	if (dualband) {
		e = E('_wl_nband');
		band5 = (e.value + '' == '' ? nvram.wl_nband : e.value) == '1' ? true : false;
	} else
		band5 = false;

	modes = [['mixed','Auto']];
	if (band5) {
		// ?? modes.push(['a-only','A Only']);
		modes.push(['n-only','N Only']);
	}
	else {
		modes.push(['b-only','B Only']);
		modes.push(['g-only','G Only']);
		if (nphy) {
			modes.push(['bg-mixed','B/G Mixed']);
			modes.push(['n-only','N Only']);
		}
	}

	e = E('_wl_net_mode');
	buf = '';
	val = (!nm_loaded || (e.value + '' == '')) ? nvram.wl_net_mode : e.value;
	if (val == 'disabled') val = 'mixed';
	for (i = 0; i < modes.length; ++i)
		buf += '<option value="' + modes[i][0] + '"' + ((modes[i][0] == val) ? ' selected' : '') + '>' + modes[i][1] + '</option>';

	e = E('__wl_net_mode');
	buf = '<select name="wl_net_mode" onchange="verifyFields(this, 1)" id = "_wl_net_mode">' + buf + '</select>';
	elem.setInnerHTML(e, buf);
	nm_loaded = 1;
}

var refresher = null;

function refreshChannels()
{
	if (refresher != null) return;

	refresher = new XmlHttp();
	refresher.onCompleted = function(text, xml) {

		var e, i, buf, val;

		eval(text);
		ghz = [];
		for (i = 0; i < wl_channels.length; ++i) {
			ghz.push([wl_channels[i][0] + '',
				(wl_channels[i][0]) ? ((wl_channels[i][1]) ? wl_channels[i][0] + ' - ' + (wl_channels[i][1] / 1000.0).toFixed(3) + ' GHz' : wl_channels[i][0] + '') : 'Auto']);
			max_channel = wl_channels[i][0] * 1;
		}

		e = E('_wl_channel');
		buf = '';
		val = (!ch_loaded || (e.value + '' == '')) ? nvram.wl_channel : e.value;
		for (i = 0; i < ghz.length; ++i)
			buf += '<option value="' + ghz[i][0] + '"' + ((ghz[i][0] == val) ? ' selected' : '') + '>' + ghz[i][1] + '</option>';

		e = E('__wl_channel');
		buf = '<select name="wl_channel" onchange="verifyFields(this, 1)" id = "_wl_channel">' + buf + '</select>';
		elem.setInnerHTML(e, buf);
		ch_loaded = 1;

		refresher = null;
		verifyFields(null, 1);
	}

	var band, bw, sb, e;

	if (dualband) {
		e = E('_wl_nband');
		band = (e.value + '' == '' ? nvram.wl_nband : e.value);
	} else
		band = '0';
	e = E('_f_wl_nctrlsb');
	sb = (e.value + '' == '' ? nvram.wl_nctrlsb : e.value);
	e = E('_wl_nbw_cap');
	bw = (e.value + '' == '' ? nvram.wl_nbw_cap : e.value) == '0' ? '20' : '40';

	refresher.onError = function(ex) { alert(ex); reloadPage(); }
	refresher.post('update.cgi', 'exec=wlchannels&arg0=' + (nphy ? '1' : '0') +
		'&arg1=' + bw + '&arg2=' + band + '&arg3=' + sb);
}

function spin(x)
{
	var e = E('_f_scan');
	e.disabled = x;
	if (x) e.value = 'Scan ' + (wscan.tries + 1);
		else e.value = 'Scan';
	E('spin').style.visibility = x ? 'visible' : 'hidden';
}

function scan()
{
	if (xob) return;

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
			var e = E('_wl_channel');
			for (i = 1; i < ghz.length; ++i) {
				var s = ghz[i][1];
				var u = wscan.inuse[ghz[i][0]];
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
		spin(0);
	}
	xob.onError = function(x) {
		alert('error: ' + x);
		spin(0);
		xob = null;
	}

	spin(1);
	xob.post('update.cgi', 'exec=wlscan');
}

function scanButton()
{
	if (xob) return;

	wscan = {
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

function random_wep()
{
	E('_wl_passphrase').value = random_x(16);
	generate_wep();
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
function generate_wep()
{
	function _wepgen(pass, i)
	{
		while (pass.length < 64) pass += pass;
		return hex_md5(pass.substr(0, 64)).substr(i, (E('_wl_wep_bit').value == 128) ? 26 : 10);
	}

	var e = E('_wl_passphrase');
	var pass = e.value;
	if (!v_length(e, false, 3)) return;
	E('_wl_key1').value = _wepgen(pass, 0);
	pass += '#$%';
	E('_wl_key2').value = _wepgen(pass, 2);
	pass += '!@#';
	E('_wl_key3').value = _wepgen(pass, 4);
	pass += '%&^';
	E('_wl_key4').value = _wepgen(pass, 6);
	verifyFields(null, 1);
}

function verifyFields(focused, quiet)
{
	var i;
	var ok = 1;
	var a, b, c, d, e;

	if (focused == E('_wl_nband')) {
		refreshNetModes();
		refreshChannels();
	}
	else if (focused == E('_f_wl_nctrlsb') || focused == E('_wl_nbw_cap')) {
		refreshChannels();
	}

	// --- visibility ---

	var vis = {
		_wan_proto: 1,
		_ppp_username: 1,
		_ppp_passwd: 1,
		_ppp_service: 1,
		_l2tp_server_ip: 1,
		_wan_ipaddr: 1,
		_wan_netmask: 1,
		_wan_gateway: 1,
		_pptp_server_ip: 1,
		_f_pptp_dhcp: 1,
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
		_wan_wins: 1,

		_f_wl_radio: 1,
		_f_wmode: 1,
		_wl_nband: (nphy && dualband) ? 1 : 0,
		_wl_net_mode: 1,
		_wl_ssid: 1,
		_f_bcast: 1,
		_wl_channel: 1,
		_wl_nbw_cap: nphy ? 1 : 0,
		_f_wl_nctrlsb: nphy ? 1 : 0,
		_f_scan: 1,

		_security_mode2: 1,
		_wl_crypto: 1,
		_wl_wpa_psk: 1,
		_f_psk_random1: 1,
		_f_psk_random2: 1,
		_wl_wpa_gtk_rekey: 1,
		_wl_radius_key: 1,
		_wl_radius_ipaddr: 1,
		_wl_radius_port: 1,
		_wl_wep_bit: 1,
		_wl_passphrase: 1,
		_f_wep_gen: 1,
		_f_wep_random: 1,
		_wl_key1: 1,
		_wl_key2: 1,
		_wl_key3: 1,
		_wl_key4: 1,

		_f_wl_lazywds: 1,
		_f_wds_0: 1
	};

	var wan = E('_wan_proto').value;
	var wmode = E('_f_wmode').value;

	if (wmode == 'wet') {
		wan = 'disabled';
		vis._wan_proto = 0;
		vis._f_dhcpd_enable = 0;
		vis._dhcp_lease = 0;
	}
	
	if ((wan == 'disabled') || (wmode == 'sta') || (wmode == 'wet')) {
		vis._f_wan_islan = 1;
	}

	switch (wan) {
	case 'disabled':
		vis._ppp_username = 0;
		vis._ppp_service = 0;
		vis._l2tp_server_ip = 0;
		vis._wan_ipaddr = 0;
		vis._wan_netmask = 0;
		vis._wan_gateway = 0;
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;
		vis._ppp_demand = 0;
		vis._mtu_enable = 0;
		vis._f_wan_mtu = 0;
		break;
	case 'dhcp':
		vis._l2tp_server_ip = 0;
		vis._ppp_demand = 0;
		vis._ppp_service = 0;
		vis._ppp_username = 0;
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;
		vis._wan_gateway = 0;
		vis._wan_ipaddr = 0;
		vis._wan_netmask = 0;

		vis._lan_gateway = 0;
		break;
	case 'pppoe':
		vis._l2tp_server_ip = 0;
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;
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
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;

		vis._lan_gateway = 0;
		break;
	case 'pptp':
		vis._l2tp_server_ip = 0;
		vis._ppp_service = 0;
		vis._wan_gateway = 0;
		vis._wan_ipaddr = (!E('_f_pptp_dhcp').checked);
		vis._wan_netmask = vis._wan_ipaddr;

		vis._lan_gateway = 0;
		break;
	case 'l2tp':
		vis._ppp_service = 0;
		vis._pptp_server_ip = 0;
		vis._f_pptp_dhcp = 0;
		vis._wan_gateway = 0;
		vis._wan_ipaddr = 0;
		vis._wan_netmask = 0;

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

	if (!E('_f_wl_radio').checked) {
		vis._f_wl_lazywds = 2;
		vis._f_wds_0 = 2;
		vis._f_wmode = 2;
		vis._security_mode2 = 2;
		vis._wl_channel = 2;
		vis._wl_nbw_cap = nphy ? 2 : 0;
		vis._wl_nband = (nphy && dualband) ? 2 : 0;
		vis._f_bcast = 2;
		vis._wl_crypto = 2;
		vis._wl_net_mode = 2;
		vis._wl_wpa_psk = 2;
		vis._wl_radius_key = 2;
		vis._wl_wpa_gtk_rekey = 2;
		vis._wl_radius_ipaddr = 2;
		vis._wl_ssid = 2;
		vis._wl_wep_bit = 2;
	}

	switch (wmode) {
	case 'apwds':
	case 'wds':
		break;
	case 'wet':
	case 'sta':
		vis._f_bcast = 0;
		vis._wl_channel = 0;
		vis._wl_nbw_cap = 0;
	default:
		vis._f_wl_lazywds = 0;
		vis._f_wds_0 = 0;
		break;
	}

	var sm2 = E('_security_mode2').value;
	switch (sm2) {
	case 'disabled':
		vis._wl_crypto = 0;
		vis._wl_wep_bit = 0;
		vis._wl_wpa_psk = 0;
		vis._wl_radius_key = 0;
		vis._wl_radius_ipaddr = 0;
		vis._wl_wpa_gtk_rekey = 0;
		break;
	case 'wep':
		vis._wl_crypto = 0;
		vis._wl_wpa_psk = 0;
		vis._wl_radius_key = 0;
		vis._wl_radius_ipaddr = 0;
		vis._wl_wpa_gtk_rekey = 0;
		break;
	case 'radius':
		vis._wl_crypto = 0;
		vis._wl_wpa_psk = 0;
		break;
	default:	// wpa*
		vis._wl_wep_bit = 0;
		if (sm2.indexOf('personal') != -1) {
			vis._wl_radius_key = 0;
			vis._wl_radius_ipaddr = 0;
		}
		else {
			vis._wl_wpa_psk = 0;
		}
		break;
	}

	if ((E('_f_wl_lazywds').value == 1) && (vis._f_wds_0 == 1)) {
		vis._f_wds_0 = 2;
	}

	if (vis._wl_nbw_cap != 0) {
		switch (E('_wl_net_mode').value) {
		case 'b-only':
		case 'g-only':
		case 'a-only':
		case 'bg-mixed':
			vis._wl_nbw_cap = 2;
			if (E('_wl_nbw_cap').value != '0') {
				E('_wl_nbw_cap').value = 0;
				refreshChannels();
			}
			break;
		}
		// avoid Enterprise-TKIP with 40MHz
		if ((sm2 == 'wpa_enterprise') && (E('_wl_crypto').value == 'tkip')) {
			vis._wl_nbw_cap = 2;
			if (E('_wl_nbw_cap').value != '0') {
				E('_wl_nbw_cap').value = 0;
				refreshChannels();
			}
		}
	}

	vis._f_wl_nctrlsb = (E('_wl_nbw_cap').value == 0) ? 0 : vis._wl_nbw_cap;

/* REMOVE-BEGIN
	// This is ugly...
	// Special case - 2.4GHz band, currently running in B/G-only mode,
	// with N/Auto and 40MHz selected in the GUI.
	// Channel list is not filtered in this case by the wl driver,
	// and includes all channels available with 20MHz channel width.
	//
REMOVE-END */
	if (vis._wl_channel == 1 && vis._f_wl_nctrlsb != 0 &&
	   (vis._wl_nband == 0 || E('_wl_nband').value == '2')) {
		switch (nvram.wl_net_mode) {
		case 'b-only':
		case 'g-only':
		case 'bg-mixed':
			i = E('_wl_channel').value * 1;
			if (i > 0 && i < 5) {
				E('_f_wl_nctrlsb').value = 'lower';
				vis._f_wl_nctrlsb = 2;
			}
			else if (i > max_channel - 4) {
				E('_f_wl_nctrlsb').value = 'upper';
				vis._f_wl_nctrlsb = 2;
			}
			break;
		}
	}

	//

	vis._ppp_passwd = vis._ppp_username;
	vis._dhcpd_startip = vis._dhcpd_endip = vis._wan_wins = vis._dhcp_lease;
	vis._f_scan = vis._wl_channel;
	vis._f_psk_random1 = vis._wl_wpa_psk;
	vis._f_psk_random2 = vis._wl_radius_key;
	vis._wl_radius_port = vis._wl_radius_ipaddr;
	vis._wl_key1 = vis._wl_key2 = vis._wl_key3 = vis._wl_key4 = vis._f_wep_gen = vis._f_wep_random = vis._wl_passphrase = vis._wl_wep_bit;

	for (i = 1; i < 10; ++i) {
		vis['_f_wds_' + i] = vis._f_wds_0;
	}
	for (a in vis) {
		b = E(a);
		c = vis[a];
		b.disabled = (c != 1);
		PR(b).style.display = c ? '' : 'none';
	}


	// --- verify ---

	if (wmode == 'sta') {
		if ((wan == 'disabled') && (E('_f_wl_radio').checked)) {
			ferror.set('_wan_proto', 'Wireless Client mode requires a valid WAN setting (usually DHCP).', quiet);
			return 0;
		}
	}
	ferror.clear('_wan_proto');

/* REMOVE-BEGIN
	if ((vis._f_wmode == 1) && (wmode != 'ap') && (sm2.substr(0, 4) == 'wpa2')) {
		ferror.set('_security_mode2', 'WPA2 is supported only in AP mode.', quiet);
		return 0;
	}
	ferror.clear('_security_mode2');
REMOVE-END */

	// N standard does not support WPA+TKIP
	switch (E('_wl_net_mode').value) {
	case 'mixed':
	case 'n-only':
		if (nphy && (E('_wl_crypto').value == 'tkip') &&
		   ((sm2 == 'wpa_enterprise') || (sm2 == 'wpa_personal'))) {
			ferror.set('_wl_crypto', 'WPA with TKIP encryption is not supported in N mode.', quiet);
			return 0;
		}
		break;
	}

	if (((wmode == 'sta') || (wmode == 'wet')) &&
	    (E('_wl_net_mode').value == 'n-only')) {
		ferror.set('_wl_net_mode', 'N-only is not supported in wireless client modes.', quiet);
		return 0;
	}

	a = E('_wl_wpa_psk');
	ferror.clear(a);
	if (vis._wl_wpa_psk == 1) {
		if ((a.value.length < 8) || ((a.value.length == 64) && (a.value.search(/[^0-9A-Fa-f]/) != -1))) {
			ferror.set('_wl_wpa_psk', 'Invalid pre-shared key. Please enter at least 8 characters or 64 hexadecimal digits.', quiet);
			ok = 0;
		}
	}

	// wl channel
	if (((wmode == 'wds') || (wmode == 'apwds')) && (vis._wl_channel == 1) && (E('_wl_channel').value == '0')) {
		ferror.set('_wl_channel', 'Fixed wireless channel required in WDS mode.', quiet);
		ok = 0;
	}
	else ferror.clear('_wl_channel');

	// domain name or IP address
	a = ['_l2tp_server_ip', '_pptp_server_ip'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && ((!v_length(a[i], 1, 1)) || ((!v_ip(a[i], 1)) && (!v_domain(a[i], 1))))) {
			ok = 0;
			if (!quiet) ferror.show(a[i]);
		}

	// IP address
	a = ['_wan_gateway','_wan_ipaddr','_lan_ipaddr', '_wl_radius_ipaddr', '_dhcpd_startip', '_dhcpd_endip'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_ip(a[i], quiet))) ok = 0;

	// IP address, blank -> 0.0.0.0
	a = ['_f_dns_1', '_f_dns_2', '_f_dns_3','_wan_wins','_lan_gateway'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_dns(a[i], quiet))) ok = 0;

	// netmask
	a = ['_wan_netmask','_lan_netmask'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_netmask(a[i], quiet))) ok = 0;

	// range
	a = [['_ppp_idletime', 3, 1440],['_ppp_redialperiod', 1, 86400],['_f_wan_mtu', 576, 1500],
		 ['_dhcp_lease', 1, 10080],['_wl_wpa_gtk_rekey', 60, 7200], ['_wl_radius_port', 1, 65535]];
	for (i = a.length - 1; i >= 0; --i) {
		v = a[i];
		if ((vis[v[0]]) && (!v_range(v[0], quiet, v[1], v[2]))) ok = 0;
	}

	// length
	a = [['_wl_ssid', 1], ['_wl_radius_key', 1]];
	for (i = a.length - 1; i >= 0; --i) {
		v = a[i];
		if ((vis[v[0]]) && (!v_length(v[0], quiet, v[1], E(v[0]).maxlength))) ok = 0;
	}

	if (vis._wl_key1) {
		a = (E('_wl_wep_bit').value == 128) ? 26 : 10;
		for (i = 1; i <= 4; ++i) {
			b = E('_wl_key' + i);
			b.maxLength = a;
			if ((b.value.length > 0) || (E('_f_wepidx_' + i).checked)) {
				if (!v_wep(b, quiet)) ok = 0;
			}
			else ferror.clear(b);
		}
	}

	ferror.clear('_f_wds_0');
	if (vis._f_wds_0 == 1) {
		b = 0;
		for (i = 0; i < 10; ++i) {
			a = E('_f_wds_' + i);
			if (!v_macz(a, quiet)) ok = 0;
				else if (!isMAC0(a.value)) b = 1;
		}
		if (!b) {
			ferror.set('_f_wds_0', 'WDS MAC address required.', quiet);
			ok = 0;
		}
	}

	a = E('_dhcpd_startip');
	b = E('_dhcpd_endip');

	if ((!a._error_msg) && (!b._error_msg)) {
		c = aton(E('_lan_netmask').value);
		d = aton(E('_lan_ipaddr').value) & c;
		e = 'Invalid IP address or subnet mask';
		if ((aton(a.value) & c) != d) {
			ferror.set(a, e, quiet);
			ok = 0;
		}
		if ((aton(b.value) & c) != d) {
			ferror.set(b, e, quiet);
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

	if (!verifyFields(null, false)) return;

	var fom = E('_fom');
	var wmode = fom.f_wmode.value;
	var sm2 = fom.security_mode2.value;
	var wradio = fom.f_wl_radio.checked;

	fom.lan_proto.value = fom.f_dhcpd_enable.checked ? 'dhcp' : 'static';

	fom.wan_mtu.value = fom.f_wan_mtu.value;
	fom.wan_mtu.disabled = fom.f_wan_mtu.disabled;

	if (wmode == 'apwds') fom.wl_mode.value = 'ap';
		else fom.wl_mode.value = wmode;

	if (wmode == 'wet') {
		fom.wan_proto.value = 'disabled';
		fom.wan_proto.disabled = 0;
		fom.lan_proto.value = 'static';
	}
	
	fom.wan_islan.value = fom.f_wan_islan.checked ? 1 : 0;
	fom.pptp_dhcp.value = fom.f_pptp_dhcp.checked ? 1 : 0;

	a = [];
	for (i = 0; i < 10; ++i) a.push(E('_f_wds_' + i).value);
	fom.wl_wds.value = joinAddr(a);

	fom.wds_save.value = nvram.wds_save;
	if (wmode.indexOf('wds') != -1) {
        fom.wds_enable.value = 1;
		fom.wl_lazywds.value = fom.f_wl_lazywds.value;
		if (fom.wl_lazywds.value == 1) fom.wl_wds.value = '';
			else fom.wds_save.value = fom.wl_wds.value;
	}
	else {
		fom.wds_enable.value = 0;
		fom.wl_wds.value = '';
		fom.wl_lazywds.value = 0;
	}

	fom.wan_dns.value = joinAddr([fom.f_dns_1.value, fom.f_dns_2.value, fom.f_dns_3.value])

	fom.wl_radio.value = fom.f_wl_radio.checked ? 1 : 0;
	fom.wl_radio.disabled = fom.f_wl_radio.disabled;

	fom.wl_auth.value = nvram.wl_auth;

	switch (sm2) {
	case 'disabled':
	case 'radius':
	case 'wep':
		fom.security_mode.value = sm2;
		fom.wl_akm.value = '';
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
		fom.security_mode.value = c;
		fom.wl_akm.value = c;
		break;
	}
	fom.wl_auth_mode.value = (sm2 == 'radius') ? 'radius' : 'none';
	fom.wl_wep.value = ((sm2 == 'radius') || (sm2 == 'wep')) ? 'enabled': 'disabled';
	fom.wl_auth_mode.disabled = fom.wl_wep.disabled = fom.security_mode.disabled = fom.wl_akm.disabled = fom.security_mode2.disabled;

	if (sm2.indexOf('wpa') != -1) fom.wl_auth.value = 0;

	fom.wl_nreqd.value = 0;
	fom.wl_gmode.value = 1;
	fom.wl_nmode.value = 0;
	fom.wl_nreqd.value = 0;
	fom.wl_nmcsidx.value = -2; // Legacy Rate
	fom.wl_nbw.value = 0;
	switch (fom.wl_net_mode.value) {
	case 'b-only':
		fom.wl_gmode.value = 0;
		break;
	case 'g-only':
		fom.wl_gmode.value = 4;
		break;
	case 'bg-mixed':
		break;
	case 'a-only':
		// ??
		break;
	case 'n-only':
		fom.wl_nmode.value = 1;
		fom.wl_nmcsidx.value = 32;
		fom.wl_nreqd.value = 1;
		break;
	default: // Auto
		fom.wl_nmode.value = -1;
		fom.wl_nmcsidx.value = -1;
		break;
	}

	fom.wl_nctrlsb.value = nvram.wl_nctrlsb;
	if (fom.wl_nmode.value != 0) {
		i = fom.wl_channel.value * 1;
		fom.wl_nctrlsb.value = fom.f_wl_nctrlsb.value;
		fom.wl_nbw.value = (fom.wl_nbw_cap.value == 0) ? 20 : 40;
	}

	fom.wl_gmode.disabled = fom.wl_net_mode.disabled;

	fom.wl_closed.value = fom.f_bcast.checked ? 0 : 1;
	fom.wl_closed.disabled = fom.f_bcast.disabled;

	a = fields.radio.selected(fom.f_wepidx);
	if (a) fom.wl_key.value = a.value;
	fom.wl_key.disabled = fom.wl_key1.disabled;

	fom.wl_mode.disabled = fom.wl_wds.disabled = fom.wds_enable.disabled = !fom.f_wl_radio.checked;

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
	refreshNetModes();
	refreshChannels();
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
<input type='hidden' name='wl_mode'>
<input type='hidden' name='wds_enable'>
<input type='hidden' name='wl_wds'>
<input type='hidden' name='wds_save'>
<input type='hidden' name='lan_proto'>
<input type='hidden' name='wan_dns'>
<input type='hidden' name='wl_radio'>
<input type='hidden' name='wl_closed'>
<input type='hidden' name='wl_key'>
<input type='hidden' name='wl_gmode'>
<input type='hidden' name='security_mode'>
<input type='hidden' name='wl_akm'>
<input type='hidden' name='wl_auth'>
<input type='hidden' name='wl_auth_mode'>
<input type='hidden' name='wl_wep'>
<input type='hidden' name='wl_lazywds'>
<input type='hidden' name='wl_nmode'>
<input type='hidden' name='wl_nmcsidx'>
<input type='hidden' name='wl_nreqd'>
<input type='hidden' name='wl_nctrlsb'>
<input type='hidden' name='wl_nbw'>


<div class='section-title'>WAN / Internet</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Type', name: 'wan_proto', type: 'select', options: [['dhcp','DHCP'],['pppoe','PPPoE'],['static','Static'],['pptp','PPTP'],['l2tp','L2TP'],['disabled','Disabled']],
		value: nvram.wan_proto },
	{ title: 'Username', name: 'ppp_username', type: 'text', maxlen: 60, size: 64, value: nvram.ppp_username },
	{ title: 'Password', name: 'ppp_passwd', type: 'password', maxlen: 60, size: 64, peekaboo: 1, value: nvram.ppp_passwd },
	{ title: 'Service Name', name: 'ppp_service', type: 'text', maxlen: 50, size: 54, value: nvram.ppp_service },
	{ title: 'L2TP Server', name: 'l2tp_server_ip', type: 'text', maxlen: 128, size: 54, value: nvram.l2tp_server_ip },
	{ title: 'Use DHCP', name: 'f_pptp_dhcp', type: 'checkbox', value: (nvram.pptp_dhcp == 1) },
	{ title: 'IP Address', name: 'wan_ipaddr', type: 'text', maxlen: 15, size: 17, value: nvram.wan_ipaddr },
	{ title: 'Subnet Mask', name: 'wan_netmask', type: 'text', maxlen: 15, size: 17, value: nvram.wan_netmask },
	{ title: 'Gateway', name: 'wan_gateway', type: 'text', maxlen: 15, size: 17, value: nvram.wan_gateway },
	{ title: 'Gateway', name: 'pptp_server_ip', type: 'text', maxlen: 128, size: 54, value: nvram.pptp_server_ip },
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


<div class='section-title'>Wireless</div>
<div class='section'>
<script type='text/javascript'>
f = [
	{ title: 'Enable Wireless', name: 'f_wl_radio', type: 'checkbox',
		value: (nvram.wl_radio == '1') && (nvram.wl_net_mode != 'disabled') },
	{ title: 'MAC Address', text: '<a href="advanced-mac.asp">' + nvram.wl0_hwaddr + '</a>' },
	{ title: 'Wireless Mode', name: 'f_wmode', type: 'select',
		options: [['ap', 'Access Point'],['apwds', 'Access Point + WDS'],['sta', 'Wireless Client'],['wet', 'Wireless Ethernet Bridge'],['wds', 'WDS']],
		value: ((nvram.wl_mode == 'ap') && (nvram.wds_enable == '1')) ? 'apwds' : nvram.wl_mode },
	{ title: 'Radio Band', name: 'wl_nband', type: 'select', options: [['2','2.4 GHz'],['1','5 GHz']],
		value: nvram.wl_nband == '0' ? '2' : nvram.wl_nband },
	{ title: 'Wireless Network Mode', name: 'wl_net_mode', type: 'select', value: (nvram.wl_net_mode == 'disabled') ? 'mixed' : nvram.wl_net_mode, options: modes, prefix: '<span id="__wl_net_mode">', suffix: '</span>' },
	{ title: 'SSID', name: 'wl_ssid', type: 'text', maxlen: 32, size: 34, value: nvram.wl_ssid },
	{ title: 'Broadcast', indent: 2, name: 'f_bcast', type: 'checkbox', value: (nvram.wl_closed == '0') },
	{ title: 'Channel', name: 'wl_channel', type: 'select', options: ghz, prefix: '<span id="__wl_channel">', suffix: '</span> <input type="button" id="_f_scan" value="Scan" onclick="scanButton()"> <img src="spin.gif" id="spin">',
		value: nvram.wl_channel },
	{ title: 'Channel Width', name: 'wl_nbw_cap', type: 'select', options: [['0','20 MHz'],['1','40 MHz']],
		value: nvram.wl_nbw_cap },
	{ title: 'Control Sideband', name: 'f_wl_nctrlsb', type: 'select', options: [['lower','Lower'],['upper','Upper']],
		value: nvram.wl_nctrlsb == 'none' ? 'lower' : nvram.wl_nctrlsb },
	null,
	{ title: 'Security', name: 'security_mode2', type: 'select',
		options: [['disabled','Disabled'],['wep','WEP'],['wpa_personal','WPA Personal'],['wpa_enterprise','WPA Enterprise'],['wpa2_personal','WPA2 Personal'],['wpa2_enterprise','WPA2 Enterprise'],['wpaX_personal','WPA / WPA2 Personal'],['wpaX_enterprise','WPA / WPA2 Enterprise'],['radius','Radius']],
		value: nvram.security_mode2 },
	{ title: 'Encryption', indent: 2, name: 'wl_crypto', type: 'select',
		options: [['tkip','TKIP'],['aes','AES'],['tkip+aes','TKIP / AES']], value: nvram.wl_crypto },
	{ title: 'Shared Key', indent: 2, name: 'wl_wpa_psk', type: 'password', maxlen: 64, size: 66, peekaboo: 1, 
		suffix: ' <input type="button" id="_f_psk_random1" value="Random" onclick="random_psk(\'_wl_wpa_psk\')">',
		value: nvram.wl_wpa_psk },
	{ title: 'Shared Key', indent: 2, name: 'wl_radius_key', type: 'password', maxlen: 80, size: 32, peekaboo: 1,
		suffix: ' <input type="button" id="_f_psk_random2" value="Random" onclick="random_psk(\'_wl_radius_key\')">',
		value: nvram.wl_radius_key },
	{ title: 'Group Key Renewal', indent: 2, name: 'wl_wpa_gtk_rekey', type: 'text', maxlen: 4, size: 6, suffix: ' <i>(seconds)</i>',
		value: nvram.wl_wpa_gtk_rekey },
	{ title: 'Radius Server', indent: 2, multi: [
		{ name: 'wl_radius_ipaddr', type: 'text', maxlen: 15, size: 17, value: nvram.wl_radius_ipaddr },
		{ name: 'wl_radius_port', type: 'text', maxlen: 5, size: 7, prefix: ' : ', value: nvram.wl_radius_port } ] },
	{ title: 'Encryption', indent: 2, name: 'wl_wep_bit', type: 'select', options: [['128','128-bits'],['64','64-bits']],
		value: nvram.wl_wep_bit },
	{ title: 'Passphrase', indent: 2, name: 'wl_passphrase', type: 'text', maxlen: 16, size: 20,
		suffix: ' <input type="button" id="_f_wep_gen" value="Generate" onclick="generate_wep()"> <input type="button" id="_f_wep_random" value="Random" onclick="random_wep()">',
		value: nvram.wl_passphrase }
];

for (i = 1; i <= 4; ++i)	{
	f.push(
		{ title: ('Key ' + i), indent: 2, name: ('wl_key' + i), type: 'text', maxlen: 26, size: 34,
			suffix: '<input type="radio" onchange="verifyFields(this,1)" onclick="verifyFields(this,1)" name="f_wepidx" id="_f_wepidx_' + i + '" value="' + i + '"' + ((nvram.wl_key == i) ? ' checked>' : '>'),
			value: nvram['wl_key' + i] });
}

f.push(null,
	{ title: 'WDS', name: 'f_wl_lazywds', type: 'select',
		 options: [['0','Link With...'],['1','Automatic']], value: nvram.wl_lazywds } );
wds = ((nvram.wl_wds == '') ? nvram.wds_save : nvram.wl_wds).split(/\s+/);
for (i = 0; i < 10; i += 2)	{
	f.push({ title: (i ? '' : 'MAC Address'), indent: 2, multi: [
		{ name: 'f_wds_' + i, type: 'text', maxlen: 17, size: 20, value: wds[i] || '00:00:00:00:00:00' },
		{ name: 'f_wds_' + (i + 1), type: 'text', maxlen: 17, size: 20, value: wds[i + 1] || '00:00:00:00:00:00' } ] } );
}

createFieldTable('', f);
</script>
</div>

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
