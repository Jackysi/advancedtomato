<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Tomato VLAN GUI
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Advanced: Firewall</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("block_wan,nf_loopback,ne_syncookies,imq_enable,imq_numdevs,multicast_pass,multicast_lan,multicast_lan1,multicast_lan2,multicast_lan3,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

function verifyFields(focused, quiet)
{
	var enable_mcast = E('_f_multicast').checked;
	E('_f_multicast_lan').disabled = ((!enable_mcast) || (nvram.lan_ifname.length < 1));
	E('_f_multicast_lan1').disabled = ((!enable_mcast) || (nvram.lan1_ifname.length < 1));
	E('_f_multicast_lan2').disabled = ((!enable_mcast) || (nvram.lan2_ifname.length < 1));
	E('_f_multicast_lan3').disabled = ((!enable_mcast) || (nvram.lan3_ifname.length < 1));
	if(nvram.lan_ifname.length < 1)
		E('_f_multicast_lan').checked = false;
	if(nvram.lan1_ifname.length < 1)
		E('_f_multicast_lan1').checked = false;
	if(nvram.lan2_ifname.length < 1)
		E('_f_multicast_lan2').checked = false;
	if(nvram.lan3_ifname.length < 1)
		E('_f_multicast_lan3').checked = false;

	var a = E('_f_imq_enable').checked;
	E('_imq_numdevs').disabled = !a;

	if ((enable_mcast) && (!E('_f_multicast_lan').checked) && (!E('_f_multicast_lan1').checked) && (!E('_f_multicast_lan2').checked) && (!E('_f_multicast_lan3').checked)) {
		ferror.set('_f_multicast', 'IGMPproxy must be enabled in least one LAN bridge', quiet);
		return 0;
	} else {
		ferror.clear('_f_multicast');
	}
	return 1;
}

function save()
{
	var fom;

	if (!verifyFields(null, 0)) return;

	fom = E('_fom');
	fom.block_wan.value = E('_f_icmp').checked ? 0 : 1;
	fom.ne_syncookies.value = E('_f_syncookies').checked ? 1 : 0;
	fom.imq_enable.value = E('_f_imq_enable').checked ? 1 : 0;
	fom.multicast_pass.value = E('_f_multicast').checked ? 1 : 0;
	fom.multicast_lan.value = E('_f_multicast_lan').checked ? 1 : 0;
	fom.multicast_lan1.value = E('_f_multicast_lan1').checked ? 1 : 0;
	fom.multicast_lan2.value = E('_f_multicast_lan2').checked ? 1 : 0;
	fom.multicast_lan3.value = E('_f_multicast_lan3').checked ? 1 : 0;
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

<input type='hidden' name='_nextpage' value='advanced-firewall.asp'>
<input type='hidden' name='_service' value='firewall-restart'>

<input type='hidden' name='block_wan'>
<input type='hidden' name='ne_syncookies'>
<input type='hidden' name='imq_enable'>
<input type='hidden' name='multicast_pass'>
<input type='hidden' name='multicast_lan'>
<input type='hidden' name='multicast_lan1'>
<input type='hidden' name='multicast_lan2'>
<input type='hidden' name='multicast_lan3'>

<div class='section-title'>Firewall</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Respond to ICMP ping', name: 'f_icmp', type: 'checkbox', value: nvram.block_wan == '0' },
	{ title: 'NAT loopback', name: 'nf_loopback', type: 'select', options: [[0,'All'],[1,'Forwarded Only'],[2,'Disabled']], value: fixInt(nvram.nf_loopback, 0, 2, 1) },
	{ title: 'Enable SYN cookies', name: 'f_syncookies', type: 'checkbox', value: nvram.ne_syncookies != '0' },
	null,
	{ title: 'Allow multicast', name: 'f_multicast', type: 'checkbox', value: nvram.multicast_pass == '1' },
	{ title: 'LAN', indent: 2, name: 'f_multicast_lan', type: 'checkbox', value: (nvram.multicast_lan == '1') },
	{ title: 'LAN1', indent: 2, name: 'f_multicast_lan1', type: 'checkbox', value: (nvram.multicast_lan1 == '1') },
	{ title: 'LAN2', indent: 2, name: 'f_multicast_lan2', type: 'checkbox', value: (nvram.multicast_lan2 == '1') },
	{ title: 'LAN3', indent: 2, name: 'f_multicast_lan3', type: 'checkbox', value: (nvram.multicast_lan3 == '1') },
	null,
	{ title: 'IMQ Enable', name: 'f_imq_enable', type: 'checkbox', value: nvram.imq_enable == '1', suffix: ' <small>*</small>' },
	{ title: 'Set IMQ Numdevs', name: 'imq_numdevs', type: 'text', maxlen: 4, size: 6, value: nvram.imq_numdevs, suffix: ' <small>* (range: 2 - 16; default: 2)</small>' },
	null,
	{ text: '<small>* May take effect after reboot</small>' }

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
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
