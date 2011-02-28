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
<title>[<% ident(); %>] New: IP/Range BW Limiter</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#qosg-grid {
	width: 100%;
}
#qosg-grid .co1 {
	width: 6%;
}
#qosg-grid .co2 {
	width: 24%;
}
#qosg-grid .co3,
#qosg-grid .co4,
#qosg-grid .co5,
#qosg-grid .co6,
#qosg-grid .co7,
#qosg-grid .co8,
#qosg-grid .co9 {
	width: 10%;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>
// <% nvram("new_qoslimit_enable,new_qoslimit_ibw,new_qoslimit_obw,new_qoslimit_rules,new_qoslimit_d_enable,new_qoslimit_d_dlr,new_qoslimit_d_dlc,new_qoslimit_d_ulr,new_qoslimit_d_ulc,qos_enable"); %>
var class_prio = [['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']];
var class_tcp = [['0','nolimit']];
var class_udp = [['0','nolimit']];
for (var i = 1; i <= 100; ++i) {
	class_tcp.push([i*10, i*10+'']);
	class_udp.push([i, i + '/s']);
}
var qosg = new TomatoGrid();

qosg.setup = function() {
	this.init('qosg-grid', 'sort', 60, [
		{ type: 'text', maxlen: 2 },
		{ type: 'text', maxlen: 31 },
		{ type: 'text', maxlen: 6 },
		{ type: 'text', maxlen: 6 },
		{ type: 'text', maxlen: 6 },
		{ type: 'text', maxlen: 6 },
		{ type: 'select', options: class_prio },
		{ type: 'select', options: class_tcp },
		{ type: 'select', options: class_udp }]);
	this.headerSet(['TC Tag', 'IP Address / IP Range', 'DLRate', 'DLCeil', 'ULRate', 'ULCeil', 'Priority', 'TCP Limit', 'UDP Limit']);
	this.sort(1);
	var qoslimitrules = nvram.new_qoslimit_rules.split('>');
	for (var i = 0; i < qoslimitrules.length; ++i) {
		var t = qoslimitrules[i].split('<');
		if (t.length == 9) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}



qosg.dataToView = function(data) {
	return [data[0],data[1],data[2]+'kbps',data[3]+'kbps',data[4]+'kbps',data[5]+'kbps',class_prio[data[6]*1][1],class_tcp[data[7]*1/10][1],class_udp[data[8]*1][1]];
}

qosg.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	var data = this.getAllData();
	var tag = '9';

	for (var i = 0; i < data.length; ++i) {	
		if (parseInt(data[i][0], 10) > parseInt(tag, 10))
			tag = data[i][0];
	}
	
	tag = parseInt(tag, 10)+1;

	f[0].value = tag+'';
	f[1].value = '';
	f[2].value = '';
	f[3].value = '';
	f[4].value = '';
	f[5].value = '';
	f[6].selectedIndex = '2';
	f[7].selectedIndex = '0';
	f[8].selectedIndex = '0';
	ferror.clearAll(fields.getAll(this.newEditor));
}

