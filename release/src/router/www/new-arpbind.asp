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
<title>[<% ident(); %>] New: ARP Binding</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#arpg-grid {
	width: 300px;
}
#arpg-grid .co1,
#arpg-grid .co2 {
	width: 150px;
}

</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("new_arpbind_enable,new_arpbind_only,new_arpbind_list"); %>

var arpg = new TomatoGrid();

arpg.exist = function(f, v)
{
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

arpg.existMAC = function(mac)
{
	if (mac == "00:00:00:00:00:00") return false;
	return this.exist(0, mac);
}

arpg.existIP = function(ip)
{
	if (ip == "0.0.0.0") return true;
	return this.exist(1, ip);
}

arpg.dataToView = function(data) {
	return [data[0], data[1]];
}

arpg.verifyFields = function(row, quiet)
{
	var ok = 1;
	var f = fields.getAll(row);
	var s;

	if (v_macz(f[0], quiet)) {
		if (this.existMAC(f[0].value)) {
			ferror.set(f[0], 'Duplicate MAC address', quiet);
			ok = 0;
		}
	}
	else ok = 0;

	if (v_ip(f[1], quiet)) {
		if(this.existIP(f[1].value)) {
			ferror.set(f[1], 'Duplicate IP address', quiet);
			ok = 0;
		}
	}
	else ok = 0;

	return ok;
}

arpg.resetNewEditor = function() {
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	
	f[0].value = '00:00:00:00:00:00';
	f[1].value = '0.0.0.0';

}

arpg.setup = function()
{
	this.init('arpg-grid', '', 50,
		[{ type: 'text', maxlen: 17 }, { type: 'text', maxlen: 15 }]);
	this.headerSet(['MAC Address', 'IP Address']);
	var s = nvram.new_arpbind_list.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 2) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}

function save()
{
	if (arpg.isEditing()) return;

	var data = arpg.getAllData();
	var arpbindlist = '';
	var i;

	if (data.length != 0) arpbindlist += data[0].join('<');	
	for (i = 1; i < data.length; ++i) {
		arpbindlist += '>' + data[i].join('<');
	}

	var fom = E('_fom');
	fom.new_arpbind_enable.value = E('_f_new_arpbind_enable').checked ? 1 : 0;
	fom.new_arpbind_only.value = E('_f_new_arpbind_only').checked ? 1 : 0;
	fom.new_arpbind_list.value = arpbindlist;
	form.submit(fom, 1);
}

function init()
{
	arpg.recolor();
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

<input type='hidden' name='_nextpage' value='new-arpbind.asp'>
<input type='hidden' name='_service' value='arpbind-restart'>

<input type='hidden' name='new_arpbind_enable'>
<input type='hidden' name='new_arpbind_only'>
<input type='hidden' name='new_arpbind_list'>

<div class='section-title'>ARP Binding</div>
<div class='section'>
	<script type='text/javascript'>
	createFieldTable('', [
		{ title: 'Enable ARP Binding', name: 'f_new_arpbind_enable', type: 'checkbox', value: nvram.new_arpbind_enable != '0' },
		{ title: 'Limit unlisted machines', name: 'f_new_arpbind_only', type: 'checkbox', value: nvram.new_arpbind_only != '0' }
	]);
	</script>
<br>
	<table class='tomato-grid' id='arpg-grid'></table>
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
<script type='text/javascript'>arpg.setup();</script>
</body>
</html>
