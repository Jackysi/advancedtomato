<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Tomato VLAN GUI
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Status: Device List</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
#dev-grid .co1 {
	width: 8%;
}
#dev-grid .co2 {
	width: 20%;
}
#dev-grid .co3 {
	width: 13%;
}
#dev-grid .co4 {
	width: 21%;
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
	width: 9%;
	text-align: right;
}
#dev-grid .co8 {
	width: 13%;
	text-align: right;
}
#dev-grid .header {
	text-align: left;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript' src='wireless.jsx?_http_id=<% nv(http_id); %>'></script>
<script type='text/javascript'>

ipp = '<% lipp(); %>.';
//<% nvram('lan_ifname,wl_ifname,wl_mode,wl_radio'); %>
//	<% devlist(); %>

list = [];

function find(mac, ip)
{
	var e, i;

	mac = mac.toUpperCase();
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		if (((e.mac == mac) && ((e.ip == ip) || (e.ip == '') || (ip == null))) ||
			((e.mac == '00:00:00:00:00:00') && (e.ip == ip))) {
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
		unit: 0,
		name: '',
		rssi: '',
		txrx: '',
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

function addWF(n)
{
	var e = list[n];
	cookie.set('addmac', [e.mac, e.name.split(',')[0]].join(','), 1);
	location.href = 'basic-wfilter.asp';
}

function addbwlimit(n)
{
	var e = list[n];
	cookie.set('addbwlimit', [e.ip, e.name.split(',')[0]].join(','), 1);
	location.href = 'bwlimit.asp';
}

var ref = new TomatoRefresh('update.cgi', 'exec=devlist', 0, 'status_devices_refresh');

ref.refresh = function(text)
{
	eval(text);
	dg.removeAllData();
	dg.populate();
	dg.resort();
	for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
		if (wl_sunit(uidx)<0)
			E("noise"+uidx).innerHTML = wlnoise[uidx];
	}
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
	var i, j;
	var a, b, c, e;

	list = [];

	for (i = 0; i < list.length; ++i) {
		list[i].ip = '';
		list[i].ifname = '';
		list[i].unit = 0;
		list[i].name = '';
		list[i].rssi = '';
		list[i].txrx = '';
		list[i].lease = '';
	}

	for (i = dhcpd_lease.length - 1; i >= 0; --i) {
		a = dhcpd_lease[i];
		e = get(a[2], a[1]);
		e.lease = '<small><a href="javascript:deleteLease(\'L' + i + '\',\'' + a[1] + '\')" title="Delete Lease" id="L' + i + '">' + a[3] + '</a></small>';
		e.name = a[0];
		e.ifname = '';
	}

	for (i = wldev.length - 1; i >= 0; --i) {
		a = wldev[i];
		if (a[0].indexOf('wds') == 0) {
			e = get(a[1], '-');
		}
		else {
			e = get(a[1], null);
		}
		e.ifname = a[0];
		e.unit = a[6] * 1;
		e.rssi = a[2];

		if ((a[3] > 1000) || (a[4] > 1000))
			e.txrx = ((a[3] > 1000) ? Math.round(a[3] / 1000) : '-') + ' / ' + ((a[4] > 1000) ? Math.round(a[4] / 1000) : '-'); //+ '<br><small>Mbps</small>';
	}

	for (i = arplist.length - 1; i >= 0; --i) {
		a = arplist[i];

		if ((e = get(a[1], a[0])) != null) {
			if (e.ifname == '') e.ifname = a[2];
		}
	}

	for (i = dhcpd_static.length - 1; i >= 0; --i) {
		a = dhcpd_static[i].split('<');
		if (a.length < 3) continue;

		if (a[1].indexOf('.') == -1) a[1] = (ipp + a[1]);

		c = a[0].split(',');
		for (j = c.length - 1; j >= 0; --j) {
			if ((e = find(c[j], a[1])) != null) break;
		}
		if (j < 0) continue;

		if (e.ip == '') {
			e.ip = a[1];
		}

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

		b = e.mac;
		if (e.mac.match(/^(..):(..):(..)/)) {
			b += '<br><small>' +
				'<a href="http://standards.ieee.org/cgi-bin/ouisearch?' + RegExp.$1 + '-' + RegExp.$2 + '-' + RegExp.$3 + '" target="_new" title="OUI Search">[oui]</a> ' +
				'<a href="javascript:addStatic(' + i + ')" title="Static Lease...">[static]</a> ' +
				'<a href="javascript:addbwlimit(' + i + ')" title="BW Limiter">[bwlimit]</a>';

			if (e.rssi != '') {
				b += ' <a href="javascript:addWF(' + i + ')" title="Wireless Filter...">[wfilter]</a>';
			}
			b += '</small>';
		}
		else {
			b = '';
		}

		var ifidx = wl_uidx(e.unit);
		if ((e.rssi !== '') && (ifidx >= 0) && (wlnoise[ifidx] < 0)) {
			e.qual = MAX(e.rssi - wlnoise[ifidx], 0);
		}
		else {
			e.qual = -1;
		}

		this.insert(-1, e, [
			e.ifname, b, (e.ip == '-') ? '' : e.ip, e.name,
			(e.rssi != 0) ? e.rssi + ' <small>dBm</small>' : '',
			(e.qual < 0) ? '' : '<small>' + e.qual + '</small> <img src="bar' + MIN(MAX(Math.floor(e.qual / 10), 1), 6) + '.gif">',
			e.txrx,	e.lease], false);
	}
}

dg.setup = function()
{
	this.init('dev-grid', 'sort');
	this.headerSet(['Interface', 'MAC Address', 'IP Address', 'Name', 'RSSI &nbsp; &nbsp; ', 'Quality', 'TX/RX Rate&nbsp;', 'Lease &nbsp; &nbsp; ']);
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
f = [];
for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	var u = wl_unit(uidx);
	if (nvram['wl'+u+'_radio'] == '1') {
		if (wl_sunit(uidx)<0) {
			var a = '';
			if ((nvram['wl'+u+'_mode'] == 'ap') || (nvram['wl'+u+'_mode'] == 'wds'))
				a = '&nbsp;&nbsp;&nbsp; <input type="button" value="Measure" onclick="javascript:window.location=\'wlmnoise.cgi?_http_id=' + nvram.http_id + '&_wl_unit=' + u +'\'">';
			f.push( { title: '<b>Noise Floor (' + wl_ifaces[uidx][0] + ')&nbsp;:</b>',
				prefix: '<span id="noise'+uidx+'">',
				custom: wlnoise[uidx],
				suffix: '</span>&nbsp;<small>dBm</small>' + a } );
		}
	}
}
createFieldTable('', f);
</script>

</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2><script type='text/javascript'>genStdRefresh(1,0,'ref.toggle()');</script></td></tr>
</table>
<script type='text/javascript'>earlyInit();</script>
</body>
</html>

