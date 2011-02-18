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
<title>[<% ident(); %>] Forwarding: Basic</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#fo-grid .co1 {
	width: 25px;
	text-align: center;
}
#fo-grid .co2 {
	width: 70px;
}
#fo-grid .co3 {
	width: 120px;
}
#fo-grid .co4 {
	width: 80px;
}
#fo-grid .co5 {
	width: 55px;
}
#fo-grid .co6 {
	width: 110px;
}
#fo-grid .co7 {
	width: 300px;
}

</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("portforward"); %>
var lipp = '<% lipp(); %>.';

var fog = new TomatoGrid();

fog.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
	case 2:	// src
	case 5:	// ia
		r = cmpIP(da[col], db[col]);
		break;
	case 0:	// on
	case 1: // proto
	case 3:	// ext prt
	case 4:	// int prt
		r = cmpInt(da[col], db[col]);
		break;
	default:
		r = cmpText(da[col], db[col]);
		break;
	}

	return this.sortAscending ? r : -r;
}

fog.dataToView = function(data) {
	return [(data[0] != '0') ? 'On' : '', ['TCP', 'UDP', 'Both'][data[1] - 1], (data[2].match(/(.+)-(.+)/)) ? (RegExp.$1 + ' -<br>' + RegExp.$2) : data[2], data[3], data[4], data[5], data[6]];
}

fog.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	return [f[0].checked ? 1 : 0, f[1].value, f[2].value, f[3].value, f[4].value, f[5].value, f[6].value];
}

fog.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	var s;

	f[2].value = f[2].value.trim();
	ferror.clear(f[2]);
	if ((f[2].value.length) && (!v_iptaddr(f[2], quiet))) return 0;

	if (!v_iptport(f[3], quiet)) return 0;
	ferror.clear(f[4]);
	if (f[3].value.search(/[-:,]/) != -1) {
		f[4].value = '';
		f[4].disabled = true;
	}
	else {
		f[4].disabled = false;
		f[4].value = f[4].value.trim();
		if (f[4].value != '') {
			if (!v_port(f[4], quiet)) return 0;
		}
	}
	ferror.clear(f[4]);

	s = f[5].value.trim();
	if (s.match(/^ *\d+ *$/)) f[5].value = lipp + s;
	if (!v_ip(f[5], quiet, 1)) return 0;

	f[6].value = f[6].value.replace(/>/g, '_');
	if (!v_nodelim(f[6], quiet, 'Description')) return 0;
	return 1;
}

fog.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].checked = 1;
	f[1].selectedIndex = 0;
	f[2].value = '';
	f[3].value = '';
	f[4].value = '';
	f[5].value = '';
	f[6].value = '';
	ferror.clearAll(fields.getAll(this.newEditor));
}

fog.setup = function() {
	this.init('fo-grid', 'sort', 50, [
		{ type: 'checkbox' },
		{ type: 'select', options: [[1, 'TCP'],[2, 'UDP'],[3,'Both']] },
		{ type: 'text', maxlen: 32 },
		{ type: 'text', maxlen: 16 },
		{ type: 'text', maxlen: 5 },
		{ type: 'text', maxlen: 15 },
		{ type: 'text', maxlen: 32 }]);
	this.headerSet(['On', 'Proto', 'Src Address', 'Ext Ports', 'Int Port', 'Int Address', 'Description']);
	var nv = nvram.portforward.split('>');
	for (var i = 0; i < nv.length; ++i) {
		var r;

		// temp: <=1.06
		if (r = nv[i].match(/^(\d)<(\d)<([\d\-\:,]+)<(\d*)<(.*)<(.*)$/)) {
			r[1] *= 1;
			r[2] *= 1;
			r[3] = r[3].replace(/:/g, '-');
			if (r[5].match(/^ *\d+ *$/)) r[5] = lipp + r[5];
			fog.insertData(-1, [r[1], r[2], '', r[3], r[4], r[5], r[6]]);
		}
		// >=1.07
		else if (r = nv[i].match(/^(\d)<(\d)<(.*)<(.+?)<(\d*)<(.*)<(.*)$/)) {
			r[1] *= 1;
			r[2] *= 1;
			r[4] = r[4].replace(/:/g, '-');
			if (r[6].match(/^ *\d+ *$/)) r[6] = lipp + r[6];
			fog.insertData(-1, r.slice(1, 8));
		}
	}
	fog.sort(6);
	fog.showNewEditor();
}

function srcSort(a, b)
{
	if (a[2].length) return -1;
	if (b[2].length) return 1;
	return 0;
}

function save()
{
	if (fog.isEditing()) return;

	var data = fog.getAllData().sort(srcSort);
	var s = '';
	for (var i = 0; i < data.length; ++i) {
		data[i][3] = data[i][3].replace(/-/g, ':');
		s += data[i].join('<') + '>';
	}
	var fom = E('_fom');
	fom.portforward.value = s;
	form.submit(fom, 0, 'tomato.cgi');
}

function init()
{
	fog.recolor();
	fog.resetNewEditor();
}
</script>
</head>
<body onload='init()'>
<form id='_fom' method='post' action='javascript:{}'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_nextpage' value='forward-basic.asp'>
<input type='hidden' name='_service' value='firewall-restart'>

<input type='hidden' name='portforward'>

<div class='section-title'>Port Forwarding</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='fo-grid'></table>
	<script type='text/javascript'>fog.setup();</script>
</div>

<div>
<ul>
<li><b>Src Address</b> <i>(optional)</i> - Forward only if from this address. Ex: "1.2.3.4", "1.2.3.4 - 2.3.4.5", "1.2.3.0/24", "me.example.com".
<li><b>Ext Ports</b> - The ports to be forwarded, as seen from the WAN. Ex: "2345", "200,300", "200-300,400".
<li><b>Int Port</b> <i>(optional)</i> - The destination port inside the LAN. If blank, the destination port
is the same as <i>Ext Ports</i>. Only one port per entry is supported when forwarding to a different internal
port.
<li><b>Int Address</b> - The destination address inside the LAN.
</ul>
</div>

<br>
<script type='text/javascript'>show_notice1('<% notice("iptables"); %>');</script>

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
