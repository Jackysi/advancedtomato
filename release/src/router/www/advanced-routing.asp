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
<title>[<% ident(); %>] Advanced: Routing</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#ara-grid .co1, #ara-grid .co2, #ara-grid .co3 {
	width: 15%;
}
#ara-grid .co4 {
	width: 6%;
}
#ara-grid .co5 {
	width: 49%;
}

#ars-grid .co1, #ars-grid .co2, #ars-grid .co3  {
	width: 15%;
}
#ars-grid .co4 {
	width: 6%;
}
#ars-grid .co5 {
	width: 10%;
}
#ars-grid .co6 {
	width: 39%;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>
// <% nvram("wk_mode,dr_setting,lan_stp,routes_static,lan_ifname,wan_ifname"); %>
// <% activeroutes(); %>

var ara = new TomatoGrid();

ara.setup = function() {
	var i, a;

	this.init('ara-grid', 'sort');
	this.headerSet(['Destination', 'Gateway', 'Subnet Mask', 'Metric', 'Interface']);
	for (i = 0; i < activeroutes.length; ++i) {
		a = activeroutes[i];
		if (a[0] == nvram.lan_ifname) a[0] += ' (LAN)';
			else if (a[0] == nvram.wan_ifname) a[0] += ' (WAN)';
		this.insertData(-1, [a[1],a[2],a[3],a[4],a[0]]);
	}
}


var ars = new TomatoGrid();

ars.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	f[5].value = f[5].value.replace('>', '_');
	return v_ip(f[0], quiet) && v_ip(f[1], quiet) && v_netmask(f[2], quiet) && v_range(f[3], quiet, 0, 10);
}

ars.setup = function() {
	this.init('ars-grid', '', 20, [
		{ type: 'text', maxlen: 15 }, { type: 'text', maxlen: 15 }, { type: 'text', maxlen: 15 },
		{ type: 'text', maxlen: 3 }, { type: 'select', options: [['LAN','LAN'],['WAN','WAN']] }, { type: 'text', maxlen: 32 }]);
	this.headerSet(['Destination', 'Gateway', 'Subnet Mask', 'Metric', 'Interface', 'Description']);
	var routes = nvram.routes_static.split('>');
	for (var i = 0; i < routes.length; ++i) {
		var r;
		if (r = routes[i].match(/^(.+)<(.+)<(.+)<(\d+)<(LAN|WAN)<(.*)$/)) {
			this.insertData(-1, [r[1], r[2], r[3], r[4], r[5],r[6]]);
		}
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
	if (ars.isEditing()) return;

	var fom = E('_fom');
	var data = ars.getAllData();
	var r = [];
	for (var i = 0; i < data.length; ++i) r.push(data[i].join('<'));
	fom.routes_static.value = r.join('>');

	var wan = '0';
	var lan = '0';
	switch (E('_dr_setting').value) {
	case '1':
		lan = '1 2';
		break;
	case '2':
		wan = '1 2';
		break;
	case '3':
		lan = '1 2';
		wan = '1 2';
		break;
	}
	fom.dr_lan_tx.value = fom.dr_lan_rx.value = lan;
	fom.dr_wan_tx.value = fom.dr_wan_rx.value = wan;

	fom.lan_stp.value = E('_f_stp').checked ? 1 : 0;

	form.submit(fom, 1);
}

function earlyInit()
{
	ara.setup();
	ars.setup();
}

function init()
{
	ara.recolor();
	ars.recolor();
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

<input type='hidden' name='_nextpage' value='advanced-routing.asp'>
<input type='hidden' name='_service' value='routing-restart'>

<input type='hidden' name='routes_static'>
<input type='hidden' name='lan_stp'>
<input type='hidden' name='dr_lan_tx'>
<input type='hidden' name='dr_lan_rx'>
<input type='hidden' name='dr_wan_tx'>
<input type='hidden' name='dr_wan_rx'>

<div class='section-title'>Current Routing Table</div>
<div class='section'>
	<table class='tomato-grid' id='ara-grid'></table>
</div>

<div class='section-title'>Static Routing Table</div>
<div class='section'>
	<table class='tomato-grid' id='ars-grid'></table>
</div>

<div class='section-title'>Miscellaneous</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Mode', name: 'wk_mode', type: 'select', options: [['gateway','Gateway'],['router','Router']], value: nvram.wk_mode },
	{ title: 'RIPv1 &amp; v2', name: 'dr_setting', type: 'select',	options: [[0,'Disabled'],[1,'LAN'],[2,'WAN'],[3,'Both']], value:	nvram.dr_setting },
	{ title: 'Spanning-Tree Protocol', name: 'f_stp', type: 'checkbox', value: nvram.lan_stp != '0' },
]);
</script>
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
<script type='text/javascript'>earlyInit()</script>
</body>
</html>

