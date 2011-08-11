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
<title>[<% ident(); %>] <% translate("Advanced"); %>: <% translate("MAC Address"); %></title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
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
		var u = which.substr(2, which.length) * 1;
		return et0plus(2 + u);
	}
}

function bdefault(which)
{
	E('_f_' + which + '_macaddr').value = defmac(which);
	verifyFields(null, true);
}

function brand(which)
{
	var mac;
	var i;

	mac = ['00'];
	for (i = 5; i > 0; --i)
		mac.push(Math.floor(Math.random() * 255).hex(2));
	E('_f_' + which + '_macaddr').value = mac.join(':');
	verifyFields(null, true);
}

function bclone(which)
{
	E('_f_' + which + '_macaddr').value = '<% compmac(); %>';
	verifyFields(null, true);
}

function findPrevMAC(mac, maxidx)
{
	if (E('_f_wan_macaddr').value == mac) return 1;

	for (var uidx = 0; uidx < maxidx; ++uidx) {
		if (E('_f_wl'+wl_unit(uidx)+'_macaddr').value == mac) return 1;
	}

	return 0;
}

function verifyFields(focused, quiet)
{
	var uidx, u, a;

	if (!v_mac('_f_wan_macaddr', quiet)) return 0;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);
		a = E('_f_wl'+u+'_macaddr');
		if (!v_mac(a, quiet)) return 0;

		if (findPrevMAC(a.value, uidx)) {	
			ferror.set(a, '<% translate("Addresses must be unique"); %>', quiet);
			return 0;
		}
	}

	return 1;
}

function save()
{
	var u, uidx, v;

	if (!verifyFields(null, false)) return;
	if (!confirm("<% translate("Warning: Changing the MAC address may require that you reboot all devices, computers or modem connected to this router. Continue anyway"); %>?")) return;

	var fom = E('_fom');
	fom.mac_wan.value = (fom._f_wan_macaddr.value == defmac('wan')) ? '' : fom._f_wan_macaddr.value;

	for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		u = wl_unit(uidx);
		v = E('_f_wl'+u+'_macaddr').value;
		E('_wl'+u+'_macaddr').value = (v == defmac('wl' + u)) ? '' : v;
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
	<div class='version'><% translate("Version"); %> <% version(); %></div>
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
	var u = wl_unit(uidx);
	W('<input type=\'hidden\' id=\'_wl'+u+'_macaddr\' name=\'wl'+u+'_macaddr\'>');
}
</script>

<div class='section-title'><% translate("MAC Address"); %></div>
<div class='section'>
<script type='text/javascript'>

f = [
	{ title: '<% translate("WAN Port"); %>', indent: 1, name: 'f_wan_macaddr', type: 'text', maxlen: 17, size: 20,
		suffix: ' <input type="button" value="<% translate("Default"); %>" onclick="bdefault(\'wan\')"> <input type="button" value="<% translate("Random"); %>" onclick="brand(\'wan\')"> <input type="button" value="<% translate("Clone PC"); %>" onclick="bclone(\'wan\')">',
		value: nvram.mac_wan || defmac('wan') }
];

for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	var u = wl_unit(uidx);
	f.push(
	{ title: '<% translate("Wireless Interface"); %> ' + ((wl_ifaces.length > 1) ? wl_ifaces[uidx][0] : ''), indent: 1, name: 'f_wl'+u+'_macaddr', type: 'text', maxlen: 17, size: 20,
		suffix:' <input type="button" value="<% translate("Default"); %>" onclick="bdefault(\'wl'+u+'\')"> <input type="button" value="<% translate("Random"); %>" onclick="brand(\'wl'+u+'\')"> <input type="button" value="<% translate("Clone PC"); %>" onclick="bclone(\'wl'+u+'\')">',
		value: nvram['wl'+u+'_macaddr'] || defmac('wl' + u) }
	);
}

createFieldTable('', f);

</script>
<br>
<table border=0 cellpadding=1>
	<tr><td><% translate("Router's LAN MAC Address"); %>:</td><td><b><% nv('et0macaddr'); %></b></td></tr>
	<tr><td><% translate("Computer's MAC Address"); %>:</td><td><b><% compmac(); %></b></td></tr>
</table>
</div>



<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='<% translate("Save"); %>' id='save-button' onclick='save()'>
	<input type='button' value='<% translate("Cancel"); %>' id='cancel-button' onclick='reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
