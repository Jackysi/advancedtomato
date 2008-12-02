<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
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
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("et0macaddr,mac_wan,mac_wl"); %>

defmac = {
	wan: et0plus(1),
	wl: et0plus(2)
};

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

function bdefault(which)
{
	E('_f_mac_' + which).value = defmac[which];
	verifyFields(null, true);
}

function brand(which)
{
	var mac;
	var i;

	mac = ['00'];
	for (i = 5; i > 0; --i)
		mac.push(Math.floor(Math.random() * 255).hex(2));
	E('_f_mac_' + which).value = mac.join(':');
	verifyFields(null, true);
}

function bclone(which)
{
	E('_f_mac_' + which).value = '<% compmac(); %>';
	verifyFields(null, true);
}

function verifyFields(focused, quiet)
{
	if (v_mac('_f_mac_wan', quiet) && v_mac('_f_mac_wl', quiet)) {
		if (E('_f_mac_wan').value != E('_f_mac_wl').value) return 1;
		ferror.set('_f_mac_wan', 'Addresses must be unique', quiet);
	}
	return 0;
}

function save()
{
	if (!verifyFields(null, false)) return;
	if (!confirm("Warning: Changing the MAC address may require that you reboot all devices, computers or modem connected to this router. Continue anyway?")) return

	var fom = E('_fom');
	fom.mac_wan.value = (fom._f_mac_wan.value == defmac.wan) ? '' : fom._f_mac_wan.value;
	fom.mac_wl.value = (fom._f_mac_wl.value == defmac.wl) ? '' : fom._f_mac_wl.value;
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
<input type='hidden' name='mac_wl'>

<div class='section-title'>MAC Address</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'WAN Port', indent: 1, name: 'f_mac_wan', type: 'text', maxlen: 17, size: 20,
		suffix: ' <input type="button" value="Default" onclick="bdefault(\'wan\')"> <input type="button" value="Random" onclick="brand(\'wan\')"> <input type="button" value="Clone PC" onclick="bclone(\'wan\')">',
		value: nvram.mac_wan || defmac.wan },
	{ title: 'Wireless Interface', indent: 1, name: 'f_mac_wl', type: 'text', maxlen: 17, size: 20,
		suffix:' <input type="button" value="Default" onclick="bdefault(\'wl\')"> <input type="button" value="Random" onclick="brand(\'wl\')"> <input type="button" value="Clone PC" onclick="bclone(\'wl\')">',
		value: nvram.mac_wl || defmac.wl }
]);
</script>
<br>
<table border=0 cellpadding=1>
	<tr><td>Router's MAC Address:</td><td><b><% nv('et0macaddr'); %></b></td></tr>
	<tr><td>Computer's MAC Address:</td><td><b><% compmac(); %></b></td></tr>
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
