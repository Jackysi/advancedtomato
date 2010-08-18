<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Status: Web Usage</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>

#dom-grid .co1 {
	width: 35%;
}
#dom-grid .co2 {
	width: 17%;
}
#dom-grid .co3 {
	width: 48%;
}

#srh-grid .co1 {
	width: 35%;
}
#srh-grid .co2 {
	width: 17%;
}
#srh-grid .co3 {
	width: 48%;
}

</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("log_wm,log_wmdmax,log_wmsmax"); %>
//	<% webmon(); %>

var list = [];

var ref = new TomatoRefresh('update.cgi', 'exec=webmon', 0, 'status_webmon_refresh');

ref.refresh = function(text) {
	try {
		eval(text);
	}
	catch (ex) {
		return;
	}

	dg.populate();
	sg.populate();
}

function monitorGridPopulate(grid, data, url)
{
	var a, e, i;
	var maxl = 45;

	list = [];
	grid.removeAllData();
	for (i = 0; i < list.length; ++i) {
		list[i].time = 0;
		list[i].ip = '';
		list[i].value = '';
	}

	for (i = data.length - 1; i >= 0; --i) {
		a = data[i];
		e = {
			time: a[0],
			ip: a[1],
			value: a[2] + ''
		};
		list.push(e);
	}

	var dt = new Date();
	for (i = list.length - 1; i >= 0; --i) {
		e = list[i];
		if (url != 0) {
			e.value = '<a href="http://' + e.value + '" target="_new">' +
				(e.value.length > maxl + 3 ? e.value.substr(0, maxl) + '...' : e.value) + '</a>';
		}
		else {
			e.value = e.value.replace(/\+/g, ' ');
			if (e.value.length > maxl + 3)
				e.value = e.value.substr(0, maxl) + '...';
		}
		dt.setTime(e.time * 1000);
		grid.insert(-1, e, [dt.toDateString() + ', ' + dt.toLocaleTimeString(),
			e.ip, e.value], false);
	}

	list = [];
	grid.resort();
}

function monitorGridCompare(grid, a, b)
{
	var col = grid.sortColumn;
	var ra = a.getRowData();
	var rb = b.getRowData();
	var r;

	switch (col) {
	case 0:
		r = cmpInt(ra.time, rb.time);
		break;
	default:
		r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
	}
	return grid.sortAscending ? r : -r;
}

var dg = new TomatoGrid();

dg.setup = function() {
	this.init('dom-grid', 'sort');
	this.headerSet(['Last Access Time', 'IP Address', 'Domain Name']);
	this.sort(0);
}

dg.populate = function() {
	monitorGridPopulate(this, wm_domains, 1);
}

dg.sortCompare = function(a, b) {
	return monitorGridCompare(this, a, b);
}

var sg = new TomatoGrid();

sg.setup = function() {
	this.init('srh-grid', 'sort');
	this.headerSet(['Search Time', 'IP Address', 'Search Criteria']);
	this.sort(0);
}

sg.populate = function() {
	monitorGridPopulate(this, wm_searches, 0);
}

sg.sortCompare = function(a, b) {
	return monitorGridCompare(this, a, b);
}

function init()
{
	dg.recolor();
	sg.recolor();

	dg.populate();
	sg.populate();

	ref.initPage();
}

function earlyInit()
{
	if (nvram.log_wm == '1' && (nvram.log_wmdmax != '0' || nvram.log_wmsmax != '0')) {
		E('webmon').style.display = '';
		E('wm-disabled').style.display = 'none';
		if (nvram.log_wmdmax == '0') E('webmon-domains').style.display = 'none';
		if (nvram.log_wmsmax == '0') E('webmon-searches').style.display = 'none';
		dg.setup();
		sg.setup();
	}
}
</script>

</head>
<body onload='init()'>
<form id='_fom' action='javascript:{}'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<div id='webmon' style='display:none'>
	<div id='webmon-domains'>
		<div class='section-title'>Recently Visited Web Sites</div>
		<div class='section'>
			<table id='dom-grid' class='tomato-grid' cellspacing=0></table>
			&raquo; <a href="webmon_recent_domains?_http_id=<% nv(http_id) %>">Download</a><br>
		</div>
	</div>

	<div id='webmon-searches'>
		<div class='section-title'>Recent Web Searches</div>
		<div class='section'>
			<table id='srh-grid' class='tomato-grid' cellspacing=0></table>
			&raquo; <a href="webmon_recent_searches?_http_id=<% nv(http_id) %>">Download</a><br>
		</div>
	</div>

	<div id='webmon-controls'>
		<script type='text/javascript'>genStdRefresh(1,0,'ref.toggle()');</script>
		<div class='section'>
			<br>&raquo; <a href="admin-log.asp">Web Monitor Configuration</a><br><br>
		</div>
	</div>
</div>

<div id='wm-disabled'>
	<b>Web Monitoring disabled.</b>
	<br><br>
	<a href="admin-log.asp">Enable &raquo;</a>
	<br><br>
</div>

<script type='text/javascript'>earlyInit();</script>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>&nbsp;</td></tr>
</table>
</form>
</body>
</html>
