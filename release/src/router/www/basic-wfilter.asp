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
<title>[<% ident(); %>] Basic: Wireless Filter</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<% css(); %>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
#sm-grid {
	width: 80%;
}
#sm-grid .co1 {
	width: 30%;
}
#sm-grid .co2 {
	width: 70%;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript' src='wireless.jsx?_http_id=<% nv(http_id); %>'></script>
<script type='text/javascript'>

//	<% nvram("wl_macmode,wl_maclist,macnames"); %>

var smg = new TomatoGrid();

smg.verifyFields = function(row, quiet) {
	var f;
	f = fields.getAll(row);

	return v_mac(f[0], quiet) && v_nodelim(f[1], quiet, 'Description', 1);
}

smg.resetNewEditor = function() {
	var f, c, n;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	if ((c = cookie.get('addmac')) != null) {
		cookie.set('addmac', '', 0);
		c = c.split(',');
		if (c.length == 2) {
			f[0].value = c[0];
			f[1].value = c[1];
			return;
		}
	}

	f[0].value = '00:00:00:00:00:00';
	f[1].value = '';
}

smg.setup = function() {
	var i, i, m, s, t, n;
	var macs, names;

	this.init('sm-grid', 'sort', 280, [
		{ type: 'text', maxlen: 17 },
		{ type: 'text', maxlen: 48 }
	]);
	this.headerSet(['MAC Address', 'Description']);
	macs = nvram.wl_maclist.split(/\s+/);
	names = nvram.macnames.split('>');
	for (i = 0; i < macs.length; ++i) {
		m = fixMAC(macs[i]);
		if ((m) && (!isMAC0(m))) {
			s = m.replace(/:/g, '');
			t = '';
			for (j = 0; j < names.length; ++j) {
				n = names[j].split('<');
				if ((n.length == 2) && (n[0] == s)) {
					t = n[1];
					break;
				}
			}
			this.insertData(-1, [m, t]);
		}
	}
	this.sort(0);
	this.showNewEditor();
	this.resetNewEditor();
}

function save()
{
	var fom;
	var d, i, macs, names, ma, na;
	var u;

	if (smg.isEditing()) return;

	fom = E('_fom');

	macs = [];
	names = [];
	d = smg.getAllData();
	for (i = 0; i < d.length; ++i) {
		ma = d[i][0];
		na = d[i][1].replace(/[<>|]/g, '');

		macs.push(ma);
		if (na.length) {
			names.push(ma.replace(/:/g, '') + '<' + na);
		}
	}
	fom.wl_maclist.value = macs.join(' ');
	fom.wl_macmode.value = E('_f_disable').checked ? 'disabled' : (E('_f_deny').checked ? 'deny' : 'allow');
	fom.macnames.value = names.join('>');

	for (i = 0; i < wl_ifaces.length; ++i) {
		u = wl_fface(i);
		E('_wl'+u+'_macmode').value = fom.wl_macmode.value;
		E('_wl'+u+'_maclist').value = fom.wl_maclist.value;
	}

	form.submit(fom, 1);
}

function earlyInit()
{
	smg.setup();
	if (nvram.wl_macmode == 'allow') E('_f_allow').checked = 1;
		else if (nvram.wl_macmode == 'deny') E('_f_deny').checked = 1;
		else E('_f_disable').checked = 1;
}

function init()
{
	smg.recolor();
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

<input type='hidden' name='_nextpage' value='basic-wfilter.asp'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_service' value='*'>

<input type='hidden' name='wl_macmode'>
<input type='hidden' name='wl_maclist'>
<input type='hidden' name='macnames'>

<script type='text/javascript'>
for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
	var u = wl_fface(uidx);
	W('<input type=\'hidden\' id=\'_wl'+u+'_macmode\' name=\'wl'+u+'_macmode\'>');
	W('<input type=\'hidden\' id=\'_wl'+u+'_maclist\' name=\'wl'+u+'_maclist\'>');
}
</script>

<div class='section-title'>Wireless Client Filter</div>
<div class='section'>
	<input type='radio' name='f_type' id='_f_disable' value='disabled'> <label for='_f_disable'>Disable filter</label><br>
	<input type='radio' name='f_type' id='_f_allow' value='allow'> <label for='_f_allow'>Permit only the following clients</label><br>
	<input type='radio' name='f_type' id='_f_deny' value='deny'> <label for='_f_deny'>Block the following clients</label><br>
	<br>
	<table id='sm-grid' class='tomato-grid'></table>
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
