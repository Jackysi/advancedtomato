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
<title>[<% ident(); %>] Basic: IPv6</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>
//	<% nvram("ipv6_prefix,ipv6_prefix_length,ipv6_rtr_addr,ipv6_service,ipv6_tun_addr,ipv6_tun_addrlen,ipv6_tun_dev,ipv6_tun_v4end"); %>

function verifyFields(focused, quiet)
{
	var i;
	var ok = 1;
	var a, b, c;

	// --- visibility ---

	var vis = {
		_ipv6_service: 1,
		_ipv6_prefix: 1,
		_ipv6_prefix_length: 1,
		_f_ipv6_rtr_addr: 1,
		_ipv6_tun_v4end: 1,
		_ipv6_tun_dev: 1,
		_ipv6_tun_addr: 1,
		_ipv6_tun_addrlen: 1
	};

	switch(E('_ipv6_service').value) {
		case '':
			vis._ipv6_prefix = 0;
			vis._ipv6_prefix_length = 0;
			vis._f_ipv6_rtr_addr = 0;
		case 'native':
		case 'other':
			vis._ipv6_tun_v4end = 0;
			vis._ipv6_tun_dev = 0;
			vis._ipv6_tun_addr = 0;
			vis._ipv6_tun_addrlen = 0;
			break;
	}
	
	for (a in vis) {
		b = E(a);
		c = vis[a];
		b.disabled = (c != 1);
		PR(b).style.display = c ? '' : 'none';
	}

	// --- verify ---

	// IP address
	a = ['_ipv6_tun_v4end'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_ip(a[i], quiet))) ok = 0;

	// range
	a = [['_ipv6_prefix_length', 3, 64], ['_ipv6_tun_addrlen', 3, 127]];
	for (i = a.length - 1; i >= 0; --i) {
		b = a[i];
		if ((vis[b[0]]) && (!v_range(b[0], quiet, b[1], b[2]))) ok = 0;
	}

	// IPv6 address
	a = ['_ipv6_prefix', '_ipv6_tun_addr', '_f_ipv6_rtr_addr'];
	for (i = a.length - 1; i >= 0; --i)
		if ((vis[a[i]]) && (!v_ipv6_addr(a[i], quiet))) ok = 0;

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

	fom.ipv6_rtr_addr.value = fom.f_ipv6_rtr_addr.value;

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

<input type='hidden' name='_nextpage' value='basic-ipv6.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_service' value='*'>

<input type='hidden' name='ipv6_rtr_addr'>


<div class='section-title'>IPv6</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'IPv6 service type', name: 'ipv6_service', type: 'select', 
		options: [['', 'Disabled'],['native','Native IPv6 from ISP'],['sit','6in4 static tunnel'],['other','Other (manually configured)']],
		value: nvram.ipv6_service },
	{ title: 'Assigned IPv6 Prefix (and prefix length)', indent: 2, multi: [
		{ name: 'ipv6_prefix', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_prefix, suffix: ' / ' },
		{ name: 'ipv6_prefix_length', type: 'text', maxlen: 3, size: 3, value: nvram.ipv6_prefix_length, 
		suffix: '<br><small>Note: for route advertisement, the prefix length is ignored and /64 default is used</small>' }
	] },
	{ title: 'Router IPv6 address will be:', indent: 2, name: 'f_ipv6_rtr_addr', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_rtr_addr},
	{ title: 'Tunnel Foreign Endpoint (IPv4 Address)', name: 'ipv6_tun_v4end', type: 'text', maxlen: 15, size: 17, value: nvram.ipv6_tun_v4end },
	{ title: 'Tunnel Device Name', name: 'ipv6_tun_dev', type: 'text', maxlen: 8, size: 10, value: nvram.ipv6_tun_dev },
	{ title: 'Tunnel IPv6 address', indent: 2, multi: [
		{ name: 'ipv6_tun_addr', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_tun_addr, suffix: ' / ' },
		{ name: 'ipv6_tun_addrlen', type: 'text', maxlen: 3, size: 3, value: nvram.ipv6_tun_addrlen }
	] }
]);
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
