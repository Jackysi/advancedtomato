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
<title>[<% ident(); %>] Status: Device List</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
#dev-grid .co1 {
	width: 10%;
}
#dev-grid .co2 {
	width: 18%;
}
#dev-grid .co3 {
	width: 17%;
}
#dev-grid .co4 {
	width: 24%;
}
#dev-grid .co5 {
	width: 8%;
	text-align: right;
}
#dev-grid .co6 {
	width: 8%;
	text-align: center;
}
#dev-grid .co7 {
	width: 15%;
	text-align: right;
}
#dev-grid .header {
	text-align: left;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

ipp = '<% lipp(); %>.';
//<% nvram('lan_ifname,wl0_ifname,wl_mode,wl_radio'); %>
//	<% devlist(); %>

list = [];

function find(mac, ip)
{
	var e, i;
	
	mac = mac.toUpperCase();
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		if ((e.mac == mac) && ((e.ip == ip) || (e.ip == '') || (ip == null))) {
			return e;
		}
	}
	return null;
}

function get(mac, ip)
{
	var e, i;
	
	mac = mac.toUpperCase();
	if ((e = find(mac, ip)) != null) {
		if (ip) e.ip = ip;
		return e;
	}
	
	e = {
		mac: mac,
		ip: ip || '',
		ifname: '',
		name: '',
		rssi: '',
		lease: ''
	};
	list.push(e);

	return e;
}


var xob = null;

function _deleteLease(ip)
{
	form.submitHidden('dhcpd.cgi', { remove: ip });
}

function deleteLease(a, ip)
{
	if (xob) return;
	if ((xob = new XmlHttp()) == null) {
		_deleteLease(ip);
		return;
	}

	a = E(a);
	a.innerHTML = 'deleting...';

	xob.onCompleted = function(text, xml) {
		a.innerHTML = '...';
		xob = null;
	}
	xob.onError = function() {
		_deleteLease(ip);
	}

	xob.post('dhcpd.cgi', 'remove=' + ip);
}

function addStatic(n)
{
	var e = list[n];
	cookie.set('addstatic', [e.mac, e.ip, e.name.split(',')[0]].join(','), 1);
	location.href = 'basic-static.asp';
}



var ref = new TomatoRefresh('update.cgi', 'exec=devlist', 0, 'status_devices_refresh');

ref.refresh = function(text)
{
	eval(text);
	dg.removeAllData();
	dg.populate();
	dg.resort();
	E("noise").innerHTML = wlnoise;
}


var dg = new TomatoGrid();

dg.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var ra = a.getRowData();
	var rb = b.getRowData();
	var r;

	switch (col) {
	case 2:
		r = cmpIP(ra.ip, rb.ip);
		break;
	case 4:
		r = cmpInt(ra.rssi, rb.rssi);
		break;
	case 5:
		r = cmpInt(ra.qual, rb.qual);
		break;
	default:
		r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
	}
	if (r == 0) {
		r = cmpIP(ra.ip, rb.ip);
		if (r == 0) r = cmpText(ra.ifname, rb.ifname);
	}
	return this.sortAscending ? r : -r;
}

dg.populate = function()
{
	var i, a, b, c, e;

	list = [];

	for (i = 0; i < list.length; ++i) {
		list[i].ip = '';
		list[i].ifname = '';
		list[i].name = '';
		list[i].rssi = '';
		list[i].lease = '';
	}
	
	for (i = dhcpd_lease.length - 1; i >= 0; --i) {
		a = dhcpd_lease[i];
		e = get(a[2], a[1]);
		e.lease = '<small><a href="javascript:deleteLease(\'L' + i + '\',\'' + a[1] + '\')" title="Delete Lease" id="L' + i + '">' + a[3] + '</a></small>';
		e.name = a[0];
		e.ifname = nvram.lan_ifname;
	}

	for (i = wldev.length - 1; i >= 0; --i) {
		a = wldev[i];
		if (a[0].indexOf('wds') == 0) {
			e = get(a[1], '-');
			e.ifname = a[0];
		}
		else {
			e = get(a[1], null);
			e.ifname = nvram.wl0_ifname;
		}
		e.rssi = a[2];
	}

	for (i = arplist.length - 1; i >= 0; --i) {
		a = arplist[i];
		
		if ((e = get(a[1], a[0])) != null) {
			if (e.ifname == '') e.ifname = a[2];
		}
	}
	
	for (i = dhcpd_static.length - 1; i >= 0; --i) {
		a = dhcpd_static[i].split('<');
		if ((e = find(a[0], ipp + a[1])) == null) continue;
		if (e.name == '') {
			e.name = a[2];
		}
		else {
			b = e.name.toLowerCase();
			c = a[2].toLowerCase();
			if ((b.indexOf(c) == -1) && (c.indexOf(b) == -1)) {
				if (e.name != '') e.name += ', ';
				e.name += a[2];
			}
		}
	}

	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		
		if ((e.ip.length == 0) || (e.ip == '-')) {
			a = '';
		}
		else {
			a = '<a href="javascript:addStatic(' + i + ')" title="Add Static Lease">' + e.ip + '</a>';
		}

		if (e.mac.match(/^(..):(..):(..)/)) {
			b = "<a href='http://standards.ieee.org/cgi-bin/ouisearch?" + RegExp.$1 + "-" + RegExp.$2 + "-" + RegExp.$3 + "' target='_new' title='OUI Search'>" + e.mac + "</a>";
		}
		else {
			b = '';
		}

		if ((e.rssi !== '') && (wlnoise < 0)) {
			e.qual = MAX(e.rssi - wlnoise, 0);
		}
		else {
			e.qual = -1;
		}
		
		this.insert(-1, e, [
			e.ifname, b, a, e.name,
			(e.rssi != 0) ? e.rssi + ' <small>dBm</small>' : '',
			(e.qual < 0) ? '' : '<small>' + e.qual + '</small> <img src="bar' + MIN(MAX(Math.floor(e.qual / 10), 1), 6) + '.gif">',
			e.lease], false);
	}
}

dg.setup = function()
{
	this.init('dev-grid', 'sort');
	this.headerSet(['Interface', 'MAC Address', 'IP Address', 'Name', 'RSSI &nbsp; &nbsp; ', 'Quality', 'Lease &nbsp; &nbsp; ']);
	this.populate();
	this.sort(2);
}

function earlyInit()
{
	dg.setup();
}

function init()
{
	dg.recolor();
	ref.initPage(3000, 3);
}
</script>
</head>
<body onload='init()'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<div class='section-title'>Device List</div>
<div class='section'>
	<table id='dev-grid' class='tomato-grid' cellspacing=0></table>
<script type='text/javascript'>
if (nvram.wl_radio == '1') {
	W('<div style="float:left"><b>Noise Floor:</b> <span id="noise">' + wlnoise + '</span> <small>dBm</small>');
	if ((nvram.wl_mode == 'ap') || (nvram.wl_mode == 'wds')) {
		W(' &nbsp; <input type="button" value="Measure" onclick="javascript:window.location=\'wlmnoise.cgi?_http_id=' + nvram.http_id + '\'">');
	}
	W('</div>');
}
</script>

</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2><script type='text/javascript'>genStdRefresh(1,0,'ref.toggle()');</script></td></tr>
</table>
<script type='text/javascript'>earlyInit();</script>
</body>
</html>

