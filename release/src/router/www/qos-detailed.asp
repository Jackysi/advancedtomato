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
<title>[<% ident(); %>] QoS: View Details</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#grid .co7 {
	width: 20px;
}
#grid .co8 {
	text-align: right;
}
#grid .co9 {
	text-align: right;
}
</style>

<script type='text/javascript' src='debug.js'></script>
<script type='text/javascript' src='protocols.js'></script>

<script type='text/javascript'>

var Unclassified = ['Unclassified'];
var classNames = nvram.qos_classnames.split(' ');		// Toastman - configurable class names
var abc = Unclassified.concat(classNames);

//	<% nvram('qos_classnames,lan_ipaddr,lan1_ipaddr,lan2_ipaddr,lan3_ipaddr,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,wan_proto,lan_netmask,lan1_netmask,lan2_netmask,lan3_netmask'); %>

var colors = ['F08080','E6E6FA','0066CC','8FBC8F','FAFAD2','ADD8E6','9ACD32','E0FFFF','90EE90','FF9933','FFF0F5'];
var filterip = [];
var filteripe = [];

if ((viewClass = '<% cgi_get("class"); %>') == '') {
	viewClass = -1;
}
else if ((isNaN(viewClass *= 1)) || (viewClass < 0) || (viewClass > 10)) {
	viewClass = 0;
}

var queue = [];
var xob = null;
var cache = [];
var lock = 0;

function resolve()
{
	if ((queue.length == 0) || (xob)) return;

	xob = new XmlHttp();
	xob.onCompleted = function(text, xml) {
		eval(text);
		for (var i = 0; i < resolve_data.length; ++i) {
			var r = resolve_data[i];
			if (r[1] == '') r[1] = r[0];
			cache[r[0]] = r[1];
			if (lock == 0) grid.setName(r[0], r[1]);
		}
		if (queue.length == 0) {
			if ((lock == 0) && (resolveCB) && (grid.sortColumn == 4)) grid.resort();
		}
		else setTimeout(resolve, 500);
		xob = null;
	}
	xob.onError = function(ex) {
		xob = null;
	}

	xob.post('/resolve.cgi', 'ip=' + queue.splice(0, 20).join(','));
}

var resolveCB = 0;

function resolveChanged()
{
	var b;

	b = E('resolve').checked ? 1 : 0;
	if (b != resolveCB) {
		resolveCB = b;
		cookie.set('qos_resolve', b);
		if (b) grid.resolveAll();
	}
}

var grid = new TomatoGrid();

grid.dataToView = function(data) {
	var s, v = [];
	for (var col = 0; col < data.length; ++col) {
		switch (col) {
		case 5:		// Class
			s = abc[data[col]] || ('' + data[col]);
			break;
		case 6:		// Rule #
			s = (data[col] * 1 > 0) ? ('' + data[col]) : '';
			break;
		case 7:		// Bytes out
		case 8:		// Bytes in
			s = scaleSize(data[col] * 1);
			break;
		default:
			s = '' + data[col];
			break;
		}
		v.push(s);
	}
	return v;
}

grid.sortCompare = function(a, b) {
	var obj = TGO(a);
	var col = obj.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
	case 0:		// Proto
	case 2:		// S port
	case 4:		// D port
	case 6:		// Rule #
	case 7:		// Bytes out
	case 8:		// Bytes in
		r = cmpInt(da[col], db[col]);
		break;
	case 5:		// Class
		r = cmpInt(da[col] ? da[col] : 10000, db[col] ? db[col] : 10000);
		break;
	case 1:
	case 3:
		var a = fixIP(da[col]);
		var b = fixIP(db[col]);
		if ((a != null) && (b != null)) {
			r = aton(a) - aton(b);
			break;
		}
	default:
		r = cmpText(da[col], db[col]);
		break;
	}
	return obj.sortAscending ? r : -r;
}

grid.onClick = function(cell) {
	var row = PR(cell);
	var ip = row.getRowData()[3];
	if (this.lastClicked != row) {
		this.lastClicked = row;
		if (ip.indexOf('<') == -1) {
			queue.push(ip);
			row.style.cursor = 'wait';
			resolve();
		}
	}
	else {
		this.resolveAll();
	}
}

grid.resolveAll = function()
{
	var i, ip, row, q, cols, j;

	q = [];
	cols = [1, 3];
	for (i = 1; i < this.tb.rows.length; ++i) {
		row = this.tb.rows[i];
		for (j = cols.length-1; j >= 0; j--) {
			ip = row.getRowData()[cols[j]];
			if (ip.indexOf('<') == -1) {
				if (!q[ip]) {
					q[ip] = 1;
					queue.push(ip);
				}
				row.style.cursor = 'wait';
			}
		}
	}
	q = null;
	resolve();
}

grid.setName = function(ip, name) {
	var i, row, data, cols, j;

	cols = [1, 3];
	for (i = this.tb.rows.length - 1; i > 0; --i) {
		row = this.tb.rows[i];
		data = row.getRowData();
		for (j = cols.length-1; j >= 0; j--) {
			if (data[cols[j]] == ip) {
				data[cols[j]] = name + ((ip.indexOf(':') != -1) ? '<br>' : ' ') + '<small>(' + ip + ')</small>';
				row.setRowData(data);
				row.cells[cols[j]].innerHTML = data[cols[j]];
				row.style.cursor = 'default';
			}
		}
	}
}

