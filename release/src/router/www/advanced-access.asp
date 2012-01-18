<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2007 Jonathan Zarate
	http://www.polarcloud.com/tomato/
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
	LAN Access admin module by Augusto Bott
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Advanced: LAN Access</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>
<style type='text/css'>
#la-grid .co1 {
  text-align: center;
  width: 30px;
}
#la-grid .co3,
#la-grid .co5 {
  text-align: center;
  width: 120px;
}
#la-grid .co6 {
  text-align: center;
  width: 250px;
}
#la-grid .co2,
#la-grid .co4 {
  text-align: center;
}

#la-grid .centered {
  text-align: center;
}
</style>
<script type='text/javascript' src='wireless.jsx?_http_id=<% nv(http_id); %>'></script>
<script type='text/javascript'>
<% nvram ("lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,lan_access");%> 

var MAX_BRIDGE_ID = 3;

var la = new TomatoGrid();
la.setup = function() {
	this.init('la-grid', 'sort', 50, [
	{ type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
	{ type: 'select', options: [[0, 'LAN (br0)'],[1, 'LAN1 (br1)'],[2, 'LAN2 (br2)'],[3, 'LAN3 (br3)']], prefix: '<div class="centered">', suffix: '</div>' },
	{ type: 'text', maxlen: 32 },
	{ type: 'select', options: [[0, 'LAN (br0)'],[1, 'LAN1 (br1)'],[2, 'LAN2 (br2)'],[3, 'LAN3 (br3)']], prefix: '<div class="centered">', suffix: '</div>' },
	{ type: 'text', maxlen: 32 },
	{ type: 'text', maxlen: 32 }]);
	this.headerSet(['On', 'Src', 'Src Address', 'Dst', 'Dst Address', 'Description']);

	var r = nvram.lan_access.split('>');
	for (var i = 0; i < r.length; ++i) {
		if(r[i].length) {
			var l = r[i].split('<');
			l[0] *= 1;
			l[1] *= 1;
			l[3] *= 1;
			la.insertData(-1, [ l[0], l[1], l[2], l[3], l[4], l[5] ] );
		}
	}

	la.recolor();
	la.showNewEditor();
	la.resetNewEditor();
}

la.sortCompare = function(a, b) {
	var col = this.sortColumn;
	var da = a.getRowData();
	var db = b.getRowData();
	var r;

	switch (col) {
	case 2:	// src
	case 4:	// dst
		r = cmpIP(da[col], db[col]);
		break;
	case 0:	// on
	case 1: // src br
	case 3:	// dst br
		r = cmpInt(da[col], db[col]);
		break;
	default:
		r = cmpText(da[col], db[col]);
		break;
	}

	return this.sortAscending ? r : -r;
}

la.resetNewEditor = function() {
	var f = fields.getAll(this.newEditor);
	f[0].checked=true;
	f[2].value='';
	f[4].value='';
	f[5].value='';
	var total=0;
	for (var i=0; i<= MAX_BRIDGE_ID; i++) {
		var j = (i == 0) ? '' : i.toString();
		if (nvram['lan' + j + '_ifname'].length < 1) {
			f[1].options[i].disabled=true;
			f[3].options[i].disabled=true;
		} else {
			++total;
		}
	}
	if((f[1].selectedIndex == f[3].selectedIndex) && (total > 1)) {
		while (f[1].selectedIndex == f[3].selectedIndex) {
			f[3].selectedIndex = (f[3].selectedIndex%(MAX_BRIDGE_ID+1)+1);
		}
	}
	ferror.clearAll(fields.getAll(this.newEditor));
}

la.verifyFields = function(row, quiet) {
	var f = fields.getAll(row);

	for (var i=0; i<= MAX_BRIDGE_ID; i++) {
		var j = (i == 0) ? '' : i.toString();
		if (nvram['lan' + j + '_ifname'].length < 1) {
			f[1].options[i].disabled=true;
			f[3].options[i].disabled=true;
		}
	}

	if(f[1].selectedIndex == f[3].selectedIndex) {
		var m = 'Source and Destination interfaces must be different';
		ferror.set(f[1], m, quiet);
		ferror.set(f[3], m, quiet);
		return 0;
	}
	ferror.clear(f[1]);
	ferror.clear(f[3]);

	f[2].value = f[2].value.trim();
	f[4].value = f[4].value.trim();

	if ((f[2].value.length) && (!v_iptaddr(f[2], quiet))) return 0;
	if ((f[4].value.length) && (!v_iptaddr(f[4], quiet))) return 0;

	ferror.clear(f[2]);
	ferror.clear(f[4]);

	f[5].value = f[5].value.replace(/>/g, '_');
	if (!v_nodelim(f[5], quiet, 'Description')) return 0;

	return 1;
}

la.dataToView = function(data) {
	return [(data[0] != 0) ? 'On' : '',
			['LAN', 'LAN1', 'LAN2', 'LAN3'][data[1]],
			data[2],
			['LAN', 'LAN1', 'LAN2', 'LAN3'][data[3]],
			data[4],
			data[5] ];
}

la.dataToFieldValues = function (data) {
	return [(data[0] != 0) ? 'checked' : '',
			data[1],
			data[2],
			data[3],
			data[4],
			data[5] ];
}

la.fieldValuesToData = function(row) {
	var f = fields.getAll(row);
	return [f[0].checked ? 1 : 0,
			f[1].selectedIndex,
			f[2].value,
			f[3].selectedIndex,
			f[4].value,
			f[5].value ];
}

function save()
{
	if (la.isEditing()) return;
	la.resetNewEditor();

	var fom = E('_fom');
	var ladata = la.getAllData();

	var s = '';
	for (var i = 0; i < ladata.length; ++i) {
		s += ladata[i].join('<') + '>';
	}
	fom.lan_access.value = s;

	form.submit(fom, 0);
}

function init() {
	la.setup();
	var c;
	if (((c = cookie.get('advanced_access_notes_vis')) != null) && (c == '1')) toggleVisibility("notes");
}

function toggleVisibility(whichone) {
	if (E('sesdiv_' + whichone).style.display == '') {
		E('sesdiv_' + whichone).style.display = 'none';
		E('sesdiv_' + whichone + '_showhide').innerHTML = '(Click here to show)';
		cookie.set('advanced_access_' + whichone + '_vis', 0);
	} else {
		E('sesdiv_' + whichone).style.display='';
		E('sesdiv_' + whichone + '_showhide').innerHTML = '(Click here to hide)';
		cookie.set('advanced_access_' + whichone + '_vis', 1);
	}
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
<input type='hidden' name='_nextpage' value='advanced-access.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_service' value='firewall-restart'>
<input type='hidden' name='lan_access'>

<div class='section-title'>LAN Access</div>
<div class='section'>
  <table class='tomato-grid' cellspacing=1 id='la-grid'></table>
</div>
</div>

<div class='section-title'>Notes <small><i><a href='javascript:toggleVisibility("notes");'><span id='sesdiv_notes_showhide'>(Click here to show)</span></a></i></small></div>
<div class='section' id='sesdiv_notes' style='display:none'>
<ul>
<li><b>Src</b> - Source LAN bridge.</li>
<li><b>Src Address</b> <i>(optional)</i> - Source address allowed. Ex: "1.2.3.4", "1.2.3.4 - 2.3.4.5", "1.2.3.0/24".</li>
<li><b>Dst</b> - Destination LAN bridge.</li>
<li><b>Dst Address</b> <i>(optional)</i> - Destination address inside the LAN.</li>
</ul>
</div>

</td></tr>
<tr><td id='footer' colspan=2>
 <span id='footer-msg'></span>
 <input type='button' value='Save' id='save-button' onclick='save()'>
 <input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
</body>
</html>
