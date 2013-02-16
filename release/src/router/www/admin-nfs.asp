<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2007-2011 Shibby
	http://openlinksys.info
	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Admin: NFS Server</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<style type='text/css'>
#nfsg-grid {
	width: 100%;
}
#nfsg-grid .co1 {
	width: 15%;
}
#nfsg-grid .co2 {
	width: 20%;
}
#nfsg-grid .co3 {
	width: 10%;
}
#nfsg-grid .co4 {
	width: 7%;
}
#nfsg-grid .co5 {
	width: 13%;
}
#nfsg-grid .co6 {
	width: 35%;
}


</style>

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("nfs_enable,nfs_exports"); %>

var access = [['rw', 'Read/Write'], ['ro', 'Read only']];
var sync = [['sync', 'Yes'], ['async', 'No']];
var subtree = [['subtree_check', 'Yes'], ['no_subtree_check', 'No']];

var nfsg = new TomatoGrid();

nfsg.exist = function(f, v)
{
	var data = this.getAllData();
	for (var i = 0; i < data.length; ++i) {
		if (data[i][f] == v) return true;
	}
	return false;
}

nfsg.dataToView = function(data) {
	return [data[0], data[1], data[2],data[3], data[4], data[5]];
}

nfsg.verifyFields = function(row, quiet)
{
	var ok = 1;

	return ok;
}

nfsg.resetNewEditor = function() {
	var f;

	f = fields.getAll(this.newEditor);
	ferror.clearAll(f);
	f[0].value = '';
	f[1].value = '';
	f[2].selectedIndex = 0;
	f[3].selectedIndex = 0;
	f[4].selectedIndex = 1;
	f[5].value = 'no_root_squash';
}

nfsg.setup = function()
{
	this.init('nfsg-grid', '', 50, [
		{ type: 'text', maxlen: 50 },
		{ type: 'text', maxlen: 30 },
		{ type: 'select', options: access },
		{ type: 'select', options: sync },
		{ type: 'select', options: subtree },
		{ type: 'text', maxlen: 50 }
	]);
	this.headerSet(['Directory', 'IP Address/Subnet', 'Access', 'Sync', 'Subtree Check', 'Other Options']);
	var s = nvram.nfs_exports.split('>');
	for (var i = 0; i < s.length; ++i) {
		var t = s[i].split('<');
		if (t.length == 6) this.insertData(-1, t);
	}
	this.showNewEditor();
	this.resetNewEditor();
}

function save()
{
	var data = nfsg.getAllData();
	var exports = '';
	var i;

	if (data.length != 0) exports += data[0].join('<');
	for (i = 1; i < data.length; ++i) {
		exports += '>' + data[i].join('<');
	}

	var fom = E('_fom');
	fom.nfs_enable.value = E('_f_nfs_enable').checked ? 1 : 0;
	fom.nfs_exports.value = exports;
	form.submit(fom, 1);
}

function init()
{
	nfsg.recolor();
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

<input type='hidden' name='_nextpage' value='admin-nfs.asp'>
<input type='hidden' name='_service' value='nfs-start'>

<input type='hidden' name='nfs_enable'>
<input type='hidden' name='nfs_exports'>

<div class='section-title'>NFS Server</div>
<div class='section'>
	<script type='text/javascript'>
	createFieldTable('', [
		{ title: 'Enable NFS Server', name: 'f_nfs_enable', type: 'checkbox', value: nvram.nfs_enable != '0' }
	]);
	</script>
<br>

<div class='section-title'>Exports</div>
<div class='section'>
	<table class='tomato-grid' cellspacing=1 id='nfsg-grid'></table>
	<script type='text/javascript'>nfsg.setup();</script>
<br>
	<ul>
	<li>You can find more information on proper NFS configuration at the following website: <a href="http://nfs.sourceforge.net/nfs-howto/" target="_blanc"><b>http://nfs.sourceforge.net</b></a>.
	</ul>
<br>
</div>

</div>

<div class='section-title'>NFS Client</div>
<div class='section'>
<br>
	<ul>
	<li>If you want to mount an NFS share from other NFS Server, you can use the mount.nfs tool via telnet/ssh.
	</ul>
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
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
