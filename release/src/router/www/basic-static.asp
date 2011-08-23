<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Enhancements by Teaman
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] <% translate("Basic"); %>: <% translate("Static DHCP/ARP & Bandwidth Monitoring of LAN Clients"); %></title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#bs-grid .co1 {
	width: 120px;
	text-align: center;
}
#bs-grid .co2 {
	width: 80px;
	text-align: center;
}
#bs-grid .co3 {
	width: 120px;
}
#bs-grid .co4 {
	width: 80px;
	text-align: center;
}
#bs-grid .centered {
	text-align: center;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("lan_ipaddr,lan_netmask,dhcpd_static,dhcpd_startip,bwm_client,dhcpd_static_only,arpbind_static"); %>

if (nvram.lan_ipaddr.match(/^(\d+\.\d+\.\d+)\.(\d+)$/)) ipp = RegExp.$1 + '.';
	else ipp = '?.?.?.';

autonum = aton(nvram.lan_ipaddr) & aton(nvram.lan_netmask);

var sg = new TomatoGrid();

sg.exist = function(f, v) {
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

sg.existMAC = function(mac) {
	if (isMAC0(mac)) return false;
	return this.exist(0, mac) || this.exist(1, mac);
}

sg.existName = function(name) {
	return this.exist(4, name);
}

sg.inStatic = function(n) {
	return this.exist(2, n);
}

sg.dataToView = function(data) {
	var v = [];
	var s = (data[0] == '00:00:00:00:00:00') ? '' : data[0];
	if (!isMAC0(data[1])) s += '<br>' + data[1];
	v.push((s == '') ? '<center><small><i>(unset)</i></small></center>' : s);
	v.push((data[2].toString() != '0') ? '<% translate("Enabled"); %>' : '');
	v.push(escapeHTML('' + data[3]));
	v.push((data[4].toString() != '0') ? '<% translate("Enabled"); %>' : '');
	v.push(escapeHTML('' + data[5]));
	return v;
}

sg.dataToFieldValues = function (data) {
	return ([data[0],
			data[1],
			(data[2].toString() != '0') ? 'checked' : '',
			data[3],
			(data[4].toString() != '0') ? 'checked' : '',
			data[5]]);
}

sg.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	return ([f[0].value,
			f[1].value,
			f[2].checked ? '1' : '0',
			f[3].value,
			f[4].checked ? '1' : '0',
			f[5].value]);
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
		r = cmpInt(da[2], db[2]);
		break;
	case 2:
		r = cmpIP(da[3], db[3]);
		break;
	case 3:
		r = cmpInt(da[4], db[4]);
		break;
	}
	if (r == 0) r = cmpText(da[5], db[5]);
	return this.sortAscending ? r : -r;
}


sg.verifyFields = function(row, quiet) {
	var f, s, i;

	f = fields.getAll(row);

	if (!v_macz(f[0], quiet)) return 0;
	if (!v_macz(f[1], quiet)) return 0;
	if (isMAC0(f[0].value)) {
		f[0].value = f[1].value;
		f[1].value = '00:00:00:00:00:00';
	}
	else if (f[0].value == f[1].value) {
		f[1].value = '00:00:00:00:00:00';
	}
	else if ((!isMAC0(f[1].value)) && (f[0].value > f[1].value)) {
		s = f[1].value;
		f[1].value = f[0].value;
		f[0].value = s;
	}

	f[1].disabled = f[2].checked;

	for (i = 0; i < 2; ++i) {
		if (this.existMAC(f[i].value)) {
			ferror.set(f[i], '<% translate("Duplicate MAC address"); %>', quiet);
			return 0;
		}
	}	

	if (f[2].value.indexOf('.') == -1) {
		s = parseInt(f[3].value, 10)
		if (isNaN(s) || (s <= 0) || (s >= 255)) {
			ferror.set(f[3], '<% translate("Invalid IP address"); %>', quiet);
			return 0;
		}
		f[2].value = ipp + s;
	}

	if ((!isMAC0(f[0].value)) && (this.inStatic(f[3].value))) {
		ferror.set(f[3], '<% translate("Duplicate IP address"); %>', quiet);
		return 0;
	}

	s = f[5].value.trim().replace(/\s+/g, ' ');
	if (s.length > 0) {
		if (s.search(/^[.a-zA-Z0-9_\- ]+$/) == -1) {
			ferror.set(f[5], '<% translate("Invalid hostname. Only characters"); %> "A-Z 0-9 . - _" <% translate("are allowed"); %>.', quiet);
			return 0;
		}
		if (this.existName(s)) {
			ferror.set(f[5], '<% translate("Duplicate hostname"); %>.', quiet);
			return 0;
		}
		f[4].value = s;
	}

	if (isMAC0(f[0].value)) {
		if (s == '') {
			s = '<% translate("Both MAC address and name fields must not be empty"); %>.';
			ferror.set(f[0], s, 1);
			ferror.set(f[5], s, quiet);
			return 0;
		} else {
			ferror.clear(f[0]);
			ferror.clear(f[5]);
		}
	}

	if (((f[0].value == '00:00:00:00:00:00') || (f[1].value == '00:00:00:00:00:00')) && (f[0].value == f[1].value)) {
		f[2].disabled=1;
		f[2].checked=0;
	} else {
		f[2].disabled=0;
	}

	return 1;
}

