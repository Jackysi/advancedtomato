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
<title>[<% ident(); %>] Access Restrictions</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
#res-over-grid .co1 {
	width: 40%;
}
#res-over-grid .co2 {
	width: 60%;
}
#res-over-grid .footer {
	text-align: right;
}
#res-over-add {
	width: 100px;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>
//	<% nvram(''); %>	// http_id
//	<% nvramseq("rrules", "rrule%d", 0, 49); %>

var dowNames = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

var og = new TomatoGrid();
og.setup = function() {
	this.init('res-over-grid', 'sort');
	this.headerSet(['Description', 'Schedule']);
	var r = this.footerSet(['<input type="button" value="Add" onclick="TGO(this).addEntry()" id="res-over-add">']);
	r.cells[0].colSpan = 2;
}
og.populate = function() {
	this.removeAllData();
	for (var i = 0; i < rrules.length; ++i) {
		var v;
		if ((v = rrules[i].match(/^(\d+)\|(-?\d+)\|(-?\d+)\|(\d+)\|(.*?)\|(.*?)\|([^|]*?)\|(\d+)\|(.*)$/m)) == null)    {
			rrules[i] = '';
			continue;
		}
		v = v.slice(1);
		if (isNaN(v[1] *= 1)) continue;
		if (isNaN(v[2] *= 1)) continue;
		if (isNaN(v[3] *= 1)) continue;

		var s = '';
		if (v[3] == 0x7F) {
			s += 'Everyday';
		}
		else {
			for (var j = 0; j < 7; ++j) {
				if (v[3] & (1 << j)) {
					if (s.length) s += ', ';
					s += dowNames[j];
				}
			}
		}

		if ((v[1] >= 0) && (v[2] >= 0)) {
			s += '<br>' + timeString(v[1]) + ' to ' + timeString(v[2]);
			if (v[2] <= v[1]) s += ' <small>(the following day)</small>';
		}
		if (v[0] != '1') s += '<br><i><b>Disabled</b></i>';
		this.insertData(-1, [i, v[8], s]);
	}
	og.sort(0);
}
og.dataToView = function(data) {
	return [escapeHTML(data[1]), data[2]];
}
og.onClick = function(cell) {
	E('_rruleN').value = PR(cell).getRowData()[0];
	form.submit('_fom');
}
og.addEntry = function() {
	for (var i = 0; i < 140; ++i) {
		if ((rrules[i] == null) || (rrules[i] == '')) {
			E('_rruleN').value = i;
			form.submit('_fom');
			return;
		}
	}
}

function init()
{
	og.populate();
}
</script>
</head>
<body onload='init()'>
<form name='_fom' id='_fom' method='post' action='tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_redirect' value='restrict-edit.asp'>
<input type='hidden' name='_commit' value='0'>
<input type='hidden' name='rruleN' id='_rruleN' value=''>

<div class='section-title'>Access Restriction Overview</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='res-over-grid'></table>
</div>

<br>
<script type='text/javascript'>show_notice1('<% notice("iptables"); %>');</script>
<br>
<script type='text/javascript'>show_notice1('<% notice("ip6tables"); %>');</script>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>&nbsp;</td></tr>
</table>
</form>
<script text='text/javascript'>og.setup();</script>
</body>
</html>

