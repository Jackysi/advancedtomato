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
<title>[<% ident(); %>] QoS: Classification</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>
<script type='text/javascript' src='protocols.js'></script>

<!-- / / / -->

<style type='text/css'>
#qg div {
	padding: 0 0 1px 0;
	margin: 0;
}

#qg .co1 {
	width: 370px;
}
#qg .co2 {
	width: 80px;
}
#qg .co3 {
	width: 300px;
}

#qg .x1a {
	width: 35%;
	float: left;
}
#qg .x1b {
	width: 64%;
	float: left;
}

#qg .x2a {
	width: 35%;
	float: left;
	clear: left;
}
#qg .x2b {
	width: 23%;
	float: left;
}
#qg .x2c {
	width: 41%;
	float: left;
}

#qg .x3a {
	width: 40%;
	float: left;
	clear: left;
}
#qg .x3b {
	width: 60%;
	float: left;
}

#qg .x4a {
	float: left;
	clear: left;
	width: 70px;
}
#qg .x4b {
	float: left;
	padding: 2px 8px 0 8px;
	width: 10px;
	text-align: center;
}
#qg .x4c {
	float: left;
	width: 70px;
}
#qg .x4d {
	float: left;
	padding: 2px 0 0 8px;
	width: 100px;
}

</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("qos_enable,qos_orules"); %>

var abc = ['Highest', 'High', 'Medium', 'Low', 'Lowest', 'A','B','C','D','E'];

var ipp2p = [
	[0,'IPP2P (disabled)'],[0xFFF,'All IPP2P filters'],[1,'AppleJuice'],[2,'Ares'],[4,'BitTorrent'],[8,'Direct Connect'],
	[16,'eDonkey'],[32,'Gnutella'],[64,'Kazaa'],[128,'Mute'],[256,'SoulSeek'],[512,'Waste'],[1024,'WinMX'],[2048,'XDCC']]

// <% layer7(); %>
layer7.sort();
for (i = 0; i < layer7.length; ++i)
	layer7[i] = [layer7[i],layer7[i]];
layer7.unshift(['', 'Layer 7 (disabled)']);

var class1 = [[-1,'Disabled']];
for (i = 0; i < 10; ++i) class1.push([i, abc[i]]);
var class2 = class1.slice(1);

var qosg = new TomatoGrid();

qosg.dataToView = function(data) {
	var b = [];
	var s, i;

	if (data[0] != 0) {
		b.push(((data[0] == 1) ? 'To ' : 'From ') + data[1]);
	}
	if (data[2] >= -1) {
		if (data[2] == -1) b.push('TCP/UDP');
			else if (data[2] >= 0) b.push(protocols[data[2]] || data[2]);
		if (data[3] != 'a') {
			if (data[3] == 'd') s = 'Dst ';
				else if (data[3] == 's') s = 'Src ';
					else s = '';
			b.push(s + 'Port: ' + data[4].replace(/:/g, '-'));
		}
	}
	if (data[5] != 0) {
		for (i = 0; i < ipp2p.length; ++i)
			if (ipp2p[i][0] == data[5]) {
				b.push('IPP2P: ' + ipp2p[i][1])
				break;
			}

	}
	else if (data[6] != '') {
		b.push('L7: ' + data[6])
	}
	
	if (data[7] != '') {
		b.push('Transferred: ' + data[7] + ((data[8] == '') ? '<small>KB+</small>' : (' - ' + data[8] + '<small>KB</small>')));
	}
	return [b.join('<br>'), class1[(data[9] * 1) + 1][1], escapeHTML(data[10])];
}

qosg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	var proto = f[2].value;
	var dir = f[3].value;
	if ((proto != -1) && (proto != 6) && (proto != 17)) dir = 'a';
	return [f[0].value, f[0].selectedIndex ? f[1].value : '',
			proto, dir, (dir != 'a') ? f[4].value : '',
			f[5].value, f[6].value, f[7].value, f[8].value, f[9].value, f[10].value];
}

qosg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].selectedIndex = 0;
	f[1].value = '';
	f[2].selectedIndex = 1;
	f[3].selectedIndex = 0;
	f[4].value = '';
	f[5].selectedIndex = 0;
	f[6].selectedIndex = 0;
	f[7].value = '';
	f[8].value = '';
	f[9].selectedIndex = 5;
	f[10].value = '';
	this.enDiFields(this.newEditor);
	ferror.clearAll(fields.getAll(this.newEditor));
}

qosg._disableNewEditor = qosg.disableNewEditor;
qosg.disableNewEditor = function(disable) {
	qosg._disableNewEditor(disable);
	if (!disable) {
		this.enDiFields(this.newEditor);
	}
}

qosg.enDiFields = function(row) {
	var f = fields.getAll(row);
	var x;

	f[1].disabled = (f[0].selectedIndex == 0);
	x = f[2].value;
	x = ((x != -1) && (x != 6) && (x != 17));
	f[3].disabled = x;
	if (f[3].selectedIndex == 0) x = 1;
	f[4].disabled = x;

	f[6].disabled = (f[5].selectedIndex != 0);
	f[5].disabled = (f[6].selectedIndex != 0);
}