sg.resetNewEditor = function() {
	var f, c, n;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addstatic')) != null) {
		cookie.set('addstatic', '', 0);
		c = c.split(',');
		if (c.length == 3) {
			f[0].value = c[0];
			f[1].value = '00:00:00:00:00:00';
			f[3].value = c[1];
			f[5].value = c[2];
			return;
		}
	}

	f[0].value = '00:00:00:00:00:00';
	f[1].value = '00:00:00:00:00:00';
	f[2].disabled = 1;
	f[5].value = '';

	n = 10;
	do {
		if (--n < 0) {
			f[2].value = '';
			return;
		}
		autonum++;
	} while (((c = fixIP(ntoa(autonum), 1)) == null) || (c == nvram.lan_ipaddr) || (this.inStatic(c)));

	f[2].value = c;
}

sg.setup = function() {
	this.init('bs-grid', 'sort', 250, [
		{ multi: [ { type: 'text', maxlen: 17 }, { type: 'text', maxlen: 17 } ] },
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
		{ type: 'text', maxlen: 15 },
		{ type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
		{ type: 'text', maxlen: 50 } ] );

	this.headerSet(['<% translate("MAC Address"); %>', '<% translate("Bound to"); %>', '<% translate("IP Address"); %>', '<% translate("BW Mon"); %>', '<% translate("Hostname"); %>' ]);

	var s = nvram.dhcpd_static.split('>');
	var bwr = nvram.bwm_client.split('>');
	var asr = nvram.arpbind_static.split('>');
	for (var i = 0; i < s.length; ++i) {
		var bwe = '0';
		var ase = '0';
		var t = s[i].split('<');
		if (t.length == 3) {
			var d = t[0].split(',');
			var ip = (t[1].indexOf('.') == -1) ? (ipp + t[1]) : t[1];

			for (var j = 0; j < bwr.length; ++j) {
				var bwl = bwr[j].split('<');
				if ((bwl.length == 2) && (bwl[0] == ip))
					bwe = '1';
			}

			for (var k = 0; k < asr.length; ++k) {
				var asl = asr[k].split('<');
				if ((asl.length == 2) && (asl[1] == d))
					ase = '1';
			}

			this.insertData(-1, [ d[0], (d.length >= 2) ? d[1] : '00:00:00:00:00:00', ase,
				ip, bwe, t[2] ]);
		}
	}

	this.sort(4);
	this.showNewEditor();
	this.resetNewEditor();
}

function save() {
	if (sg.isEditing()) return;

	var data = sg.getAllData();
	var sdhcp = '';
	var sarp = '';
	var bwm = '';
	var i;

	for (i = 0; i < data.length; ++i) {
		var d = data[i];
		sdhcp += d[0];
		if (!isMAC0(d[1])) sdhcp += ',' + d[1];
		sdhcp += '<' + d[3] + '<' + d[5] + '>';

		if (d[2] == '1') sarp += d[3] + '<' + d[0] + '>';
		if (d[4] == '1') bwm += d[3] + '<' + d[5].split(' ').splice(0,1) + '>';
	}

	var fom = E('_fom');
	fom.bwm_client.value = bwm;
	fom.dhcpd_static.value = sdhcp;
	fom.dhcpd_static_only.value = E('_f_dhcpd_static_only').checked ? '1' : '0';
	fom.arpbind_static.value = sarp;
	form.submit(fom, 1);
}

function init() {
	var c;
	if (((c = cookie.get('basic_static_notes_vis')) != null) && (c == '1')) {
		toggleVisibility("notes");
	}
	if (((c = cookie.get('basic_static_options_vis')) != null) && (c == '1')) {
		toggleVisibility("options");
	}
	sg.recolor();
}

function toggleVisibility(whichone) {
	if(E('sesdiv' + whichone).style.display=='') {
		E('sesdiv' + whichone).style.display='none';
		E('sesdiv' + whichone + 'showhide').innerHTML='(<% translate("Click here to show"); %>)';
		cookie.set('basic_static_' + whichone + '_vis', 0);
	} else {
		E('sesdiv' + whichone).style.display='';
		E('sesdiv' + whichone + 'showhide').innerHTML='(<% translate("Click here to hide"); %>)';
		cookie.set('basic_static_' + whichone + '_vis', 1);
	}
}

function verifyFields(focused, quiet) {
	return 1;
}

</script>
</head>
<body onload='init()'>
<form id='_fom' method='post' action='tomato.cgi'>
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'><% translate("Version"); %> <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<input type='hidden' name='_nextpage' value='basic-static.asp'>
<input type='hidden' name='_service' value='dhcpd-restart,bwclimon-restart,arpbind-restart'>

<input type='hidden' name='bwm_client'>
<input type='hidden' name='dhcpd_static'>
<input type='hidden' name='dhcpd_static_only'>
<input type='hidden' name='arpbind_static'>

<div class='section-title'><% translate("Static DHCP/ARP & Bandwidth Monitoring of LAN Clients"); %></div>
<div class='section'>
	<table class='tomato-grid' id='bs-grid'></table>
</div>

<!-- / / / -->

<div class='section-title'><% translate("Options"); %> <small><i><a href='javascript:toggleVisibility("options");'><span id='sesdivoptionsshowhide'>(<% translate("Click here to show"); %>)</span></a></i></small></div>
<div class='section' id='sesdivoptions' style='display:none'>
<script type='text/javascript'>
createFieldTable('', [
{ title: '<% translate("Ignore DHCP requests from unknown devices"); %>', name: 'f_dhcpd_static_only', type: 'checkbox', value: nvram.dhcpd_static_only == '1' }
]);
</script>
</div>

<!-- / / / -->

<div class='section-title'><% translate("Notes"); %> <small><i><a href='javascript:toggleVisibility("notes");'><span id='sesdivnotesshowhide'>(<% translate("Click here to show"); %>)</span></a></i></small></div>
<div class='section' id='sesdivnotes' style='display:none'>
<ul>
<li><b><% translate("MAC Address"); %></b> - <% translate("Unique identifier associated to a network interface on this particular device"); %>.</li>
<li><b><% translate("Bound to"); %></b> - <% translate("Enforce static ARP binding of this particular IP/MAC address pair"); %>.</li>
<li><b><% translate("IP Address"); %></b> - <% translate("Network address assigned to this device on the local network"); %>.</li>
<li><b><% translate("BW Mon"); %></b> - <% translate("Monitor volume of network traffic from/to this IP address that goes through the router"); %>.</li>
<li><b><% translate("Hostname"); %></b> - <% translate("Human-readable nickname/label assigned to this device on the network"); %>.</li>
</ul>
<ul>
<li><b><% translate("Ignore DHCP requests from unknown devices"); %></b> - <% translate("Unlisted MAC addresses won't be able to obtain an IP address through DHCP"); %>.</li>
</ul>
<small>
<ul>
<li><b><% translate("Other relevant notes/hints"); %>:</b>
<ul>
<li><% translate("To specify multiple hostnames for a device, separate them with spaces"); %>.</li>
<li><% translate("To enable/enforce static ARP binding for a particular device, it must have only one MAC associated with that particular IP address (i.e. you can't have two MAC addresses linked to the same hostname/device in the table above)"); %>.</li>
<li><% translate("When ARP binding is enabled for a particular MAC/IP address pair, that device will always be shown as 'active' in the 'Wake On LAN' table"); %>.</li>
</ul>
</ul>
</small>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='<% translate("Save"); %>' id='save-button' onclick='save()'>
	<input type='button' value='<% translate("Cancel"); %>' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>sg.setup();</script>
</body>
</html>
