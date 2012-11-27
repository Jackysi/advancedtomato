<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2012 Shibby
	http://openlinksys.info
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Admin: TomatoAnon Project</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='<% nv('web_css'); %>.css'>
<script type='text/javascript' src='tomato.js'></script>
<script type='text/javascript'>
//	<% nvram("tomatoanon_enable,tomatoanon_answer,tomatoanon_cru,tomatoanon_id"); %>

var anon_link = '&nbsp;&nbsp;<a href="http://tomato.groov.pl/tomatoanon.php?search=9&routerid=<% nv('tomatoanon_id'); %>" target="_blank"><i>[Checkout my router]</i></a>';

function verifyFields(focused, quiet)
{
	var o = (E('_tomatoanon_answer').value == '1');
	E('_tomatoanon_enable').disabled = !o;

	var s = (E('_tomatoanon_enable').value == '1');
	E('_tomatoanon_cru').disabled = !o || !s;

	return 1;
}

function save()
{
	if (verifyFields(null, 0)==0) return;
	var fom = E('_fom');

	fom._service.value = 'tomatoanon-restart';
	form.submit('_fom', 1);
}

function init()
{
}
</script>
</head>

<body onLoad="init()">
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
<div class='title'>Tomato</div>
<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>
<form id='_fom' method='post' action='tomato.cgi'>
<input type='hidden' name='_nextpage' value='admin-tomatoanon.asp'>
<input type='hidden' name='_service' value='tomatoanon-restart'>
<div class='section-title'>About TomatoAnon Project</div>
<div class="fields"><font color="#FFFFFF">
<b>Hello</b><br>
<br>
I would like to present You a new Project named TomatoAnon.<br>
TomatoAnon script will send anonymous or incompleted information about router`s model and installed tomato version.<br>
Those information will be used ONLY for stats.<br>
<b>Any of private information (like MAC`s, IP`s etc) will NOT be send!</b><br>
Script is open, writen is bash, well anybody can look what truly is send to database.<br>
<br>
Results you can explore on <a href=http://tomato.groov.pl/tomatoanon.php target=_blanc><b>http://tomato.groov.pl/tomatoanon.php</b></a> page.<br>
Those informations may help you with a choice the best and most popular router in your country.<br>
You can check which tomato version is used by the most people and which one is the most stable.<br>
<br>
If you don`t agree to run this script you can simple disable it<br>
Remember, you can enable it any time you want.<br>
<br>
Sended information:<br>
 - MD5SUM of WAN+LAN MAC addresses - this will identify a router. Ex: 1c1dbd4202d794251ec1acf1211bb2c8<br>
 - Model of router. Ex: Asus RT-N66U<br>
 - Version of installed tomato. Ex: 102 K26 USB<br>
 - Builtype. Ex: Mega-VPN-64K<br>
 - Country. Ex: POLAND<br>
 - ISO Country code. Ex: PL<br>
 - Uptime of your router. Ex: 3 days<br>
That`s all !!<br>
<br>
Thank You for read that and please make a right choice.<br>
<br>
<b>Best Regards!</b></font>
</div>
<br>
<br>
<div class='section-title'>TomatoAnon Settings <script>W(anon_link);</script></div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Do you know what TomatoAnon doing ?', name: 'tomatoanon_answer', type: 'select', options: [ ['0','No, i don`t. Have to read all information, before i will make a choice'], ['1','Yes, i do and want to make a choice'] ], value: nvram.tomatoanon_answer, suffix: ' '},
	{ title: 'Do you want enable TomatoAnon ?', name: 'tomatoanon_enable', type: 'select', options: [ ['-1','I`m not sure right now'], ['1','Yes, i`m sure i do'], ['0','No, i definitely wont enable it'] ], value: nvram.tomatoanon_enable, suffix: ' '},
	{ title: 'Send every', indent: 2, name: 'tomatoanon_cru', type: 'text', maxlen: 5, size: 7, value: nvram.tomatoanon_cru, suffix: ' <small>hours (range: 1 - 12; default: 6)</small>' }
]);
</script>
</div>
</form>
</div>
</td></tr>
<tr><td id='footer' colspan=2>
 <form>
 <span id='footer-msg'></span>
 <input type='button' value='Save' id='save-button' onclick='save()'>
 <input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
 </form>
</div>
</td></tr>
</table>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
