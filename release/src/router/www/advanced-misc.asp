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
<title>[<% ident(); %>] Advanced: Miscellaneous</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("wait_time,wan_speed"); %>


function verifyFields(focused, quiet)
{
	return 1;
}

function save()
{
	var fom = E('_fom');
	if (fom.wan_speed.value != nvram.wan_speed) {
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
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_nextpage' value='advanced-misc.asp'>
<input type='hidden' name='_reboot' value='0'>

<div class='section-title'>Miscellaneous</div>
<div class='section'>
<script type='text/javascript'>
a = [];
for (i = 3; i <= 20; ++i) a.push([i, i + ' seconds']);
createFieldTable('', [
	{ title: 'Boot Wait Time *', name: 'wait_time', type: 'select', options: a, value: fixInt(nvram.wait_time, 3, 20, 3) },
	{ title: 'WAN Port Speed *', name: 'wan_speed', type: 'select', options: [[0,'10Mb Full'],[1,'10Mb Half'],[2,'100Mb Full'],[3,'100Mb Half'],[4,'Auto']], value: nvram.wan_speed }
]);
</script>
<br>
<small>* Not all models support these options.</small>
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
