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
<title>[<% ident(); %>] Edit Access Restrictions</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
#res-comp-grid {
	width: 60%;
}
#res-bp-grid .box1 {
	width: 20%;
	float: left;
}
#res-bp-grid .box2 {
	width: 80%;
	float: left;
}
#res-bp-grid .box3 {
	width: 20%;
	float: left;
	clear: left;
}
#res-bp-grid .box4 {
	width: 20%;
	float: left;
}
#res-bp-grid .box5 {
	width: 60%;
	float: left;
}
#res-bp-grid .box6 {
	width: 30%;
	float: left;
	clear: left;
	padding-top: 2px;
}
#res-bp-grid .box7 {
	width: 70%;
	float: left;
	padding-top: 2px;
}

textarea {
	width: 99%;
	height: 8em;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>
//	<% nvram(''); %>	// http_id

/* REMOVE-BEGIN

{enable}|{begin_mins}|{end_mins}|{dow}|{comp[<comp]}|{rules<rules[...]>}|{http[ ...]}|{http_file}|{desc}

<=1.15
A<B<C<D<E<F>
A
	-1	TCP/UDP
	6	TCP
	17	UDP
B
	a	any port
	d	dst port
	s	src port
	x	dst/src port
C
	port
D
	IPP2P
E
	L7
 
 
>=1.1x
A<B<C<D<E<F>
A
	a	any address
	d	dst address
B
	address
C
	-1	TCP/UDP
	6	TCP
	17	UDP
D
	a	any port
	d	dst port
	s	src port
	x	dst/src port
E
	port
F
	IPP2P
G
	L7



REMOVE-END */

//	<% rrule(); %>
if ((rule = rrule.match(/^(\d+)\|(-?\d+)\|(-?\d+)\|(\d+)\|(.*?)\|(.*?)\|([^|]*?)\|(\d+)\|(.*)$/m)) == null) {
	rule = ['', 1, 1380, 240, 31, '', '', '', 0, 'New Rule ' + (rruleN + 1)];
}
rule[2] *= 1;
rule[3] *= 1;
rule[4] *= 1;
rule[8] *= 1;

// <% layer7(); %>
layer7.sort();
for (i = 0; i < layer7.length; ++i)
	layer7[i] = [layer7[i],layer7[i]];
layer7.unshift(['', 'Layer 7 (disabled)']);

var ipp2p = [
	[0,'IPP2P (disabled)'],[0xFFFF,'All IPP2P Filters'],[1,'AppleJuice'],[2,'Ares'],[4,'BitTorrent'],[8,'Direct Connect'],
	[16,'eDonkey'],[32,'Gnutella'],[64,'Kazaa'],[128,'Mute'],[256,'SoulSeek'],[512,'Waste'],[1024,'WinMX'],[2048,'XDCC']];

var dowNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

//

var cg = new TomatoGrid();

cg.verifyFields = function(row, quiet) {
	var f = fields.getAll(row)[0];
	if (v_mac(f, true)) return true;
	if (v_iptip(f, true)) return true;
	ferror.set(f, 'Invalid MAC address or IP address/range', quiet);
	return false;
}

cg.setup = function() {
	var a, i, count, ex;

	this.init('res-comp-grid', 'sort', 50, [ { type: 'text', maxlen: 32 } ] );
	this.headerSet(['MAC / IP Address']);
	this.showNewEditor();
	this.resetNewEditor();

	if (rule[5] == '~') return;	// wireless disable rule

	ex = 0;
	count = 0;
	a = rule[5].split('>');
	for (i = 0; i < a.length; ++i) {
		if (!a[i].length) continue;
		if (a[i] == '!') {
			ex = 1;
		}
		else {
			cg.insertData(-1, [a[i]]);
			++count;
		}
	}
	
	a = E('_f_comp_all')
	if (count) {
		a.value = ex ? 2 : 1;
	}
	else {
		a.value = 0;
	}
}

B_IPX = 0;
B_IP = 1;
B_PROTO = 2;
B_PORTX = 3;
B_PORT = 4;
B_IPP2P = 5;
B_L7 = 6;

var bpg = new TomatoGrid();

bpg.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	ferror.clearAll(f);
	this.enDiFields(row);
	if ((f[B_IPX].selectedIndex != 0) && (!v_iptip(f[B_IP], quiet))) return 0;
	if ((f[B_PORTX].selectedIndex != 0) && (!v_iptport(f[B_PORT], quiet))) return 0;
	if ((f[B_IPX].selectedIndex == 0) && (f[B_PORTX].selectedIndex == 0) && (f[B_IPP2P].selectedIndex == 0) && (f[B_L7].selectedIndex == 0)) {
		var m = 'Please enter a specific IP address, port or select an application match';
		ferror.set(f[B_L7], m, 1);
		ferror.set(f[B_IPP2P], m, 1);
		ferror.set(f[B_PORTX], m, 1);
		ferror.set(f[B_IPX], m, quiet);
		return 0;
	}
	ferror.clear(f[3]);
	return 1;
}

