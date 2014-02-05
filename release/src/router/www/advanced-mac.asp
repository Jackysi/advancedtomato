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
<title>[<% ident(); %>] Advanced: MAC Address</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript' src='wireless.jsx?_http_id=<% nv(http_id); %>'></script>
<script type='text/javascript'>

//	<% nvram("et0macaddr,mac_wan,wl_macaddr,wl_hwaddr"); %>

function et0plus(plus)
{
	var mac = nvram.et0macaddr.split(':');
	if (mac.length != 6) return '';
	while (plus-- > 0) {
		for (var i = 5; i >= 3; --i) {
			var n = (parseInt(mac[i], 16) + 1) & 0xFF;
			mac[i] = n.hex(2);
			if (n != 0) break;
		}
	}
	return mac.join(':');
}

function defmac(which)
{
	if (which == 'wan')
		return et0plus(1);
	else {	// wlX
/* REMOVE-BEGIN */
// trying to mimic the behaviour of static int set_wlmac(int idx, int unit, int subunit, void *param) in router/rc/network.c when we have wlX or wlX.X
/* REMOVE-END */
		var u, s, t, v;
		u = which.substr(2, which.length) * 1;
		s = parseInt(u.toString().substr(u.toString().indexOf(".") + 1, u.toString().length) * 1);
		u = parseInt(u.toString().substr(0, u.toString().indexOf(".") - 1) * 1);
		t = et0plus(2 + u + ((s > 0) ? (u * 0x10 + s) : 0)).split(':');
		v = (parseInt(t[0], 16) + ((s > 0) ? (u * 0x10 + 2) : 0) ) & 0xFF;
		t[0] = v.hex(2);
		return t.join(':');
	}
}

function bdefault(which)
{
	E('_f_' + which + '_hwaddr').value = defmac(which);
	verifyFields(null, true);
}

function brand(which)
{
	var mac;
	var i;

	mac = ['00'];
	for (i = 5; i > 0; --i)
		mac.push(Math.floor(Math.random() * 255).hex(2));
	E('_f_' + which + '_hwaddr').value = mac.join(':');
	verifyFields(null, true);
}

function bclone(which)
{
	E('_f_' + which + '_hwaddr').value = '<% compmac(); %>';
	verifyFields(null, true);
}

function findPrevMAC(mac, maxidx)
{
	if (E('_f_wan_hwaddr').value == mac) return 1;

	for (var uidx = 0; uidx < maxidx; ++uidx) {
			if (E('_f_wl'+wl_fface(uidx)+'_hwaddr').value == mac) return 1;
	}

	return 0;
}

function verifyFields(focused, quiet)
{
	var uidx, u, a;

	if (!v_mac('_f_wan_hwaddr', quiet)) return 0;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_fface(uidx);
		a = E('_f_wl'+u+'_hwaddr');
		if (!v_mac(a, quiet)) return 0;

		if (findPrevMAC(a.value, uidx)) {
			ferror.set(a, 'Addresses must be unique', quiet);
			return 0;
		}
	}
	return 1;
}

function save()
{
	var u, uidx, v;

	if (!verifyFields(null, false)) return;
	if (!confirm("Warning: Changing the MAC address may require that you reboot all devices, computers or modem connected to this router. Continue anyway?")) return;

	var fom = E('_fom');
	fom.mac_wan.value = (fom._f_wan_hwaddr.value == defmac('wan')) ? '' : fom._f_wan_hwaddr.value;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_fface(uidx);
		v = E('_f_wl'+u+'_hwaddr').value;
		E('_wl'+u+'_hwaddr').value = (v == defmac('wl' + u)) ? '' : v;
	}

	form.submit(fom, 1);
}

</script>
</head>

<body>
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

<input type='hidden' name='_nextpage' value='advanced-mac.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_service' value='*'>

<input type='hidden' name='mac_wan'>

<script type='text/javascript'>
for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	var u = wl_fface(uidx);
	W('<input type=\'hidden\' id=\'_wl'+u+'_hwaddr\' name=\'wl'+u+'_hwaddr\'>');
}
</script>

<div class='section-title'>MAC Address</div>
<div class='section'>
<script type='text/javascript'>

f = [
	{ title: 'WAN Port', indent: 1, name: 'f_wan_hwaddr', type: 'text', maxlen: 17, size: 20,
		suffix: ' <input type="button" value="Default" onclick="bdefault(\'wan\')"> <input type="button" value="Random" onclick="brand(\'wan\')"> <input type="button" value="Clone PC" onclick="bclone(\'wan\')">',
		value: nvram.mac_wan || defmac('wan') }
];

for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	var u = wl_fface(uidx);
	f.push(
		{ title: 'Wireless Interface ' + ((wl_ifaces.length > 1) ? wl_ifaces[uidx][0] : ''), indent: 1, name: 'f_wl'+u+'_hwaddr', type: 'text', maxlen: 17, size: 20,
			suffix:' <input type="button" value="Default" onclick="bdefault(\'wl'+u+'\')"> <input type="button" value="Random" onclick="brand(\'wl'+u+'\')"> <input type="button" value="Clone PC" onclick="bclone(\'wl'+u+'\')">',
			value: nvram['wl'+u+'_hwaddr'] || defmac('wl' + u) }
		);
}

createFieldTable('', f);

</script>
<br>
<table border=0 cellpadding=1>
	<tr><td>Router's LAN MAC Address:</td><td><b><script type='text/javascript'>W(('<% nv('et0macaddr'); %>').toUpperCase());</script></b></td></tr>
	<tr><td>Computer's MAC Address:</td><td><b><script type='text/javascript'>W(('<% compmac(); %>').toUpperCase());</script></b></td></tr>
</table>
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
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
