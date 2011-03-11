<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Copyright (C) 2011 Deon 'PrinceAMD' Thomas 
	rate limit & connection limit from Conanxu, 
	adapted by Victek, Shibby, PrinceAMD, Phykris

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] New: QoS Limit</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#qosg-grid {
	width: 100%;
}
#qosg-grid .co1 {
	width: 30%;
}
#qosg-grid .co2,
#qosg-grid .co3,
#qosg-grid .co4,
#qosg-grid .co5,
#qosg-grid .co6,
#qosg-grid .co7,
#qosg-grid .co8 {
	width: 10%;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>
// <% nvram("new_qoslimit_enable,qos_ibw,qos_obw,new_qoslimit_rules,lan_ipaddr,lan_netmask,qosl_enable,qosl_dlr,qosl_dlc,qosl_ulr,qosl_ulc,qosl_udp,qosl_tcp"); %>

var class_prio = [['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']];
var class_tcp = [['0','nolimit']];
var class_udp = [['0','nolimit']];
for (var i = 1; i <= 100; ++i) {
	class_tcp.push([i*10, i*10+'']);
	class_udp.push([i, i + '/s']);
}
var qosg = new TomatoGrid();

qosg.setup = function() {
	this.init('qosg-grid', '', 40, [
		{ type: 'text', maxlen: 31 },
		{ type: 'text', maxlen: 6 },
		{ type: 'text', maxlen: 6 },
		{ type: 'text', maxlen: 6 },
		{ type: 'text', maxlen: 6 },
		{ type: 'select', options: class_prio },
		{ type: 'select', options: class_tcp },
		{ type: 'select', options: class_udp }]);
	this.headerSet(['IP | IP Range | MAC Address', 'DLRate', 'DLCeil', 'ULRate', 'ULCeil', 'Priority', 'TCP Limit', 'UDP Limit']);
	var qoslimitrules = nvram.new_qoslimit_rules.split('>');
	for (var i = 0; i < qoslimitrules.length; ++i) {
		var t = qoslimitrules[i].split('<');
		if (t.length == 8) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}

qosg.dataToView = function(data) {
	return [data[0],data[1]+'kbps',data[2]+'kbps',data[3]+'kbps',data[4]+'kbps',class_prio[data[5]*1][1],class_tcp[data[6]*1/10][1],class_udp[data[7]*1][1]];
}

qosg.resetNewEditor = function() {
	var f, c, n;

	var f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	if ((c = cookie.get('addbwlimit')) != null) {
		cookie.set('addbwlimit', '', 0);
		c = c.split(',');
		if (c.length == 2) {
	f[0].value = c[0];
	f[1].value = c[1];
	f[2].value = '';
	f[3].value = '';
	f[4].value = '';
	f[5].selectedIndex = '2';
	f[6].selectedIndex = '0';
	f[7].selectedIndex = '0';
	return;
		}
	}

	f[0].value = '';
	f[1].value = '';
	f[2].value = '';
	f[3].value = '';
	f[4].value = '';
	f[5].selectedIndex = '2';
	f[6].selectedIndex = '0';
	f[7].selectedIndex = '0';
	
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

/*
	if (v_ip(f[0], quiet)) {
		if(this.existIP(f[0].value)) {
			ferror.set(f[0], 'duplicate IP address', quiet);
			ok = 0;
		}
	}
*/
	if(v_macip(f[0], quiet, 0, nvram.lan_ipaddr, nvram.lan_netmask)) {
		if(this.existIP(f[0].value)) {
			ferror.set(f[0], 'duplicate IP or MAC address', quiet);
			ok = 0;
		}
	}
     
	if( this.checkRate(f[1].value)) {
		ferror.set(f[1], 'DLRate must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRate(f[2].value)) {
		ferror.set(f[2], 'DLCeil must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRateCeil(f[1].value, f[2].value)) {
		ferror.set(f[2], 'DLCeil must be greater than DLRate', quiet);
		ok = 0;
	}

	if( this.checkRate(f[3].value)) {
		ferror.set(f[3], 'ULRate must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRate(f[4].value)) {
		ferror.set(f[4], 'ULCeil must between 1 and 99999', quiet);
		ok = 0;
	}

	if( this.checkRateCeil(f[3].value, f[4].value)) {
		ferror.set(f[4], 'ULCeil must be greater than ULRate', quiet);
		ok = 0;
	}

	return ok;
}

function verifyFields(focused, quiet)
{
	var a = !E('_f_new_qoslimit_enable').checked;
	var b = !E('_f_qosl_enable').checked;

	E('_qos_ibw').disabled = a;
	E('_qos_obw').disabled = a;
	E('_f_qosl_enable').disabled = a;

	E('_qosl_dlr').disabled = b || a;
	E('_qosl_dlc').disabled = b || a;
	E('_qosl_ulr').disabled = b || a;
	E('_qosl_ulc').disabled = b || a;
	E('_qosl_tcp').disabled = b || a;
	E('_qosl_udp').disabled = b || a;

	elem.display(PR('_qos_ibw'), PR('_qos_obw'), !a);
	elem.display(PR('_qosl_dlr'), PR('_qosl_dlc'), PR('_qosl_ulr'), PR('_qosl_ulc'), PR('_qosl_tcp'), PR('_qosl_udp'), !a && !b);

	return 1;
}

function save()
{
	if (qosg.isEditing()) return;

	var data = qosg.getAllData();
	var qoslimitrules = '';
	var i;

    if (data.length != 0) qoslimitrules += data[0].join('<');	
	for (i = 1; i < data.length; ++i) {
		qoslimitrules += '>' + data[i].join('<');
	}

	var fom = E('_fom');
	fom.new_qoslimit_enable.value = E('_f_new_qoslimit_enable').checked ? 1 : 0;
	fom.qosl_enable.value = E('_f_qosl_enable').checked ? 1 : 0;
	fom.new_qoslimit_rules.value = qoslimitrules;
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

<input type='hidden' name='_nextpage' value='new-qoslimit.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_service' value='qoslimit-restart'>

<input type='hidden' name='new_qoslimit_enable'>
<input type='hidden' name='new_qoslimit_rules'>
<input type='hidden' name='qosl_enable'>


<div id='bwlimit'>

	<div class='section-title'>QoS Limit</div>
	<div class='section'>
		<script type='text/javascript'>
			createFieldTable('', [
			{ title: 'Enable Limiter', name: 'f_new_qoslimit_enable', type: 'checkbox', value: nvram.new_qoslimit_enable != '0' },
			{ title: 'Max Download Bandwidth <small>(also used by QOS)', name: 'qos_ibw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qos_ibw },
			{ title: 'Max Upload Bandwidth <small>(also used by QOS)', name: 'qos_obw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qos_obw }
			]);
		</script>
		<br>
		<table class='tomato-grid' id='qosg-grid'></table>
		<div>
			<ul>
				<li><b>IP Address / IP Range</b> - i.e: 192.168.1.5 or 192.168.1.45-57 or 45-57 -  A range of IP's will <b>share</b> the bandwidth.
			</ul>
		</div>
	</div>
	
	<br>

	<div class='section-title'>Default Class rate/ceiling for unlisted IP's</div>
	<div class='section'>
		<script type='text/javascript'>
			createFieldTable('', [
				{ title: 'Enable', name: 'f_qosl_enable', type: 'checkbox', value: nvram.qosl_enable == '1'},
				{ title: 'Download rate', name: 'qosl_dlr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_dlr },
				{ title: 'Download ceil', name: 'qosl_dlc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_dlc },
				{ title: 'Upload rate', name: 'qosl_ulr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_ulr },
				{ title: 'Upload ceil', name: 'qosl_ulc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_ulc },
				{ title: 'TCP Limit', name: 'qosl_tcp', type: 'select', options:
					[['0', 'no limit'],
					['1', '1'],
					['2', '2'],
					['5', '5'],
					['10', '10'],
					['20', '20'],
					['50', '50'],
					['100', '100'],
					['200', '200'],
					['500', '500'],
					['1000', '1000']], value: nvram.qosl_tcp },
				{ title: 'UDP limit', name: 'qosl_udp', type: 'select', options:
					[['0', 'no limit'],
					['1', '1/s'],
					['2', '2/s'],
					['5', '5/s'],
					['10', '10/s'],
					['20', '20/s'],
					['50', '50/s'],
					['100', '100/s']], value: nvram.qosl_udp }
			]);
		</script>
		<div>
			<ul>
				<li><b>Default Class</b> - All clients not included in the list will <b>share</b> the Default Rate/Ceiling setting.
			</ul>
		</div>
	</div>
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
<script type='text/javascript'>qosg.setup(); verifyFields(null, 1);</script>
</body>
</html>