bpg.dataToView = function(data) {
	var s, i;

	if (data[B_IPX] == 'd') s = 'Dst IP ' + data[B_IP] + ', ';
		else s = '';
	
	if (data[B_PROTO] == 6) s += 'TCP';
		else if (data[B_PROTO] == 17) s += 'UDP';
			else s += 'TCP/UDP';
	if (data[B_PORTX] == 'd') s += ', Dst Port ';
		else if (data[B_PORTX] == 's') s += ', Src Port ';
			else if (data[B_PORTX] == 'x') s += ', Port ';
	if (data[B_PORTX] != 'a') s += data[B_PORT];

	if (data[B_IPP2P] != 0) {
		for (i = 0; i < ipp2p.length; ++i) {
			if (data[B_IPP2P] == ipp2p[i][0]) {
				s += ', IPP2P: ' + ipp2p[i][1];
				break;
			}
		}
	}
	else if (data[B_L7] != '') {
		s += ', L7: ' + data[B_L7];
	}
	return [s];
}

bpg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	return [f[B_IPX].value, (f[B_IPX].selectedIndex == 0) ? '' : f[B_IP].value,
		f[B_PROTO].value, f[B_PORTX].value, (f[B_PORTX].selectedIndex == 0) ? '' : f[B_PORT].value,
		f[B_IPP2P].value, f[B_L7].value];
},

bpg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[B_IPX].selectedIndex = 0;
	f[B_IP].value = '';
	f[B_PROTO].selectedIndex = 0;
	f[B_PORTX].selectedIndex = 0;
	f[B_PORT].value = '';
	f[B_IPP2P].selectedIndex = 0;
	f[B_L7].selectedIndex = 0;
	this.enDiFields(this.newEditor);
	ferror.clearAll(fields.getAll(this.newEditor));
}

bpg._createEditor = bpg.createEditor;
bpg.createEditor = function(which, rowIndex, source) {
	var row = this._createEditor(which, rowIndex, source);
	if (which == 'edit') this.enDiFields(row);
	return row;
}

bpg.enDiFields = function(row) {
	var f = fields.getAll(row);
	f[B_IP].disabled = (f[B_IPX].selectedIndex == 0);
	f[B_PORT].disabled = (f[B_PORTX].selectedIndex == 0);
	f[B_IPP2P].disabled = (f[B_L7].selectedIndex != 0);
	f[B_L7].disabled = (f[B_IPP2P].selectedIndex != 0);
}

bpg.setup = function() {
	var a, i, r, count;

	this.init('res-bp-grid', 'sort', 50, [ { multi: [
		{ type: 'select', prefix: '<div class="box1">', suffix: '</div>',options: [['a', 'Any IP'], ['d', 'Dst IP']] },
		{ type: 'text', prefix: '<div class="box2">', suffix: '</div>', maxlen: 32 },

		{ type: 'select', prefix: '<div class="box3">', suffix: '</div>',options: [[-1, 'TCP/UDP'], [6, 'TCP'], [17, 'UDP']] },
		{ type: 'select', prefix: '<div class="box4">', suffix: '</div>',
			options: [['a','Any Port'],['d','Dst Port'],['s','Src Port'],['x','Src or Dst']] },
		{ type: 'text', prefix: '<div class="box5">', suffix: '</div>', maxlen: 32 },

		{ type: 'select', prefix: '<div class="box6">', suffix: '</div>', options: ipp2p },
		{ type: 'select', prefix: '<div class="box7">', suffix: '</div>', options: layer7 }
		] } ] );
	this.headerSet(['Rules']);
	this.showNewEditor();
	this.resetNewEditor();
	count = 0;
	a = rule[6].split('>');
	for (i = 0; i < a.length; ++i) {
		if ((r = a[i].match(/^(x|s|d)<(.*?)<(-1|6|17)<(.)<(.*?)<(.*?)<(.*?)$/)) != null) {
			// 
			r = r.slice(1, r.length);
		}
		else if ((r = a[i].match(/^(-1|6|17)<(.)<(.*?)<(.*?)<(.*?)$/)) != null) {
			// <= 1.15
			r.splice(0, 1, 'x', '');
		}
		else {
			continue;
		}
		r[B_PORT] = r[B_PORT].replace(/:/g, '-')
		this.insertData(-1, r);
		++count;
	}
	return count;
}

