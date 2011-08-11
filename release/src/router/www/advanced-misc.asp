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
<title>[<% ident(); %>] <% translate("Advanced"); %>: <% translate("Miscellaneous"); %></title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("t_features,wait_time,wan_speed,jumbo_frame_enable,jumbo_frame_size,ctf_disable"); %>

et1000 = features('1000et');

function verifyFields(focused, quiet)
{
	E('_jumbo_frame_size').disabled = !E('_f_jumbo_frame_enable').checked;
	return 1;
}

function save()
{
	var fom = E('_fom');
	fom.jumbo_frame_enable.value = E('_f_jumbo_frame_enable').checked ? 1 : 0;
/* CTF-BEGIN */
	fom.ctf_disable.value = E('_f_ctf_disable').checked ? 0 : 1;
/* CTF-END */

	if ((fom.wan_speed.value != nvram.wan_speed) ||
/* CTF-BEGIN */
	    (fom.ctf_disable.value != nvram.ctf_disable) ||
/* CTF-END */
	    (fom.jumbo_frame_enable.value != nvram.jumbo_frame_enable) ||
	    (fom.jumbo_frame_size.value != nvram.jumbo_frame_size)) {
		fom._reboot.value = '1';
		form.submit(fom, 0);
	}
	else {
		form.submit(fom, 1);
	}
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

<input type='hidden' name='_nextpage' value='advanced-misc.asp'>
<input type='hidden' name='_reboot' value='0'>

<input type='hidden' name='jumbo_frame_enable'>
<!-- CTF-BEGIN -->
<input type='hidden' name='ctf_disable'>
<!-- CTF-END -->

<div class='section-title'><% translate("Miscellaneous"); %></div>
<div class='section'>
<script type='text/javascript'>
a = [];
for (i = 3; i <= 20; ++i) a.push([i, i + ' <% translate("seconds"); %>']);
createFieldTable('', [
	{ title: '<% translate("Boot Wait Time"); %> *', name: 'wait_time', type: 'select', options: a, value: fixInt(nvram.wait_time, 3, 20, 3) },
	{ title: '<% translate("WAN Port Speed"); %> *', name: 'wan_speed', type: 'select', options: [[0,'10Mb <% translate("Full"); %>'],[1,'10Mb <% translate("Half"); %>'],[2,'100Mb <% translate("Full"); %>'],[3,'100Mb <% translate("Half"); %>'],[4,'<% translate("Auto"); %>']], value: nvram.wan_speed },
	null,
/* CTF-BEGIN */
	{ title: 'CTF (<% translate("Cut-Through Forwarding"); %>)', name: 'f_ctf_disable', type: 'checkbox', value: nvram.ctf_disable != '1' },
	null,
/* CTF-END */
	{ title: '<% translate("Enable Jumbo Frames"); %> *', name: 'f_jumbo_frame_enable', type: 'checkbox', value: nvram.jumbo_frame_enable != '0', hidden: !et1000 },
	{ title: '<% translate("Jumbo Frame Size"); %> *', name: 'jumbo_frame_size', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.jumbo_frame_size, 1, 9720, 2000),
		suffix: ' <small><% translate("Bytes"); %> (<% translate("range"); %>: 1 - 9720; <% translate("default"); %>: 2000)</small>', hidden: !et1000 }
]);
</script>
<br>
<small>* <% translate("Not all models support these options"); %>.</small>
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
