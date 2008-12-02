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
<title>[<% ident(); %>] Basic: Static DHCP</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style tyle='text/css'>
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

//	<% nvram("lan_ipaddr,dhcp_start,dhcp_num,dhcpd_static"); %>

if (nvram.lan_ipaddr.match(/^(\d+\.\d+\.\d+)\.(\d+)$/)) {
	ipp = RegExp.$1 + '.';
	router = RegExp.$2;
}
else {
	ipp = '?.?.?.';
	router = '1';
}


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
	if (mac == "00:00:00:00:00:00") return false;
	return this.exist(0, mac);
}

sg.existName = function(name)
{
	return this.exist(2, name);
}

sg.inStatic = function(n)
{
	return this.exist(1, n);
}

sg.inDynamic = function(n)
{
	return (n >= (nvram.dhcp_start * 1)) && (n < ((nvram.dhcp_start * 1) + (nvram.dhcp_num * 1)));
}

sg.dataToView = function(data) {
	return [data[0], (data[1].indexOf('.') == -1) ? (ipp + data[1]) : data[1], data[2]];
}

sg.sortCompare = function(a, b) {
	var da = a.getRowData();
	var db = b.getRowData();
	var r = 0;
	switch (this.sortColumn) {
	case 0:
		r = cmpText(da[0], db[0]);
		break;
	case 1:
		r = cmpInt(da[1], db[1]);
		break;
	}
	if (r == 0) r = cmpText(da[2], db[2]);
	return this.sortAscending ? r : -r;
}

sg.verifyFields = function(row, quiet)
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

	var fip = f[1].value.indexOf('.') != -1;
	if (fip) {
		if (v_ip(f[1], quiet)) {
			s = f[1].value;
			if (s.indexOf(ipp) == 0) {
				f[1].value = s.substring(ipp.length, s.length);
				fip = 0;
			}
		}
	}
	if (!fip) {
		s = parseInt(f[1].value, 10)
		if (isNaN(s) || (s <= 0) || (s >= 255)) {
			ferror.set(f[1], 'Invalid IP address', quiet);
			ok = 0;
		}
		if (this.inStatic(s)) {
			ferror.set(f[1], 'Duplicate IP address', quiet);
			ok = 0;
		}
		if (ok) f[1].value = s;
	}

	s = f[2].value.trim().replace(/\s+/g, ' ');
	if (s.length > 0) {
		if (s.search(/^[.a-zA-Z0-9_\- ]+$/) == -1) {
			ferror.set(f[2], 'Invalid name. Only characters "A-Z 0-9 . - _" are allowed.', quiet);
			ok = 0;
		}
		if (this.existName(s)) {
			ferror.set(f[2], 'Duplicate name.', quiet);
			ok = 0;
		}
		if (ok) f[2].value = s;
	}

	if (ok) {
		if (f[0].value == '00:00:00:00:00:00') {
			if (s == '') {
				s = 'Both MAC address and name fields must not be empty.';
				ferror.set(f[0], s, 1);
				ferror.set(f[2], s, quiet);
				ok = 0;
			}
		}
	}

	return ok;
}

sg.resetNewEditor = function() {
	var f, c;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	
	if ((c = cookie.get('addstatic')) != null) {
		cookie.set('addstatic', '', 0);
		c = c.split(',');
		if (c.length == 3) {
			f[0].value = c[0];
			f[1].value = c[1];
			f[2].value = c[2];
			return;
		}
	}

	f[0].value = '00:00:00:00:00:00';
	f[2].value = '';

	var n = f[1].value;
	if (n.indexOf('.') == -1) {
		if (n == '') {
			n = 1;
		}
		else {
			n = parseInt(n, 10);
			if (isNaN(n)) return;
		}
		for (var i = 254; i > 0; --i) {
			if ((!this.inDynamic(n)) && (!this.inStatic(n)) && (n != router)) {
				f[1].value = ipp + n;
				return;
			}
			++n;
			if (n > 254) n = 1;
		}
		f[1].value = '';
	}
}

sg.setup = function()
{
	this.init('bs-grid', 'sort', 100,
		[{ type: 'text', maxlen: 17 }, { type: 'text', maxlen: 15 }, { type: 'text', maxlen: 32 }]);
	this.headerSet(['MAC Address', 'IP Address', 'Hostname']);
	var s = nvram.dhcpd_static.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 3) this.insertData(-1, t);
	}
	this.sort(2);
	this.showNewEditor();
	this.resetNewEditor();
}

function save()
{
	if (sg.isEditing()) return;

	var data = sg.getAllData();
	var sdhcp = '';
	var i;

	for (i = 0; i < data.length; ++i) {
		sdhcp += data[i].join('<') + '>';
	}

	var fom = E('_fom');
	fom.dhcpd_static.value = sdhcp;
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
<input type='hidden' name='_service' value='dhcpd-restart'>

<input type='hidden' name='dhcpd_static'>

<div class='section-title'>Static DHCP</div>
<div class='section'>
	<table class='tomato-grid' id='bs-grid'></table>
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
<script type='text/javascript'>sg.setup();</script>
</body>
</html>
