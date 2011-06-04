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
<title>[<% ident(); %>] Basic: Static DHCP</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#bs-grid {
	width: 600px;
}
#bs-grid .co1,
#bs-grid .co2 {
	width: 130px;
}
#bs-grid .co3 {
	width: 340px;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("lan_ipaddr,lan_netmask,dhcpd_static,dhcpd_startip,arpbind_enable,arpbind_only"); %>

if (nvram.lan_ipaddr.match(/^(\d+\.\d+\.\d+)\.(\d+)$/)) ipp = RegExp.$1 + '.';
	else ipp = '?.?.?.';

autonum = aton(nvram.lan_ipaddr) & aton(nvram.lan_netmask);

var sg = new TomatoGrid();

sg.exist = function(f, v)
{
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

sg.existMAC = function(mac)
{
	if (isMAC0(mac)) return false;
	return this.exist(0, mac) || this.exist(1, mac);
}

sg.existName = function(name)
{
	return this.exist(3, name);
}

sg.inStatic = function(n)
{
	return this.exist(2, n);
}

sg.dataToView = function(data) {
	var v = [];
	
	var s = data[0];
	if (!isMAC0(data[1])) s += '<br>' + data[1];
	v.push(s);
	
	for (var i = 2; i < data.length; ++i)
		v.push(escapeHTML('' + data[i]));

	return v;
},

sg.sortCompare = function(a, b) {
	var da = a.getRowData();
	var db = b.getRowData();
	var r = 0;
	switch (this.sortColumn) {
	case 0:
		r = cmpText(da[0], db[0]);
		break;
	case 1:
		r = cmpIP(da[2], db[2]);
		break;
	}
	if (r == 0) r = cmpText(da[3], db[3]);
	return this.sortAscending ? r : -r;
}

sg.verifyFields = function(row, quiet)
{
	var f, s, i;

	f = fields.getAll(row);

	if (!v_macz(f[0], quiet)) return 0;
	if (!v_macz(f[1], quiet)) return 0;
	if (isMAC0(f[0].value)) {
		f[0].value = f[1].value;
		f[1].value = '00:00:00:00:00:00';
	}
	else if (f[0].value == f[1].value) {
		f[1].value = '00:00:00:00:00:00';
	}
	else if ((!isMAC0(f[1].value)) && (f[0].value > f[1].value)) {
		s = f[1].value;
		f[1].value = f[0].value;
		f[0].value = s;
	}
	for (i = 0; i < 2; ++i) {
		if (this.existMAC(f[i].value)) {
			ferror.set(f[i], 'Duplicate MAC address', quiet);
			return 0;
		}
	}	

	if (f[2].value.indexOf('.') == -1) {
		s = parseInt(f[2].value, 10)
		if (isNaN(s) || (s <= 0) || (s >= 255)) {
			ferror.set(f[2], 'Invalid IP address', quiet);
			return 0;
		}
		f[2].value = ipp + s;
	}

	if ((!isMAC0(f[0].value)) && (this.inStatic(f[2].value))) {
		ferror.set(f[2], 'Duplicate IP address', quiet);
		return 0;
	}

	if (!v_hostname(f[3], quiet, 5)) return 0;
	if (!v_nodelim(f[3], quiet, 'Hostname', 1)) return 0;
	s = f[3].value;
	if (s.length > 0) {
		if (this.existName(s)) {
			ferror.set(f[3], 'Duplicate name.', quiet);
			return 0;
		}
	}

	if (isMAC0(f[0].value)) {
		if (s == '') {
			s = 'Both MAC address and name fields must not be empty.';
			ferror.set(f[0], s, 1);
			ferror.set(f[3], s, quiet);
			return 0;
		}
	}

	return 1;
}

sg.resetNewEditor = function() {
	var f, c, n;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addstatic')) != null) {
		cookie.set('addstatic', '', 0);
		c = c.split(',');
		if (c.length == 3) {
			f[0].value = c[0];
			f[1].value = '00:00:00:00:00:00';
			f[2].value = c[1];
			f[3].value = c[2];
			return;
		}
	}

	f[0].value = '00:00:00:00:00:00';
	f[1].value = '00:00:00:00:00:00';
	f[3].value = '';

	n = 10;
	do {
		if (--n < 0) {
			f[2].value = '';
			return;
		}
		autonum++;
	} while (((c = fixIP(ntoa(autonum), 1)) == null) || (c == nvram.lan_ipaddr) || (this.inStatic(c)));

	f[2].value = c;
}