grid.setup = function() {
	this.init('grid', 'sort');
	this.headerSet(['Proto', 'Source', 'S Port', 'Destination', 'D Port', 'Class', 'Rule', 'Bytes Out', 'Bytes In']);
}

var ref = new TomatoRefresh('/update.cgi', '', 0, 'qos_detailed');

ref.refresh = function(text)
{
	var i, b, d, cols, j;

	++lock;

	try {
		ctdump = [];
		eval(text);
	}
	catch (ex) {
		ctdump = [];
	}

	grid.lastClicked = null;
	grid.removeAllData();

	var c = [];
	var q = [];
	var cursor;
	var ip;

	var fskip;

	cols = [2, 3];

	for (i = 0; i < ctdump.length; ++i) {
		b = ctdump[i];
		fskip=0;
		if (E('_f_excludegw').checked) {
			if ((b[2] == nvram.lan_ipaddr) || (b[3] == nvram.lan_ipaddr) ||
				(b[2] == nvram.lan1_ipaddr) || (b[3] == nvram.lan1_ipaddr) ||
				(b[2] == nvram.lan2_ipaddr) || (b[3] == nvram.lan2_ipaddr) ||
				(b[2] == nvram.lan3_ipaddr) || (b[3] == nvram.lan3_ipaddr) ) {
				fskip=1;
			}
		}
		if (fskip == 1) continue;

		if (filteripe.length>0) {
			fskip = 0;
			for (x = 0; x < filteripe.length; ++x) {
				if ((b[2] == filteripe[x]) || (b[3] == filteripe[x])) {
					fskip=1;
					break;
				}
			}
		}
		if (fskip == 1) continue;

		if (filterip.length>0) {
			fskip = 1;
			for (x = 0; x < filterip.length; ++x) {
				if ((b[2] == filterip[x]) || (b[3] == filterip[x])) {
					fskip=0;
					break;
				}
			}
		}
		if (fskip == 1) continue;

		for (j = cols.length-1; j >= 0; j--) {
			ip = b[cols[j]];
			if (cache[ip] != null) {
				c[ip] = cache[ip];
				b[cols[j]] = cache[ip] + ((ip.indexOf(':') != -1) ? '<br>' : ' ') + '<small>(' + ip + ')</small>';
				cursor = 'default';
			}
			else {
				if (resolveCB) {
					if (!q[ip]) {
						q[ip] = 1;
						queue.push(ip);
					}
					cursor = 'wait';
				}
				else cursor = null;
			}
		}

		d = [protocols[b[0]] || b[0], b[2], b[4], b[3], b[5], b[8], b[9], b[6], b[7]];
		var row = grid.insertData(-1, d);
		if (cursor) row.style.cursor = cursor;
	}
	cache = c;
	c = null;
	q = null;

	grid.resort();
	setTimeout(function() { E('loading').style.visibility = 'hidden'; }, 100);

	--lock;

	if (resolveCB) resolve();
}

function init()
{
	var c;

	if (((c = cookie.get('qos_resolve')) != null) && (c == '1')) {
		E('resolve').checked = resolveCB = 1;
	}

	if (viewClass != -1) E('stitle').innerHTML = 'View Details: ' + abc[viewClass];
	grid.setup();
	ref.postData = 'exec=ctdump&arg0=' + viewClass;
	ref.initPage(250);

	if (!ref.running) ref.once = 1;
	ref.start();
}

function dofilter() {
	if (E('_f_filter_ip').value.length>6) {
		filterip = E('_f_filter_ip').value.split(',');
	} else {
		filterip = [];
	}

	if (E('_f_filter_ipe').value.length>6) {
		filteripe = E('_f_filter_ipe').value.split(',');
	} else {
		filteripe = [];
	}

	if (!ref.running)
		ref.start();
}

function toggleFiltersVisibility(){
	if(E('sesdivfilters').style.display=='')
		E('sesdivfilters').style.display='none';
	else
		E('sesdivfilters').style.display='';
}

function verifyFields(focused, quiet)
{
	dofilter();
	return 1;
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

<div class='section-title'>Filters <small><i><a href='javascript:toggleFiltersVisibility();'>(Toggle Visibility)</a></i></small></div>
<div class='section' id='sesdivfilters' style='display:none'>
<script type='text/javascript'>
var c;
c = [];
c.push({ title: 'Only these IPs', name: 'f_filter_ip', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)</small>' });
c.push({ title: 'Exclude these IPs', name: 'f_filter_ipe', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)</small>' });
c.push({ title: 'Exclude gateway traffic', name: 'f_excludegw', type: 'checkbox', value: 1 });
createFieldTable('',c);
</script>
</div>
</div>

<div class='section-title' id='stitle' onclick='document.location="qos-graphs.asp"' style='cursor:pointer'>Details</div>
<div class='section'>
<table id='grid' class='tomato-grid' style="float:left" cellspacing=1></table>
<input type='checkbox' id='resolve' onclick='resolveChanged()' onchange='resolveChanged()'> Automatically Resolve Addresses
<div id='loading'><br><b>Loading...</b></div>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<script type='text/javascript'>genStdRefresh(1,3,'ref.toggle()');</script>
</td></tr>
</table>
</form>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
