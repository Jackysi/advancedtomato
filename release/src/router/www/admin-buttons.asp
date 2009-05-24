<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2009 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Admin: Buttons</title>
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
//	<% nvram("sesx_b0,sesx_b1,sesx_b2,sesx_b3,sesx_script,script_brau,t_model,t_features"); %>

var ses = features('ses');
var brau = features('brau');

function verifyFields(focused, quiet)
{
	return 1;
}

function save()
{
	form.submit('_fom', 1);
}

function earlyInit()
{
	if ((!brau) && (!ses)) {
		E('save-button').disabled = 1;
		return;
	}

	if (brau) E('braudiv').style.display = '';
	E('sesdiv').style.display = '';
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

<input type='hidden' name='_nextpage' value='admin-buttons.asp'>
<input type='hidden' name='sesx_led' value='0'>

<div id='sesdiv' style='display:none'>
<div class='section-title'>SES/AOSS Button</div>
<div class='section'>
<script type='text/javascript'>
a = [[0,'Do Nothing'],[1,'Toggle Wireless'],[2,'Reboot'],[3,'Shutdown'],[4,'Run Custom Script']];
createFieldTable('', [
	{ title: "When Pushed For..." },
	{ title: '0-2 Seconds', indent: 2, name: 'sesx_b0', type: 'select', options: a, value: nvram.sesx_b0 || 0 },
	{ title: '4-6 Seconds', indent: 2, name: 'sesx_b1', type: 'select', options: a, value: nvram.sesx_b1 || 0 },
	{ title: '8-10 Seconds', indent: 2, name: 'sesx_b2', type: 'select', options: a, value: nvram.sesx_b2 || 0 },
	{ title: '12+ Seconds', indent: 2, name: 'sesx_b3', type: 'select', options: a, value: nvram.sesx_b3 || 0 },
	{ title: 'Custom Script', indent: 2, name: 'sesx_script', type: 'textarea', value: nvram.sesx_script }
]);
</script>
</div>
</div>

<div id='braudiv' style='display:none'>
<div class='section-title'>Bridge/Auto Switch</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Custom Script', indent: 2, name: 'script_brau', type: 'textarea', value: nvram.script_brau }
]);
</script>
</div>
</div>

<script type='text/javascript'>
if ((!ses) && (!brau)) W('<i>This feature is not supported on this router.</i>');
</script>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>earlyInit()</script>
</body>
</html>

