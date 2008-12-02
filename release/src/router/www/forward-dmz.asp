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
<title>[<% ident(); %>] Forwarding: DMZ</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("dmz_enable,dmz_ipaddr,dmz_sip"); %>

var lipp = '<% lipp(); %>.';

function verifyFields(focused, quiet)
{
	var sip, dip, off;

	off = !E('_f_dmz_enable').checked;
	
	dip = E('_f_dmz_ipaddr')
	dip.disabled = off;

	sip = E('_dmz_sip');
	sip.disabled = off;

	if (off) {
		ferror.clearAll(dip, sip);
		return 1;
	}	
	
	if (dip.value.indexOf('.') == -1) dip.value = lipp + dip.value;
	if (!v_ip(dip)) return 0;

	if ((sip.value.length) && (!v_iptip(sip))) return 0;
	
	return 1;
}

function save()
{
	var fom;
	var en;
	var s;
	
	if (!verifyFields(null, false)) return;

	fom = E('_fom');
	en = fom.f_dmz_enable.checked;
	fom.dmz_enable.value = en ? 1 : 0;
	if (en) {
		// shorten it if possible to be more compatible with original
		s = fom.f_dmz_ipaddr.value;
		fom.dmz_ipaddr.value = (s.indexOf(lipp) == 0) ? s.replace(lipp, '') : s;
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

<input type='hidden' name='_nextpage' value='forward-dmz.asp'>
<input type='hidden' name='_service' value='firewall-restart'>

<input type='hidden' name='dmz_enable'>
<input type='hidden' name='dmz_ipaddr'>

<div class='section-title'>DMZ</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Enable DMZ', name: 'f_dmz_enable', type: 'checkbox', value: (nvram.dmz_enable == '1') },
	{ title: 'Destination Address', indent: 2, name: 'f_dmz_ipaddr', type: 'text', maxlen: 15, size: 17,
		value: (nvram.dmz_ipaddr.indexOf('.') != -1) ? nvram.dmz_ipaddr : (lipp + nvram.dmz_ipaddr) },
	{ title: 'Source Address<br>Restriction', indent: 2, name: 'dmz_sip', type: 'text', maxlen: 32, size: 32,
		value: nvram.dmz_sip, suffix: '&nbsp;<small>(optional; ex: "1.1.1.1", "1.1.1.0/24" or "1.1.1.1 - 2.2.2.2")</small>' }
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

