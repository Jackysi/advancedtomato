<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2007-2011 Shibby
	http://openlinksys.info
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Bandwidth: Clientmon Settings</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#cmong-grid {
	width: 100%;
}
#cmong-grid .co1 {
	width: 40%;
}
#cmong-grid .co2 {
	width: 60%;
}

</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("cmon_enable,cmon_users,imq_enable"); %>

var cmong = new TomatoGrid();

cmong.exist = function(f, v)
{
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

cmong.dataToView = function(data) {
	return [data[0], data[1]];
}

cmong.verifyFields = function(row, quiet)
{
	var f = fields.getAll(row);
	var ok = 1;

	if (!v_ip(f[0], quiet, 1)) return 0;

	return ok;
}

cmong.resetNewEditor = function() {
	var f, c;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addcmon')) != null) {
		cookie.set('addbcmon', '', 0);
		c = c.split(',');
		if (c.length == 2) {
		f[0].value = c[0];
		f[1].value = '';
		return;
		}
	}

	f[0].value = '';
	f[1].value = '';
}

cmong.setup = function()
{
	this.init('cmong-grid', '', 8, [
		{ type: 'text', maxlen: 15 },
		{ type: 'text', maxlen: 12 }
	]);
	this.headerSet(['Address IP', 'Username / Alias']);
	var s = nvram.cmon_users.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 2) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}

function save()
{
	var data = cmong.getAllData();
	var cusers = '';
	var i;

	if (data.length != 0) cusers += data[0].join('<');
	for (i = 1; i < data.length; ++i) {
		cusers += '>' + data[i].join('<');
	}

	var fom = E('_fom');
	fom.cmon_enable.value = E('_f_cmon_enable').checked ? 1 : 0;
	fom.cmon_users.value = cusers;
	form.submit(fom, 1);
}

function init()
{
	cmong.recolor();
}
</script>
</head>
<body onload='init()'>
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

<input type='hidden' name='_nextpage' value='bwm-clientmon.asp'>
<input type='hidden' name='_service' value='cmon-start'>
<input type='hidden' name='cmon_enable'>
<input type='hidden' name='cmon_users'>

<div id='cmondis'>

<div class='section-title'>Clients Monitor Settings</div>
<div class='section'>
	<script type='text/javascript'>
	createFieldTable('', [
		{ title: 'Enable Client Monitor', name: 'f_cmon_enable', type: 'checkbox', value: nvram.cmon_enable != '0' }
	]);
	</script>
<br>
</div>

<div class='section-title'>List of users</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='cmong-grid'></table>
	<script type='text/javascript'>cmong.setup();</script>
<br>
	<ul>
	<li>Make sure you have set a sufficient number of IMQ devices!!
	<li>For each user you need <b>two</b> IMQ devices.
	<li>Dont use "spaces" in Username field.
	<li>Maximum number of users on the list is 8.
	<li>Results can be viewed in Real-Time graph.
	</ul>
<br>
	<b>Example:</b> If you have three users on the list you have to previously set IMQ numdevs = 6 or more
</div>


</div>
<!-- / / / -->

<div class='note-cmon' style='display:none' id='imqwarn'>
	<b>IMQ is disabled !!</b><br><br>
	You have to enable IMQ modules<br>
	<a href='advanced-firewall.asp'>Enable &raquo;</a>
</div>

</td></tr>

<div id='savebtn'>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='reloadPage();'>
</td></tr>
</div>
</table>
</form>
<script type='text/javascript'>
if (nvram.imq_enable != '1') {
	elem.display('imqwarn', true);
	elem.display('cmondis', false);
	elem.display('savebtn', false);
}
</script>
</body>
</html>
