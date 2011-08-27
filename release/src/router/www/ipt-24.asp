<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	IP Traffic enhancements
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] IP Traffic: Last 24 Hours</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
#txt {
	width: 550px;
	white-space: nowrap;
}
#bwm-controls {
	text-align: right;
	margin-right: 5px;
	margin-top: 5px;
	float: right;
	visibility: hidden;
}
ul.tabs a,
#tabs a {
	width: 140px;
	height: 12px;
	font-size: 9px;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript' src='wireless.jsx?_http_id=<% nv(http_id); %>'></script>
<script type='text/javascript' src='bwm-common.js'></script>
<script type='text/javascript' src='bwm-hist.js'></script>
<script type='text/javascript' src='interfaces.js'></script>

<script type='text/javascript'>

//	<% nvram("wan_ifname,lan_ifname,wl_ifname,wan_proto,wan_iface,web_svg,cstats_enable,cstats_colors,dhcpd_static,lan_ipaddr,lan_netmask,lan1_ipaddr,lan1_netmask,lan2_ipaddr,lan2_netmask,lan3_ipaddr,lan3_netmask"); %>

//	<% devlist(); %>

//<% sysinfo(); %>

var cprefix = 'ipt_24';
var updateInt = 120;
var updateDiv = updateInt;
var updateMaxL = 720;
var updateReTotal = 1;
var hours = 24;
var lastHours = 0;
var debugTime = 0;

function showHours() {
	if (hours == lastHours) return;
	showSelectedOption('hr', lastHours, hours);
	lastHours = hours;
}

function switchHours(h) {
	if ((!svgReady) || (updating)) return;

	hours = h;
	updateMaxL = (720 / 24) * hours;
	showHours();
	loadData();
	cookie.set(cprefix + 'hrs', hours);
}

var ref = new TomatoRefresh('update.cgi', 'exec=ipt_bandwidth&arg0=speed');

ref.refresh = function(text) {
	++updating;
	try {
		this.refreshTime = 1500;
		speed_history = {};
		try {
			eval(text);
			if (cstats_busy) {
				E('rbusy').style.display = 'none';
				cstats_busy = 0;
			}
			this.refreshTime = (fixInt(speed_history._next, 1, 120, 60) + 2) * 1000;
		} catch (ex) {
			speed_history = {};
			cstats_busy = 1;
			E('rbusy').style.display = '';
		}
		if (debugTime) E('dtime').innerHTML = (ymdText(new Date())) + ' ' + (this.refreshTime / 1000);
		loadData();
	}
	catch (ex) {
/* REMOVE-BEGIN
//		alert('ex=' + ex);
REMOVE-END */
	}
	--updating;
}

ref.showState = function() {
	E('refresh-button').value = this.running ? 'Stop' : 'Start';
}

ref.toggleX = function() {
	this.toggle();
	this.showState();
	cookie.set(cprefix + 'refresh', this.running ? 1 : 0);
}

ref.initX = function() {
	var a;

	a = fixInt(cookie.get(cprefix + 'refresh'), 0, 1, 1);
	if (a) {
		ref.refreshTime = 100;
		ref.toggleX();
	}
}

function init() {
	if (nvram.cstats_enable != '1') return;

	try {
	//	<% ipt_bandwidth("speed"); %>
	}
	catch (ex) {
/* REMOVE-BEGIN
//		speed_history = {};
REMOVE-END */
	}
	cstats_busy = 0;
	if (typeof(speed_history) == 'undefined') {
		speed_history = {};
		cstats_busy = 1;
		E('rbusy').style.display = '';
	}

	populateCache();

	hours = fixInt(cookie.get(cprefix + 'hrs'), 1, 24, 24);
	updateMaxL = (720 / 24) * hours;
	showHours();

	initCommon(1, 0, 0);
	ref.initX();
}
</script>

