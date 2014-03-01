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
<title>[<% ident(); %>] Advanced: Routing</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#ara-grid .co1, #ara-grid .co2, #ara-grid .co3 {
	width: 20%;
}
#ara-grid .co4 {
	width: 6%;
}
#ara-grid .co5 {
	width: 34%;
}

#ars-grid .co1, #ars-grid .co2, #ars-grid .co3  {
	width: 20%;
}
#ars-grid .co4 {
	width: 6%;
}
#ars-grid .co5 {
	width: 10%;
}
#ars-grid .co6 {
	width: 24%;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>
// <% nvram("wk_mode,dr_setting,lan_stp,routes_static,dhcp_routes,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,wan_ifname,wan_iface,emf_enable,dr_lan_rx,dr_lan1_rx,dr_lan2_rx,dr_lan3_rx,dr_wan_rx,wan_proto"); %>
// <% activeroutes(); %>

var ara = new TomatoGrid();

ara.setup = function() {
	var i, a;

	this.init('ara-grid', 'sort');
	this.headerSet(['Destination', 'Gateway / Next Hop', 'Subnet Mask', 'Metric', 'Interface']);
	for (i = 0; i < activeroutes.length; ++i) {
		a = activeroutes[i];
		if (a[0] == nvram.lan_ifname) a[0] += ' (LAN)';
			else if (a[0] == nvram.lan1_ifname) a[0] += ' (LAN1)';
			else if (a[0] == nvram.lan2_ifname) a[0] += ' (LAN2)';
			else if (a[0] == nvram.lan3_ifname) a[0] += ' (LAN3)';
			else if (a[0] == nvram.wan_iface) a[0] += ' (WAN)';
			else if (a[0] == nvram.wan_ifname) a[0] += ' (MAN)';
		this.insertData(-1, [a[1],a[2],a[3],a[4],a[0]]);
	}
}

var ars = new TomatoGrid();

ars.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);
	f[5].value = f[5].value.replace('>', '_');
	return v_ip(f[0], quiet) && v_ip(f[1], quiet) && v_netmask(f[2], quiet) && v_range(f[3], quiet, 0, 10) && v_nodelim(f[5], quiet, 'Description');
}

ars.setup = function() {
	this.init('ars-grid', '', 20, [
		{ type: 'text', maxlen: 15 }, { type: 'text', maxlen: 15 }, { type: 'text', maxlen: 15 },
		{ type: 'text', maxlen: 3 }, { type: 'select', options: [['LAN','LAN'],['LAN1','LAN1'],['LAN2','LAN2'],['LAN3','LAN3'],['WAN','WAN'],['MAN','MAN']] }, { type: 'text', maxlen: 32 }]);

	this.headerSet(['Destination', 'Gateway', 'Subnet Mask', 'Metric', 'Interface', 'Description']);
	var routes = nvram.routes_static.split('>');
	for (var i = 0; i < routes.length; ++i) {
		var r;
		if (r = routes[i].match(/^(.+)<(.+)<(.+)<(\d+)<(LAN|LAN1|LAN2|LAN3|WAN|MAN)<(.*)$/)) {
			this.insertData(-1, [r[1], r[2], r[3], r[4], r[5],r[6]]);
		}
	}
	this.showNewEditor();
	this.resetNewEditor();
}

ars.resetNewEditor = function() {
	var i, e;

	e = fields.getAll(this.newEditor);

	if(nvram.lan_ifname.length < 1)
		e[4].options[0].disabled=true;
	else
		e[4].options[0].disabled=false;
	if(nvram.lan1_ifname.length < 1)
		e[4].options[1].disabled=true;
	else
		e[4].options[1].disabled=false;
	if(nvram.lan2_ifname.length < 1)
		e[4].options[2].disabled=true;
	else
		e[4].options[2].disabled=false;
	if(nvram.lan3_ifname.length < 1)
		e[4].options[3].disabled=true;
	else
		e[4].options[3].disabled=false;

	ferror.clearAll(e);
	for (i = 0; i < e.length; ++i) {
		var f = e[i];
		if (f.selectedIndex) f.selectedIndex = 0;
			else f.value = '';
	}
	try { if (e.length) e[0].focus(); } catch (er) { }
}

