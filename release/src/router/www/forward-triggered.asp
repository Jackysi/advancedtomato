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
<title>[<% ident(); %>] Forwarding: Triggered</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#tg-grid .co1 {
	width: 5%;
	text-align: center;
}
#tg-grid .co2 {
	width: 10%;
}
#tg-grid .co3 {
	width: 20%;
}
#tg-grid .co4 {
	width: 20%;
}
#tg-grid .co5 {
	width: 45%;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("trigforward"); %>

var tg = new TomatoGrid();

tg.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
	case 2:	// t prt
	case 3:	// f prt
		r = cmpInt(da[col], db[col]);
		break;
	default:
		r = cmpText(da[col], db[col]);
		break;
	}

	return this.sortAscending ? r : -r;
}

tg.dataToView = function(data) {
	return [data[0] ? 'On' : '', ['TCP', 'UDP', 'Both'][data[1] - 1], data[2], data[3], data[4]];
}

tg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	return [f[0].checked ? 1 : 0, f[1].value, f[2].value, f[3].value, f[4].value];
}

tg.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	ferror.clearAll(f);
	if (!v_portrange(f[2], quiet)) return 0;
	if (!v_portrange(f[3], quiet)) return 0;
	f[4].value = f[4].value.replace(/>/g, '_');
	if (!v_nodelim(f[4], quiet, 'Description')) return 0;
	return 1;
}

tg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].checked = 1;
	f[1].selectedIndex = 0;
	f[2].value = '';
	f[3].value = '';
	f[4].value = '';
	ferror.clearAll(f);
}

tg.setup = function() {
	this.init('tg-grid', 'sort', 50, [
		{ type: 'checkbox' },
		{ type: 'select', options: [[1, 'TCP'],[2, 'UDP'],[3,'Both']] },
		{ type: 'text', maxlen: 16 },
		{ type: 'text', maxlen: 16 },
		{ type: 'text', maxlen: 32 }]);
	this.headerSet(['On', 'Protocol', 'Trigger Ports', 'Forwarded Ports', 'Description']);
	var nv = nvram.trigforward.split('>');
	for (var i = 0; i < nv.length; ++i) {
		var r;
		if (r = nv[i].match(/^(\d)<(\d)<(.+?)<(.+?)<(.*)$/)) {
			r[1] *= 1;
			r[2] *= 1;
			r[3] = r[3].replace(/:/g, '-');
			r[4] = r[4].replace(/:/g, '-');
			tg.insertData(-1, r.slice(1, 6));
		}
	}
	tg.sort(4);
	tg.showNewEditor();
}


function save()
{
	if (tg.isEditing()) return;

	var data = tg.getAllData();
	var s = '';
	for (var i = 0; i < data.length; ++i) {
		data[i][2] = data[i][2].replace(/-/g, ':');
		data[i][3] = data[i][3].replace(/-/g, ':');
		s += data[i].join('<') + '>';
	}
	var fom = E('_fom');
	fom.trigforward.value = s;
	form.submit(fom, 1);
}

function init()
{
	tg.recolor();
	tg.resetNewEditor();
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

<input type='hidden' name='_nextpage' value='forward.asp'>
<input type='hidden' name='_service' value='firewall-restart'>

<input type='hidden' name='trigforward'>

<div class='section-title'>Triggered Port Forwarding</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='tg-grid'></table>
	<script type='text/javascript'>tg.setup();</script>
</div>

<div>
<ul>
<li>Use "-" to specify a range of ports (200-300).
<li>Trigger Ports are the initial LAN to WAN "trigger".
<li>Forwarded Ports are the WAN to LAN ports that are opened if the "trigger" is activated.
<li>These ports are automatically closed after a few minutes of inactivity.
</ul>
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
</body>
</html>