qosg.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	var a, b, e;

	this.enDiFields(row);
	ferror.clearAll(f);

	a = f[0].value * 1;
	if ((a == 1) || (a == 2)) {
		if (!v_iptip(f[1], quiet)) return 0;
	}
	else if ((a == 3) && (!v_mac(f[1], quiet))) return 0;

	b = f[2].selectedIndex;
	if ((b > 0) && (b <= 3) && (f[3].selectedIndex != 0) && (!v_iptport(f[4], quiet))) return 0;

	var BMAX = 1024 * 1024;

	e = f[7];
	a = e.value = e.value.trim();
	if (a != '') {
		if (!v_range(e, quiet, 0, BMAX)) return 0;
		a *= 1;
	}
		
	e = f[8];
	b = e.value = e.value.trim();
	if (b != '') {
		b *= 1;
		if (b >= BMAX) e.value = '';
			else if (!v_range(e, quiet, 0, BMAX)) return 0;
		if (a == '') f[7].value = a = 0;
	}
	else if (a != '') {
		b = BMAX;
	}
	
	if ((b != '') && (a >= b)) {
		ferror.set(f[7], 'Invalid range', quiet);
		return 0;
	}
	
	return v_length(f[10], quiet);
}

qosg.setup = function() {
	var i, a, b;
	a = [[-2, 'Any Protocol'],[-1,'TCP/UDP'],[6,'TCP'],[17,'UDP']];
	for (i = 0; i < 256; ++i) {
		if ((i != 6) && (i != 17)) a.push([i, protocols[i] || i]);
	}

	// what a mess...
	this.init('qg', 'move', 50, [
		{ multi: [
			{ type: 'select', options: [[0,'Any Address'],[1,'Dst IP'],[2,'Src IP'],[3,'Src MAC']],
				prefix: '<div class="x1a">', suffix: '</div>' },
			{ type: 'text', prefix: '<div class="x1b">', suffix: '</div>' },
			{ type: 'select', prefix: '<div class="x2a">', suffix: '</div>', options: a },
			{ type: 'select', prefix: '<div class="x2b">', suffix: '</div>',
				options: [['a','Any Port'],['d','Dst Port'],['s','Src Port'],['x','Src or Dst']] },
            { type: 'text', prefix: '<div class="x2c">', suffix: '</div>' },
			{ type: 'select', prefix: '<div class="x3a">', suffix: '</div>', options: ipp2p },
			{ type: 'select', prefix: '<div class="x3b">', suffix: '</div>', options: layer7 },
			
			{ type: 'text', prefix: '<div class="x4a">', suffix: '</div>' },
			{ type: 'text', prefix: '<div class="x4b"> - </div><div class="x4c">', suffix: '</div><div class="x4d">KB Transferred</div>' }
			
		] },
		{ type: 'select', options: class1, vtop: 1 },
		{ type: 'text', maxlen: 32, vtop: 1 }
	]);

	this.headerSet(['Match Rule', 'Class', 'Description']);

// addr_type < addr < proto < port_type < port < ipp2p < L7 < bcount < class < desc
	
	a = nvram.qos_orules.split('>');
	for (i = 0; i < a.length; ++i) {
		b = a[i].split('<');
		if (b.length == 9) {
			// fixup < 0.08		!!! temp
			b.splice(7, 0, '', '');
		}
		else if (b.length == 10) {
			c = b[7].split(':');
			b.splice(7, 1, c[0], (c.length == 1) ? '' : c[1]);
			b[10] = unescape(b[10]);
		}
		else continue;
		b[4] = b[4].replace(/:/g, '-');
		qosg.insertData(-1, b);
	}

	this.showNewEditor();
	this.resetNewEditor();
}

function verifyFields(focused, quiet)
{
	return 1;
}

function save()
{
	if (qosg.isEditing()) return;

	var fom = E('_fom');
	var i, a, b, c;

	c = qosg.getAllData();
	a = [];
	for (i = 0; i < c.length; ++i) {
		b = c[i].slice(0);
		b[4] = b[4].replace(/-/g, ':');
		b.splice(7, 2, (b[7] == '') ? '' : [b[7],b[8]].join(':'));
		b[9] = escapeD(b[9]);
		a.push(b.join('<'));
	}
	fom.qos_orules.value = a.join('>');

	form.submit(fom, 1);
}

function init()
{
	qosg.recolor();
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

<input type='hidden' name='_nextpage' value='qos-classify.asp'>
<input type='hidden' name='_service' value='qos-restart'>
<input type='hidden' name='qos_orules'>

<div class='section-title'>Outbound Direction</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='qg'></table>
</div>

<script type='text/javascript'>
if (nvram.qos_enable != '1') {
	W('<div class="note-disabled"><b>QoS disabled.</b> &nbsp; <a href="qos-settings.asp">Enable &raquo;</a></div>');
}
</script>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>qosg.setup();</script>
</body>
</html>

