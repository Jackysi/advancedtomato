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
<title>[<% ident(); %>] Admin: SNMP</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>
<style type='text/css'>
textarea {
 width: 98%;
 height: 15em;
}
</style>
<script type='text/javascript'>
//	<% nvram("snmp_enable,snmp_port,snmp_remote,snmp_remote_sip,snmp_location,snmp_contact,snmp_ro"); %>

function verifyFields(focused, quiet)
{
	var ok = 1;

	var a = E('_f_snmp_enable').checked;

	E('_snmp_port').disabled = !a;
	E('_f_snmp_remote').disabled = !a;
	E('_snmp_remote_sip').disabled = !a;
	E('_snmp_location').disabled = !a;
	E('_snmp_contact').disabled = !a;
	E('_snmp_ro').disabled = !a;
	E('_snmp_remote_sip').disabled = (!a || !E('_f_snmp_remote').checked);

	return ok;
}

function save()
{
  if (verifyFields(null, 0)==0) return;
  var fom = E('_fom');
  fom.snmp_enable.value = E('_f_snmp_enable').checked ? 1 : 0;
  fom.snmp_remote.value = E('_f_snmp_remote').checked ? 1 : 0;

  if (fom.snmp_enable.value == 0) {
  	fom._service.value = 'snmp-stop';
  }
  else {
  	fom._service.value = 'snmp-restart,firewall-restart'; 
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
<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'>
<div id='ident'><% ident(); %></div>
<div class='section-title'>SNMP Settings</div>
<div class='section' id='config-section'>
<form id='_fom' method='post' action='tomato.cgi'>
<input type='hidden' name='_nextpage' value='admin-snmp.asp'>
<input type='hidden' name='_service' value='snmp-restart,firewall-restart'>
<input type='hidden' name='snmp_enable'>
<input type='hidden' name='snmp_remote'>

<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Enable SNMP', name: 'f_snmp_enable', type: 'checkbox', value: nvram.snmp_enable == '1' },
	null,
	{ title: 'Port', name: 'snmp_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.snmp_port, 161) },
	{ title: 'Remote access', indent: 2, name: 'f_snmp_remote', type: 'checkbox', value: nvram.snmp_remote == '1' },
	{ title: 'Allowed Remote<br>IP Address', indent: 2, name: 'snmp_remote_sip', type: 'text', maxlen: 512, size: 64, value: nvram.snmp_remote_sip,
			suffix: '<br><small>(optional; ex: "1.1.1.1", "1.1.1.0/24", "1.1.1.1 - 2.2.2.2" or "me.example.com")</small>' },
	null,
	{ title: 'Location', indent: 2, name: 'snmp_location', type: 'text', maxlen: 40, size: 64, value: nvram.snmp_location },
	{ title: 'Contact', indent: 2, name: 'snmp_contact', type: 'text', maxlen: 40, size: 64, value: nvram.snmp_contact },
	{ title: 'RO Community', indent: 2, name: 'snmp_ro', type: 'text', maxlen: 40, size: 64, value: nvram.snmp_ro }
]);
</script>
</div>
</form>
</div>
</td></tr>
<tr><td id='footer' colspan=2>
 <form>
 <span id='footer-msg'></span>
 <input type='button' value='Save' id='save-button' onclick='save()'>
 <input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
 </form>
</div>
</td></tr>
</table>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