qosg.exist = function(f, v)
{
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

qosg.existID = function(id)
{
	return this.exist(0, id);
}

qosg.existIP = function(ip)
{
	if (ip == "0.0.0.0") return true;
	return this.exist(1, ip);
}

qosg.checkRate = function(rate)
{
	var s = parseInt(rate, 10);
	if( isNaN(s) || s <= 0 || a >= 100000 ) return true;
	return false;
}

qosg.checkRateCeil = function(rate, ceil)
{
	var r = parseInt(rate, 10);
	var c = parseInt(ceil, 10);
	if( r > c ) return true;
	return false;
}

qosg.verifyFields = function(row, quiet)
{
	var ok = 1;
	var f = fields.getAll(row);
	var s;

	if (v_range(f[0], quiet, 10, 98)) {
		if(this.existID(f[0].value)) {
			ferror.set(f[0], 'ID must between 10 and 98', quiet);
			ok = 0;
		}
	}

	if (v_iptip(f[1], quiet)) {
		if(this.existIP(f[1].value)) {
			ferror.set(f[1], 'duplicate IP address', quiet);
			ok = 0;
		}
	}

	if( this.checkRate(f[2].value)) {
		ferror.set(f[2], 'DLRate must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRate(f[3].value)) {
		ferror.set(f[3], 'DLCeil must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRateCeil(f[2].value, f[3].value)) {
		ferror.set(f[3], 'DLCeil must be greater than DLRate', quiet);
		ok = 0;
	}

	if( this.checkRate(f[4].value)) {
		ferror.set(f[4], 'ULRate must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRate(f[5].value)) {
		ferror.set(f[5], 'ULCeil must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRateCeil(f[4].value, f[5].value)) {
		ferror.set(f[5], 'ULCeil must be greater than ULRate', quiet);
		ok = 0;
	}

	return ok;
}

function init()
{
	qosg.recolor();
}

function verifyFields(focused, quiet)
{
	var a = !E('_f_new_qoslimit_enable').checked;
	var b = !E('_f_new_qoslimit_d_enable').checked;

	E('_new_qoslimit_ibw').disabled = a;
	E('_new_qoslimit_obw').disabled = a;
	E('_f_new_qoslimit_d_enable').disabled = a;

	E('_new_qoslimit_d_dlr').disabled = b || a;
	E('_new_qoslimit_d_dlc').disabled = b || a;
	E('_new_qoslimit_d_ulr').disabled = b || a;
	E('_new_qoslimit_d_ulc').disabled = b || a;

	elem.display(PR('_new_qoslimit_ibw'), PR('_new_qoslimit_obw'), !a);
	elem.display(PR('_new_qoslimit_d_dlr'), PR('_new_qoslimit_d_dlc'), PR('_new_qoslimit_d_ulr'), PR('_new_qoslimit_d_ulc'), !a && !b);

	return 1;
}

function save()
{
	if (!verifyFields(null, 0)) return;
	if (qosg.isEditing()) return;

	var data = qosg.getAllData();
	var qoslimitrules = '';
	var i;

	if (data.length != 0) qoslimitrules += data[0].join('<');
	for (i = 1; i < data.length; ++i) {
		qoslimitrules += '>' + data[i].join('<');
	}

	var fom = E('_fom');
	fom.new_qoslimit_d_enable.value = E('_f_new_qoslimit_d_enable').checked ? 1 : 0;
	fom.new_qoslimit_enable.value = E('_f_new_qoslimit_enable').checked ? 1 : 0;
	fom.new_qoslimit_rules.value = qoslimitrules;
	form.submit(fom, 1);
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

<input type='hidden' name='_nextpage' value='new-qoslimit.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_service' value='qoslimit-restart'>

<input type='hidden' name='new_qoslimit_enable'>
<input type='hidden' name='new_qoslimit_rules'>
<input type='hidden' name='new_qoslimit_d_enable'>

<div id='bwlimit'>

	<div class='section-title'>QoS Limit</div>
	<div class='section'>
		<script type='text/javascript'>
			createFieldTable('', [
				{ title: 'Enable QoS Limit', name: 'f_new_qoslimit_enable', type: 'checkbox', value: nvram.new_qoslimit_enable == '1' },
				{ title: 'Download Bandwidth', name: 'new_qoslimit_ibw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.new_qoslimit_ibw },
				{ title: 'Upload Bandwidth', name: 'new_qoslimit_obw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.new_qoslimit_obw }
			]);
		</script>
		<br>
		<table class='tomato-grid' id='qosg-grid'></table>
		<div>
			<ul>
				<li><b>IP Address / IP Range</b> - i.e: 192.168.1.5 or 192.168.1.4-192.168.1.7
			</ul>
		</div>
	</div>

	<br>

	<div class='section-title'>Default Class rate/ceil for unlisted IP's</div>
	<div class='section'>
		<script type='text/javascript'>
			createFieldTable('', [
				{ title: 'Enable', name: 'f_new_qoslimit_d_enable', type: 'checkbox', value: nvram.new_qoslimit_d_enable == '1'},
				{ title: 'Download rate', name: 'new_qoslimit_d_dlr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.new_qoslimit_d_dlr },
				{ title: 'Download ceil', name: 'new_qoslimit_d_dlc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.new_qoslimit_d_dlc },
				{ title: 'Upload rate', name: 'new_qoslimit_d_ulr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.new_qoslimit_d_ulr },
				{ title: 'Upload ceil', name: 'new_qoslimit_d_ulc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.new_qoslimit_d_ulc }
			]);
		</script>
		<br>
		<div>
			<ul>
				<li><b>Default Class</b> - IP's non included in the list will take the Default Rate/Ceil setting.
			</ul>
		</div>
	</div>
</div>

<div class='note-disabledw' style='display:none' id='qoswarn'>
	<b>You have to disable standard QoS prior to Qos/IP Range BW Limit activation.</b><br><br>
	Both features can't run together.<br><br>
	<a href='qos-settings.asp'>Disable &raquo;</a>
</div>

<!-- / / / -->

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
qosg.setup(); verifyFields(null, 1);

if (nvram.qos_enable != '0') {
	elem.display('qoswarn', true);
	elem.display('bwlimit', false);
	elem.display('savebtn', false);
}
</script>
</body>
</html>
