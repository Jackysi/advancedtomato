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
<title>[<% ident(); %>] Advanced: DHCP / DNS</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
textarea {
	width: 98%;
	height: 15em;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("dhcpd_dmdns,dns_addget,dhcpd_gwmode,dns_intcpt,dhcpd_slt,dnsmasq_custom,dnsmasq_norw,dhcpd_lmax,dhcpc_vendorclass,dhcpc_requestip"); %>

if ((isNaN(nvram.dhcpd_lmax)) || ((nvram.dhcpd_lmax *= 1) < 1)) nvram.dhcpd_lmax = 255;

function verifyFields(focused, quiet)
{
	var b = (E('_f_dhcpd_sltsel').value == 1);
	elem.display('_dhcpd_sltman', b);
	if ((b) && (!v_range('_f_dhcpd_slt', quiet, 1, 43200))) return 0;
	if (!v_length('_dnsmasq_custom', quiet, 0, 2048)) return 0;
	if (!v_range('_dhcpd_lmax', quiet, 1, 0xFFFF)) return 0;
	if (!v_ipz('_dhcpc_requestip', quiet)) return 0;
	return 1;
}

function nval(a, b)
{
	return (a == null || (a + '').trim() == '') ? b : a;
}

function save()
{
	if (!verifyFields(null, false)) return;

	var a;
	var fom = E('_fom');

	fom.dhcpd_dmdns.value = E('_f_dhcpd_dmdns').checked ? 1 : 0;
	a = E('_f_dhcpd_sltsel').value;
	fom.dhcpd_slt.value = (a != 1) ? a : E('_f_dhcpd_slt').value;
	fom.dns_addget.value = E('_f_dns_addget').checked ? 1 : 0;
	fom.dhcpd_gwmode.value = E('_f_dhcpd_gwmode').checked ? 1 : 0;
	fom.dns_intcpt.value = E('_f_dns_intcpt').checked ? 1 : 0;

	if (fom.dhcpc_vendorclass.value != nvram.dhcpc_vendorclass ||
	    nval(fom.dhcpc_requestip.value, '0.0.0.0') != nval(nvram.dhcpc_requestip, '0.0.0.0')) {
		nvram.dhcpc_vendorclass = fom.dhcpc_vendorclass.value;
		nvram.dhcpc_requestip = fom.dhcpc_requestip.value;
		fom._service.value = '*';
	}
	else {
		fom._service.value = 'dnsmasq-restart';
	}

	if (fom.dns_intcpt.value != nvram.dns_intcpt) {
		nvram.dns_intcpt = fom.dns_intcpt.value;
		if (fom._service.value != '*') fom._service.value += ',firewall-restart';
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

<input type='hidden' name='_nextpage' value='advanced-dhcpdns.asp'>
<input type='hidden' name='_service' value=''>

<input type='hidden' name='dhcpd_dmdns'>
<input type='hidden' name='dhcpd_slt'>
<input type='hidden' name='dns_addget'>
<input type='hidden' name='dhcpd_gwmode'>
<input type='hidden' name='dns_intcpt'>

<div class='section-title'>DHCP / DNS Server (LAN)</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Use internal DNS', name: 'f_dhcpd_dmdns', type: 'checkbox', value: nvram.dhcpd_dmdns == '1' },
	{ title: 'Use received DNS with user-entered DNS', name: 'f_dns_addget', type: 'checkbox', value: nvram.dns_addget == '1' },
	{ title: 'Intercept DNS port<br>(UDP 53)', name: 'f_dns_intcpt', type: 'checkbox', value: nvram.dns_intcpt == '1' },
	{ title: 'Use user-entered gateway if WAN is disabled', name: 'f_dhcpd_gwmode', type: 'checkbox', value: nvram.dhcpd_gwmode == '1' },
	{ title: 'Maximum active DHCP leases', name: 'dhcpd_lmax', type: 'text', maxlen: 5, size: 8, value: nvram.dhcpd_lmax },
	{ title: 'Static lease time', multi: [
		{ name: 'f_dhcpd_sltsel', type: 'select', options: [[0,'Same as normal lease time'],[-1,'"Infinite"'],[1,'Custom']],
			value: (nvram.dhcpd_slt < 1) ? nvram.dhcpd_slt : 1 },
		{ name: 'f_dhcpd_slt', type: 'text', maxlen: 5, size: 8, prefix: '<span id="_dhcpd_sltman"> ', suffix: ' <i>(minutes)</i></span>',
			value: (nvram.dhcpd_slt >= 1) ? nvram.dhcpd_slt : 3600 } ] },
	{ title: '<a href="http://www.thekelleys.org.uk/" target="_new">Dnsmasq</a><br>Custom configuration', name: 'dnsmasq_custom', type: 'textarea', value: nvram.dnsmasq_custom }
]);
</script>
<br>
Note: The file /etc/dnsmasq.custom is also added to the end of Dnsmasq's configuration file if it exists.
</div>
<br>

<div class='section-title'>DHCP Client (WAN)</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Vendor Class ID', name: 'dhcpc_vendorclass', type: 'text', maxlen: 80, size: 34, value: nvram.dhcpc_vendorclass },
	{ title: 'IP Address to request', name: 'dhcpc_requestip', type: 'text', maxlen: 15, size: 34, value: nvram.dhcpc_requestip }
]);
</script>
</div>


<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>verifyFields(null, true);</script>
</body>
</html>