sg.setup = function()
{
	this.init('bs-grid', 'sort', 250, [
		{ multi: [ { type: 'text', maxlen: 17 }, { type: 'text', maxlen: 17 } ] },
		{ type: 'text', maxlen: 15 },
		{ type: 'text', maxlen: 63 } ] );

	this.headerSet(['MAC Address', 'IP Address', 'Hostname']);
	var s = nvram.dhcpd_static.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 3) {
			var d = t[0].split(',');
			this.insertData(-1, [d[0], (d.length >= 2) ? d[1] : '00:00:00:00:00:00',
				(t[1].indexOf('.') == -1) ? (ipp + t[1]) : t[1], t[2]]);
		}
	}
	this.sort(2);
	this.showNewEditor();
	this.resetNewEditor();
}

function verifyFields(focused, quiet)
{
	var j = E('_f_arpbind_enable').checked;
	E('_f_arpbind_only').disabled = !j;

	return 1;
}

function save()
{
	if (sg.isEditing()) return;

	var data = sg.getAllData();
	var sdhcp = '';
	var i;

	for (i = 0; i < data.length; ++i) {
		var d = data[i];
		sdhcp += d[0];
		if (!isMAC0(d[1])) sdhcp += ',' + d[1];
		sdhcp += '<' + d[2] + '<' + d[3] + '>';
	}

	var fom = E('_fom');
	fom.dhcpd_static.value = sdhcp;
	fom.arpbind_enable.value = E('_f_arpbind_enable').checked ? 1 : 0;
	fom.arpbind_only.value = E('_f_arpbind_only').checked ? 1 : 0;
	form.submit(fom, 1);
}

function init()
{
	sg.recolor();
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

<input type='hidden' name='_nextpage' value='basic-static.asp'>
<input type='hidden' name='_service' value='dhcpd-restart,arpbind-restart'>
<input type='hidden' name='dhcpd_static'>

<input type='hidden' name='arpbind_enable'>
<input type='hidden' name='arpbind_only'>

<div class='section-title'>Static DHCP</div>
<div class='section'>
	<table class='tomato-grid' id='bs-grid'></table>
</div>

<div>
		<small>
		<ul>
		<li>To specify multiple hostnames per device, separate them with spaces.<br>
		</ul>
		</small>
</div>		

<br>
<div class='section-title'>Static ARP</div>
<div class='section'>
	<script type='text/javascript'>
	createFieldTable('', [
		{ title: 'Enable static ARP', name: 'f_arpbind_enable', type: 'checkbox', value: nvram.arpbind_enable != '0' },
		{ title: 'Restrict unlisted machines', name: 'f_arpbind_only', type: 'checkbox', value: nvram.arpbind_only != '0' }
	]);
	</script>
</div>

<div>
		<small>
		<ul>
		<li>Static ARP only works if there's one MAC address per IP. You can't enter two MAC addresses in the above table.<br>
		</ul>
		</small>
</div>	

			<br>
			<br>
			<br>

<div>
			<ul>
			<b>When using "Restrict unlisted machines"</b>
			</ul>
</div>

<div>
		<small>
		<ul>
		<li> DHCP should issue a "range" with only 1 IP address, preferably the administrator's IP - e.g. 192.168.1.100-100.<br>
		<li> You <b>MUST</b> enter your own (administrator) IP and MAC into the table, or you may be locked out of the router.<br>
		<li> You must add the IP/MAC address of all your access point(s) etc. to the table.<br>
		<li> All listed IP's will now show as "active" in the WOL table.<br>
		</ul>
		</small>

</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>sg.setup();verifyFields(null, 1);</script>
</body>
</html>