//

function verifyFields(focused, quiet)
{
	var b, e;

	tgHideIcons();

	elem.display(PR('_f_sched_begin'), !E('_f_sched_allday').checked);
	elem.display(PR('_f_sched_sun'), !E('_f_sched_everyday').checked);

	b = E('_f_type').value == '0';
	elem.display(PR('_f_comp_all'), PR('_f_block_all'), b);

	elem.display(PR('res-comp-grid'), b && E('_f_comp_all').value != 0);
	elem.display(PR('res-bp-grid'), PR('_f_block_http'), PR('_f_activex'), b && !E('_f_block_all').checked);

	ferror.clear('_f_comp_all');
	
	e = E('_f_block_http');
	e.value = e.value.replace(/[|"']/g, ' ');
	if (!v_length(e, quiet, 0, 2048)) return 0;

	e = E('_f_desc');
	e.value = e.value.replace(/\|/g, '_');
	if (!v_length(e, quiet, 1)) return 0;

	return 1;
}

function cancel()
{
	document.location = 'restrict.asp';
}

function remove()
{
	if (!confirm('Delete this rule?')) return;

	E('delete-button').disabled = 1;
	
	e = E('_rrule');
	e.name = 'rrule' + rruleN;
	e.value = '';
	form.submit('_fom');
}

function save()
{
	if (!verifyFields(null, false)) return;
	if ((cg.isEditing()) || (bpg.isEditing())) return;

	var a, b, e, s, n, data;

	data = [];
	data.push(E('_f_enabled').checked ? '1' : '0');
	if (E('_f_sched_allday').checked) data.push(-1, -1);
		else data.push(E('_f_sched_begin').value, E('_f_sched_end').value);

	if (E('_f_sched_everyday').checked) {
		n = 0x7F;
	}
	else {
		n = 0;
		for (i = 0; i < 7; ++i) {
			if (E('_f_sched_' + dowNames[i].toLowerCase()).checked) n |= (1 << i);
		}
		if (n == 0) n = 0x7F;
	}
	data.push(n);
	
	if (E('_f_type').value == 0) {
		e = E('_f_comp_all');
		if (e.value != 0) {
			a = cg.getAllData();
			if (a.length == 0) {
				ferror.set(e, 'No MAC or IP address was specified', 0);
				return;
			}
			if (e.value == 2) a.unshift('!');
			data.push(a.join('>'));
		}
		else {
			data.push('');
		}

		if (E('_f_block_all').checked) {
			data.push('', '', '', '0');
		}
		else {
			a = bpg.getAllData();
			b = [];		
			for (i = 0; i < a.length; ++i) {
				a[i][B_PORT] = a[i][B_PORT].replace(/-/g, ':');
				b.push(a[i].join('<'));
			}
			data.push(b.join('>'));
			data.push(E('_f_block_http').value.replace(/\r+/g, ' ').replace(/\n+/g, '\n').replace(/ +/g, ' ').replace(/^\s+|\s+$/g, ''));
			
			n = 0;
			if (E('_f_activex').checked) n = 1;
			if (E('_f_flash').checked) n |= 2;
			if (E('_f_java').checked) n |= 4;
			data.push(n);		
		}
	}
	else {
		data.push('~');
		data.push('', '', '', '0');
	}

	data.push(E('_f_desc').value);
	data = data.join('|');
	
	if (data.length > 2048) {
		alert('This rule is too big. Please reduce by ' + (data.length - 2048) + ' characters.');
		return;
	}
	
	e = E('_rrule');
	e.name = 'rrule' + rruleN;
	e.value = data;

	E('delete-button').disabled = 1;
	form.submit('_fom');
}

function init()
{
	cg.recolor();
	bpg.recolor();
}

function earlyInit()
{
	var count;

	cg.setup();

	count = bpg.setup();
	E('_f_block_all').checked = (count == 0) && (rule[7].search(/[^\s\r\n]/) == -1) && (rule[8] == 0);
	verifyFields(null, 1);
}
</script>
</head>
<body onload='init()'>
<form name='_fom' id='_fom' method='post' action='tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version() %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_nextpage' value='restrict.asp'>
<input type='hidden' name='_service' value='restrict-restart'>
<input type='hidden' name='rruleNN' id='_rrule' value=''>

<div class='section-title'>Access Restriction</div>
<div class='section'>
<script type='text/javascript'>
W('<div style="float:right"><small>'+ 'ID: ' + rruleN.pad(2) + '</small>&nbsp;</div><br>');
tm = [];
for (i = 0; i < 1440; i += 15) tm.push([i, timeString(i)]);

createFieldTable('', [
	{ title: 'Enabled', name: 'f_enabled', type: 'checkbox', value: rule[1] == '1' },
	{ title: 'Description', name: 'f_desc', type: 'text', maxlen: 32, size: 35, value: rule[9] },
	{ title: 'Schedule', multi: [
		{ name: 'f_sched_allday', type: 'checkbox', suffix: ' All Day &nbsp; ', value: (rule[2] < 0) || (rule[3] < 0) },
		{ name: 'f_sched_everyday', type: 'checkbox', suffix: ' Everyday', value: (rule[4] & 0x7F) == 0x7F } ] },
	{ title: 'Time', indent: 2, multi: [
		{ name: 'f_sched_begin', type: 'select', options: tm, value: (rule[2] < 0) ? 0 : rule[2], suffix: ' - ' },
		{ name: 'f_sched_end', type: 'select', options: tm, value: (rule[3] < 0) ? 0 : rule[3] } ] },
	{ title: 'Days', indent: 2, multi: [
		{ name: 'f_sched_sun', type: 'checkbox', suffix: ' Sun &nbsp; ', value: (rule[4] & 1) },
		{ name: 'f_sched_mon', type: 'checkbox', suffix: ' Mon &nbsp; ', value: (rule[4] & (1 << 1)) },
		{ name: 'f_sched_tue', type: 'checkbox', suffix: ' Tue &nbsp; ', value: (rule[4] & (1 << 2)) },
		{ name: 'f_sched_wed', type: 'checkbox', suffix: ' Wed &nbsp; ', value: (rule[4] & (1 << 3)) },
		{ name: 'f_sched_thu', type: 'checkbox', suffix: ' Thu &nbsp; ', value: (rule[4] & (1 << 4)) },
		{ name: 'f_sched_fri', type: 'checkbox', suffix: ' Fri &nbsp; ', value: (rule[4] & (1 << 5)) },
		{ name: 'f_sched_sat', type: 'checkbox', suffix: ' Sat', value: (rule[4] & (1 << 6)) } ] },
	{ title: 'Type', name: 'f_type', type: 'select', options: [[0,'Normal Restriction'],[1,'Disable Wireless']], value: (rule[5] == '~') ? 1 : 0 },
	{ title: 'Applies To', name: 'f_comp_all', type: 'select', options: [[0,'All Computers / Devices'],[1,'The Following...'],[2,'All Except...']], value: 0 },
	{ title: '&nbsp;', text: '<table class="tomato-grid" cellspacing=1 id="res-comp-grid"></table>' },
	{ title: 'Blocked Resources', name: 'f_block_all', type: 'checkbox', suffix: ' Block All Internet Access', value: 0 },
	{ title: 'IP<br>Port<br>Application', indent: 2, text: '<table class="tomato-grid" cellspacing=1 id="res-bp-grid"></table>' },
	{ title: 'HTTP Request', indent: 2, name: 'f_block_http', type: 'textarea', value: rule[7] },
	{ title: 'HTTP Requested Files', indent: 2, multi: [
		{ name: 'f_activex', type: 'checkbox', suffix: ' ActiveX (ocx, cab) &nbsp;&nbsp;', value: (rule[8] & 1) },
		{ name: 'f_flash', type: 'checkbox', suffix: ' Flash (swf) &nbsp;&nbsp;', value: (rule[8] & 2) },
		{ name: 'f_java', type: 'checkbox', suffix: ' Java (class, jar) &nbsp;&nbsp;', value: (rule[8] & 4) } ] }
]);
</script>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Delete...' id='delete-button' onclick='remove()'>
	&nbsp;
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='cancel()'>
</td></tr>
</table>
<br><br>
</form>
<script type='text/javascript'>earlyInit();</script>
</body>
</html>
