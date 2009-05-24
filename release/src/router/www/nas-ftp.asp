<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	FTP Server - !!TB

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] NAS: FTP Server</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style tyle='text/css'>
#aft-grid {
	width: 99%;
}
#aft-grid .co1,
#aft-grid .co2 {
	width: 40%;
}
#aft-grid .co3 {
	width: 20%;
}
</style>

<style type='text/css'>
textarea {
	width: 98%;
	height: 5em;
}
</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("ftp_enable,ftp_super,ftp_anonymous,ftp_dirlist,ftp_port,ftp_max,ftp_ipmax,ftp_staytimeout,ftp_rate,ftp_anonrate,ftp_anonroot,ftp_pubroot,ftp_pvtroot,ftp_custom,ftp_users,log_ftp"); %>


var aftg = new TomatoGrid();

aftg.exist = function(f, v)
{
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

aftg.existName = function(name)
{
	return this.exist(0, name);
}

aftg.sortCompare = function(a, b) {
	var da = a.getRowData();
	var db = b.getRowData();
	var r = cmpText(da[this.sortColumn], db[this.sortColumn]);
	return this.sortAscending ? r : -r;
}

aftg.verifyFields = function(row, quiet)
{
	var f, s;
	f = fields.getAll(row);

	ferror.clear(f[0]);
	ferror.clear(f[1]);

	s = f[0].value.trim().replace(/\s+/g, ' ');
	if (s.length > 0) {
		if (s.search(/^[a-zA-Z0-9_\-]+$/) == -1) {
			ferror.set(f[0], 'Invalid user name. Only characters "A-Z 0-9 - _" are allowed.', quiet);
			return 0;
		}
		if (this.existName(s)) {
			ferror.set(f[0], 'Duplicate user name.', quiet);
			return 0;
		}
		f[0].value = s;
	}
	else {
		ferror.set(f[0], 'Empty user name is not allowed.', quiet);
		return 0;
	}

	f[1].value = f[1].value.replace(/>/g, '_');
	if (f[1].value.length <= 0) {
		ferror.set(f[1], 'Password must not be empty.', quiet);
		return 0;
	}

	return 1;
}

aftg.resetNewEditor = function() {
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);

	f[0].value = '';
	f[1].value = '';
	f[2].selectedIndex = 0;
}

aftg.setup = function()
{
	this.init('aft-grid', 'sort', 50, [
		{ type: 'text', maxlen: 16 },
		{ type: 'text', maxlen: 16 }, 
		{ type: 'select', options: [['Read/Write', 'Read/Write'],['Read Only', 'Read Only'],['View Only', 'View Only'],['Private', 'Private']] }
	]);
	this.headerSet(['User Name', 'Password', 'Access']);

	var s = nvram.ftp_users.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 3) {
			this.insertData(-1, t);
		}
	}

	this.showNewEditor();
	this.resetNewEditor();
}

function verifyFields(focused, quiet)
{
	var a, b;

	a = E('_ftp_enable').value;	
	b = E('_ftp_port');
	elem.display(PR(b), (a != 0));

	if ((a != 0) && (!v_port(b, quiet))) {
		return 0;
	}

	E('_ftp_anonymous').disabled = (a == 0);
	E('_f_ftp_super').disabled = (a == 0);
	E('_f_log_ftp').disabled = (a == 0);
	E('_ftp_pubroot').disabled = (a == 0);
	E('_ftp_pvtroot').disabled = (a == 0);
	E('_ftp_anonroot').disabled = (a == 0);
	E('_ftp_dirlist').disabled = (a == 0);
	E('_ftp_max').disabled = (a == 0);
	E('_ftp_ipmax').disabled = (a == 0);
	E('_ftp_rate').disabled = (a == 0);
	E('_ftp_anonrate').disabled = (a == 0);
	E('_ftp_staytimeout').disabled = (a == 0);
	E('_ftp_custom').disabled = (a == 0);

	if (a != 0) {
		if (!v_range('_ftp_max', quiet, 0, 12)) return 0;
		if (!v_range('_ftp_ipmax', quiet, 0, 12)) return 0;
		if (!v_range('_ftp_rate', quiet, 0, 99999)) return 0;
		if (!v_range('_ftp_anonrate', quiet, 0, 99999)) return 0;
		if (!v_range('_ftp_staytimeout', quiet, 0, 65535)) return 0;
		if (!v_length('_ftp_custom', quiet, 0, 2048)) return 0;
	}

	return 1;
}