</head>
<body onload='init()'>
<form>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<div id='cstats'>
	<div id='tab-area'></div>

	<script type='text/javascript'>
	if ((nvram.web_svg != '0') && (nvram.cstats_enable == '1')) {
		// without a div, Opera 9 moves svgdoc several pixels outside of <embed> (?)
		W("<div style='border-top:1px solid #f0f0f0;border-bottom:1px solid #f0f0f0;visibility:hidden;padding:0;margin:0' id='graph'><embed src='bwm-graph.svg?<% version(); %>' style='width:760px;height:300px;margin:0;padding:0' type='image/svg+xml' pluginspage='http://www.adobe.com/svg/viewer/install/'></embed></div>");
	}
	</script>

	<div id='bwm-controls'>
		<small>(2 minute interval)</small><br>
		<br>
		Hours:&nbsp;
			<a href='javascript:switchHours(1);' id='hr1'>1</a>,
			<a href='javascript:switchHours(2);' id='hr2'>2</a>,
			<a href='javascript:switchHours(4);' id='hr4'>4</a>,
			<a href='javascript:switchHours(6);' id='hr6'>6</a>,
			<a href='javascript:switchHours(12);' id='hr12'>12</a>,
			<a href='javascript:switchHours(18);' id='hr18'>18</a>,
			<a href='javascript:switchHours(24);' id='hr24'>24</a><br>
		Avg:&nbsp;
			<a href='javascript:switchAvg(1)' id='avg1'>Off</a>,
			<a href='javascript:switchAvg(2)' id='avg2'>2x</a>,
			<a href='javascript:switchAvg(4)' id='avg4'>4x</a>,
			<a href='javascript:switchAvg(6)' id='avg6'>6x</a>,
			<a href='javascript:switchAvg(8)' id='avg8'>8x</a><br>
		Max:&nbsp;
			<a href='javascript:switchScale(0)' id='scale0'>Uniform</a>,
			<a href='javascript:switchScale(1)' id='scale1'>Per Address</a><br>
		Display:&nbsp;
			<a href='javascript:switchDraw(0)' id='draw0'>Solid</a>,
			<a href='javascript:switchDraw(1)' id='draw1'>Line</a><br>
		Color:&nbsp; <a href='javascript:switchColor()' id='drawcolor'>-</a><br>
		<small><a href='javascript:switchColor(1)' id='drawrev'>[reverse]</a></small><br>
		<br><br>
		&nbsp; &raquo; <a href="admin-iptraffic.asp">Configure</a>
	</div>

	<br><br>
	<table border=0 cellspacing=2 id='txt'>
	<tr>
		<td width='8%' align='right' valign='top'><b style='border-bottom:blue 1px solid' id='rx-name'>RX</b></td>
			<td width='15%' align='right' valign='top'><span id='rx-current'></span></td>
		<td width='8%' align='right' valign='top'><b>Avg</b></td>
			<td width='15%' align='right' valign='top' id='rx-avg'></td>
		<td width='8%' align='right' valign='top'><b>Peak</b></td>
			<td width='15%' align='right' valign='top' id='rx-max'></td>
		<td width='8%' align='right' valign='top'><b>Total</b></td>
			<td width='14%' align='right' valign='top' id='rx-total'></td>
		<td>&nbsp;</td>
	</tr>
	<tr>
		<td width='8%' align='right' valign='top'><b style='border-bottom:blue 1px solid' id='tx-name'>TX</b></td>
			<td width='15%' align='right' valign='top'><span id='tx-current'></span></td>
		<td width='8%' align='right' valign='top'><b>Avg</b></td>
			<td width='15%' align='right' valign='top' id='tx-avg'></td>
		<td width='8%' align='right' valign='top'><b>Peak</b></td>
			<td width='15%' align='right' valign='top' id='tx-max'></td>
		<td width='8%' align='right' valign='top'><b>Total</b></td>
			<td width='14%' align='right' valign='top' id='tx-total'></td>
		<td>&nbsp;</td>
	</tr>
	</table>
</div>
<br>

<script type='text/javascript'>
if (nvram.cstats_enable != '1') {
	W('<div class="note-disabled">IP Traffic monitoring disabled.</b><br><br><a href="admin-iptraffic.asp">Enable &raquo;</a><div>');
	E('cstats').style.display = 'none';
} else if (sysinfo.uptime < 300) {
	W('<div class="note-disabledw">No data to show just yet.<br>Try reloading after a few minutes.<div>');
	E('cstats').style.display = 'none';
}else {
	W('<div class="note-warning" style="display:none" id="rbusy">The cstats program is not responding or is busy. Try reloading after a few seconds.</div>');
}
</script>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='dtime'></span>
	<img src='spin.gif' id='refresh-spinner' onclick='debugTime=1'>
	<input type='button' value='Refresh' id='refresh-button' onclick='ref.toggleX()'>
</td></tr>
</table>
</form>
</body>
</html>
