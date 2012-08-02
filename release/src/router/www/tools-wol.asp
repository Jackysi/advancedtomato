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
<title>[<% ident(); %>] Tools: WOL</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<style type='text/css'>
#wol-grid .co1 {
	width: 15%;
}
#wol-grid .co2 {
	width: 15%;
}
#wol-grid .co3 {
	width: 25%;
}
#wol-grid .co4 {
	width: 45%;
}
textarea {
	width: 99%;
	height: 8em;
}
#refreshb, #save-button {
	width: 90px;
}
</style>

<script type='text/javascript'>

//	<% arplist(); %>
//	<% nvram('dhcpd_static,lan_ifname'); %>

var wg = new TomatoGrid();
wg.setup = function() {
	this.init('wol-grid', 'sort');
	this.headerSet(['MAC Address', 'IP Address', 'Status', 'Name']);
	this.sort(3);
}
wg.sortCompare = function(a, b) {
        var da = a.getRowData();
        var db = b.getRowData();
        var r = 0;
        var c = this.sortColumn;
        if (c == 1)
                r = cmpIP(da[c], db[c]);
        else
                r = cmpText(da[c], db[c]);
        return this.sortAscending ? r : -r;
}
wg.populate = function()
{
	var i, j, r, s;

	this.removeAllData();

	s = [];
	var q = nvram.dhcpd_static.split('>');
	for (i = 0; i < q.length; ++i) {
		var e = q[i].split('<');
		if ((e.length == 3) || (e.length == 4)) {
			var m = e[0].split(',');
			for (j = 0; j < m.length; ++j) {
				s.push([m[j], e[1], e[2]]);
			}
		}
	}

	// show entries in static dhcp list
	for (i = 0; i < s.length; ++i) {
		var t = s[i];
		var active = '-';
		for (j = 0; j < arplist.length; ++j) {
			if ((arplist[j][2] == nvram.lan_ifname) && (t[0] == arplist[j][1])) {
				active = 'Active (In ARP)';
				arplist[j][1] = '!';
				break;
			}
		}
		if (t.length == 3) {
			r = this.insertData(-1, [t[0], (t[1].indexOf('.') != -1) ? t[1] : ('<% lipp(); %>.' + t[1]), active, t[2]]);
			for (j = 0; j < 4; ++j)
				r.cells[j].title = 'Click to wake up';
		}
	}

	// show anything else in ARP that is awake
	for (i = 0; i < arplist.length; ++i) {
		if ((arplist[i][2] != nvram.lan_ifname) || (arplist[i][1].length != 17)) continue;
		r = this.insertData(-1, [arplist[i][1], arplist[i][0], 'Active (In ARP)', '']);
		for (j = 0; j < 4; ++j)
			r.cells[j].title = 'Click to wake up';
	}

	this.resort(2);
}
wg.onClick = function(cell)
{
	wake(PR(cell).getRowData()[0]);
}

function verifyFields(focused, quiet)
{
	var e;

	e = E('_f_mac');
	e.value = e.value.replace(/[\t ]+/g, ' ');
	return 1;
}

function spin(x)
{
	E('refreshb').disabled = x;
	E('wakeb').disabled = x;
}

var waker = null;

function wake(mac)
{
	if (!mac) {
		if (!verifyFields(null, 1)) return;
		mac = E('_f_mac').value;
		cookie.set('wakemac', mac);
	}
	E('_mac').value = mac;
	form.submit('_fom', 1);
}



var refresher = null;
var timer = new TomatoTimer(refresh);
var running = 0;

function refresh()
{
	if (!running) return;

	timer.stop();

	refresher = new XmlHttp();
	refresher.onCompleted = function(text, xml) {
		eval(text);
		wg.populate();
		timer.start(5000);
		refresher = null;
	}
	refresher.onError = function(ex) { alert(ex); reloadPage(); }
	refresher.post('update.cgi', 'exec=arplist');
}

function refreshClick()
{
	running ^= 1;
	E('refreshb').value = running ? 'Stop' : 'Refresh';
	E('spin').style.visibility = running ? 'visible' : 'hidden';
	if (running) refresh();
}

function init()
{
	wg.recolor();
}
</script>

</head>
<body onload='init()'>
<form id='_fom' action='wakeup.cgi' method='post'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_redirect' value='tools-wol.asp'>
<input type='hidden' name='_nextwait' value='1'>
<input type='hidden' name='mac' value='' id='_mac'>

<div class='section-title'>Wake On LAN</div>
<div class='section'>
	<table id='wol-grid' class='tomato-grid' cellspacing=1></table>
	<div style='float:right'><img src='spin.gif' id='spin' style='vertical-align:middle;visibility:hidden'> &nbsp; <input type='button' value='Refresh' onclick='refreshClick()' id='refreshb'></div>
</div>
<div id='msg' style='visibility:hidden;background:#ffffa0;margin:auto;width:50%;text-align:center;padding:2px;border:1px solid #fee'></div>
<div class='section-title'></div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'MAC Address List', name: 'f_mac', type: 'textarea', value: cookie.get('wakemac') || '' },
]);
</script>
<div style='float:right'><input type='button' value='Wake Up' onclick='wake(null)' id='save-button'></div>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>&nbsp;</td></tr>
</table>
</form>
<script type='text/javascript'>wg.setup();wg.populate();</script>
</body>
</html>