function save()
{
	if (aftg.isEditing()) return;
	if (!verifyFields(null, 0)) return;

	var fom = E('_fom');

	var data = aftg.getAllData();
	var r = [];
	for (var i = 0; i < data.length; ++i) r.push(data[i].join('<'));
	fom.ftp_users.value = r.join('>');

	fom.ftp_super.value = E('_f_ftp_super').checked ? 1 : 0;
	fom.log_ftp.value = E('_f_log_ftp').checked ? 1 : 0;

	form.submit(fom, 1);
}
</script>

</head>
<body>
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

<input type='hidden' name='_nextpage' value='nas-ftp.asp'>
<input type='hidden' name='_service' value='ftpd-restart'>

<input type='hidden' name='ftp_super'>
<input type='hidden' name='log_ftp'>
<input type='hidden' name='ftp_users'>

<div class='section-title'>FTP Server Configuration</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Enable FTP Server', name: 'ftp_enable', type: 'select',
		options: [['0', 'No'],['1', 'Yes, WAN and LAN'],['2', 'Yes, LAN only']],
		value: nvram.ftp_enable },
	{ title: 'FTP Port', indent: 2, name: 'ftp_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.ftp_port, 21) },
	{ title: 'Anonymous Users Access', name: 'ftp_anonymous', type: 'select',
		options: [['0', 'Disabled'],['1', 'Read/Write'],['2', 'Read Only'],['3', 'Write Only']],
		value: nvram.ftp_anonymous },
	{ title: 'Allow Super User to Login*', name: 'f_ftp_super', type: 'checkbox',
		suffix: ' <small>Allows users to connect with admin account.</small>',
		value: nvram.ftp_super == 1 },
	{ title: 'Log FTP requests and responses', name: 'f_log_ftp', type: 'checkbox',
		value: nvram.log_ftp == 1 }
]);
</script>
<br>
<small>* Avoid using this option when FTP server is enabled for WAN. IT PROVIDES FULL ACCESS TO THE ROUTER FILE SYSTEM!</small>
</div>

<div class='section-title'>Directories</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Public Root Directory*', name: 'ftp_pubroot', type: 'text', maxlen: 256, size: 32,
		suffix: ' <small>(for authenticated users access)</small>',
		value: nvram.ftp_pubroot },
	{ title: 'Private Root Directory*', name: 'ftp_pvtroot', type: 'text', maxlen: 256, size: 32,
		suffix: ' <small>(for authenticated users access in private mode)</small>',
		value: nvram.ftp_pvtroot },
	{ title: 'Anonymous Root Directory*', name: 'ftp_anonroot', type: 'text', maxlen: 256, size: 32, 
		suffix: ' <small>(for anonymous connections)</small>',
		value: nvram.ftp_anonroot },
	{ title: 'Directory Listings', name: 'ftp_dirlist', type: 'select',
		options: [['0', 'Enabled'],['1', 'Disabled'],['2', 'Disabled for Anonymous']],
		value: nvram.ftp_dirlist }
]);
</script>
<br>
<small>* When no directory is specified, /mnt is used as a root directory.</small>
</div>

<div class='section-title'>Limits</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Maximum Users Allowed to Log in', name: 'ftp_max', type: 'text', maxlen: 5, size: 7,
		suffix: ' <small>(0 - unlimited)</small>',
		value: nvram.ftp_max },
	{ title: 'Maximum Connections from the same IP', name: 'ftp_ipmax', type: 'text', maxlen: 5, size: 7,
		suffix: ' <small>(0 - unlimited)</small>',
		value: nvram.ftp_ipmax },
	{ title: 'Maximum Bandwidth for Anonymous Users', name: 'ftp_anonrate', type: 'text', maxlen: 5, size: 7,
		suffix: ' <small>KBytes/sec (0 - unlimited)</small>',
		value: nvram.ftp_anonrate },
	{ title: 'Maximum Bandwidth for Authenticated Users', name: 'ftp_rate', type: 'text', maxlen: 5, size: 7,
		suffix: ' <small>KBytes/sec (0 - unlimited)</small>',
		value: nvram.ftp_rate },
	{ title: 'Idle Timeout', name: 'ftp_staytimeout', type: 'text', maxlen: 5, size: 7,
		suffix: ' <small>seconds (0 - no timeout)</small>',
		value: nvram.ftp_staytimeout }
]);
</script>
</div>

<div class='section-title'>Custom Configuration</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: '<a href="http://vsftpd.beasts.org/" target="_new">Vsftpd</a><br>Custom Configuration', name: 'ftp_custom', type: 'textarea', value: nvram.ftp_custom }
]);
</script>
</div>

<div class='section-title'>User Accounts</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='aft-grid'></table>
	<script type='text/javascript'>aftg.setup();</script>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