function verifyFields(focused, quiet)
{
	E('_f_dr_lan').disabled = (nvram.lan_ifname.length < 1);
	if (E('_f_dr_lan').disabled)
		E('_f_dr_lan').checked = false;
	E('_f_dr_lan1').disabled = (nvram.lan1_ifname.length < 1);
	if (E('_f_dr_lan1').disabled)
		E('_f_dr_lan1').checked = false;
	E('_f_dr_lan2').disabled = (nvram.lan2_ifname.length < 1);
	if (E('_f_dr_lan2').disabled)
		E('_f_dr_lan2').checked = false;
	E('_f_dr_lan3').disabled = (nvram.lan3_ifname.length < 1);
	if (E('_f_dr_lan3').disabled)
		E('_f_dr_lan3').checked = false;
	E('_f_dr_wan').disabled = (nvram.wan_proto.length == 'disabled');
	if (E('_f_dr_wan').disabled)
		E('_f_dr_wan').checked = false;
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

/* ZEBRA-BEGIN */

	fom.dr_lan_tx.value = fom.dr_lan_rx.value = (E('_f_dr_lan').checked) ? '1 2' : '0';
	fom.dr_lan1_tx.value = fom.dr_lan1_rx.value = (E('_f_dr_lan1').checked) ? '1 2' : '0';
	fom.dr_lan2_tx.value = fom.dr_lan2_rx.value = (E('_f_dr_lan2').checked) ? '1 2' : '0';
	fom.dr_lan3_tx.value = fom.dr_lan3_rx.value = (E('_f_dr_lan3').checked) ? '1 2' : '0';
	fom.dr_wan_tx.value = fom.dr_wan_rx.value = (E('_f_dr_wan').checked) ? '1 2' : '0';

/* ZEBRA-END */

	fom.dhcp_routes.value = E('_f_dhcp_routes').checked ? '1' : '0';
	fom._service.value = (fom.dhcp_routes.value != nvram.dhcp_routes) ? 'wan-restart' : 'routing-restart';

/* EMF-BEGIN */
	fom.emf_enable.value = E('_f_emf').checked ? 1 : 0;
	if (fom.emf_enable.value != nvram.emf_enable) fom._service.value = '*';
/* EMF-END */

	form.submit(fom, 1);
}

function submit_complete()
{
	reloadPage();
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
<input type='hidden' name='dhcp_routes'>
<input type='hidden' name='emf_enable'>
<input type='hidden' name='dr_lan_tx'>
<input type='hidden' name='dr_lan_rx'>
<input type='hidden' name='dr_lan1_tx'>
<input type='hidden' name='dr_lan1_rx'>
<input type='hidden' name='dr_lan2_tx'>
<input type='hidden' name='dr_lan2_rx'>
<input type='hidden' name='dr_lan3_tx'>
<input type='hidden' name='dr_lan3_rx'>
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
/* ZEBRA-BEGIN */
	{ title: 'RIPv1 &amp; v2' },
	{ title: 'LAN', indent: 2, name: 'f_dr_lan', type: 'checkbox', value: ((nvram.dr_lan_rx != '0') && (nvram.dr_lan_rx != '')) },
	{ title: 'LAN1', indent: 2, name: 'f_dr_lan1', type: 'checkbox', value: ((nvram.dr_lan1_rx != '0') && (nvram.dr_lan1_rx != '')) },
	{ title: 'LAN2', indent: 2, name: 'f_dr_lan2', type: 'checkbox', value: ((nvram.dr_lan2_rx != '0') && (nvram.dr_lan2_rx != '')) },
	{ title: 'LAN3', indent: 2, name: 'f_dr_lan3', type: 'checkbox', value: ((nvram.dr_lan3_rx != '0') && (nvram.dr_lan3_rx != '')) },
	{ title: 'WAN', indent: 2, name: 'f_dr_wan', type: 'checkbox', value: ((nvram.dr_wan_rx != '0') && (nvram.dr_wan_rx != '')) },
/* ZEBRA-END */
/* EMF-BEGIN */
	{ title: 'Efficient Multicast Forwarding', name: 'f_emf', type: 'checkbox', value: nvram.emf_enable != '0' },
/* EMF-END */
	{ title: 'DHCP Routes', name: 'f_dhcp_routes', type: 'checkbox', value: nvram.dhcp_routes != '0' },
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
<script type='text/javascript'>earlyInit(); verifyFields(null, 1);</script>
</body>
</html>

