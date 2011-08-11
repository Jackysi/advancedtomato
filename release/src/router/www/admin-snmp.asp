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
<title>[<% ident(); %>] <% translate("Admin"); %>: <% translate("SNMP"); %></title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='<% nv('web_css'); %>.css'>
<script type='text/javascript' src='tomato.js'></script>
<style type='text/css'>
textarea {
 width: 98%;
 height: 15em;
}
</style>
<script type='text/javascript'>
//	<% nvram("snmp_enable,snmp_location,snmp_contact,snmp_ro"); %>

function verifyFields(focused, quiet)
{
	var ok = 1;

	var a = E('_f_snmp_enable').checked;

	E('_snmp_location').disabled = !a;
	E('_snmp_contact').disabled = !a;
	E('_snmp_ro').disabled = !a;

	return ok;
}

function save()
{
  if (verifyFields(null, 0)==0) return;
  var fom = E('_fom');
  fom.snmp_enable.value = E('_f_snmp_enable').checked ? 1 : 0;

  if (fom.snmp_enable.value == 0) {
  	fom._service.value = 'snmp-stop';
  }
  else {
  	fom._service.value = 'snmp-restart'; 
  }
	form.submit('_fom', 1);
}

function init()
{
}
</script>
</head>

<body onLoad="init()">
<table id='container' cellspacing=0>
<tr><td colspan=2 id='header'>
<div class='title'>Tomato</div>
<div class='version'><% translate("Version"); %> <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>
<div class='section-title'><% translate("SNMP Settings"); %></div>
<div class='section' id='config-section'>
<form id='_fom' method='post' action='tomato.cgi'>
<input type='hidden' name='_nextpage' value='admin-snmp.asp'>
<input type='hidden' name='_service' value='snmp-restart'>
<input type='hidden' name='snmp_enable'>

<script type='text/javascript'>
createFieldTable('', [
	{ title: '<% translate("Enable SNMP"); %>', name: 'f_snmp_enable', type: 'checkbox', value: nvram.snmp_enable == '1' },
	{ title: '<% translate("Location"); %>', indent: 2, name: 'snmp_location', type: 'text', maxlen: 20, size: 25, value: nvram.snmp_location },
	{ title: '<% translate("Contact"); %>', indent: 2, name: 'snmp_contact', type: 'text', maxlen: 20, size: 25, value: nvram.snmp_contact },
	{ title: '<% translate("RO Community"); %>', indent: 2, name: 'snmp_ro', type: 'text', maxlen: 20, size: 25, value: nvram.snmp_ro }
]);
</script>
</div>
</form>
</div>
</td></tr>
<tr><td id='footer' colspan=2>
 <form>
 <span id='footer-msg'></span>
 <input type='button' value='<% translate("Save"); %>' id='save-button' onclick='save()'>
 <input type='button' value='<% translate("Cancel"); %>' id='cancel-button' onclick='javascript:reloadPage();'>
 </form>
</div>
</td></tr>
</table>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
