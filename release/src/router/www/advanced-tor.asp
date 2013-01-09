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
<title>[<% ident(); %>] Advanced: TOR Project</title>
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
//	<% nvram("tor_enable,tor_socksport,tor_transport,tor_dnsport,tor_datadir,tor_users,tor_custom,tor_iface,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

function verifyFields(focused, quiet)
{
	var ok = 1;

	var a = E('_f_tor_enable').checked;
	var o = (E('_tor_iface').value == 'custom');

	E('_tor_socksport').disabled = !a;
	E('_tor_transport').disabled = !a;
	E('_tor_dnsport').disabled = !a;
	E('_tor_datadir').disabled = !a;
	E('_tor_iface').disabled = !a;
	E('_tor_custom').disabled = !a;

	elem.display('_tor_users', o && a);

	var bridge = E('_tor_iface');
	if(nvram.lan_ifname.length < 1)
		bridge.options[0].disabled=true;
	if(nvram.lan1_ifname.length < 1)
		bridge.options[1].disabled=true;
	if(nvram.lan2_ifname.length < 1)
		bridge.options[2].disabled=true;
	if(nvram.lan3_ifname.length < 1)
		bridge.options[3].disabled=true;

	var s = E('_tor_custom');

	if (s.value.search(/SocksPort/) == 0)  {
		ferror.set(s, 'Cannot set "SocksPort" option here. You can set it in Tomato GUI', quiet);
		ok = 0; }

	if (s.value.search(/SocksBindAddress/) == 0)  {
		ferror.set(s, 'Cannot set "SocksBindAddress" option here.', quiet);
		ok = 0; }

	if (s.value.search(/AllowUnverifiedNodes/) == 0)  {
		ferror.set(s, 'Cannot set "AllowUnverifiedNodes" option here.', quiet);
		ok = 0; }

	if (s.value.search(/Log/) == 0)  {
		ferror.set(s, 'Cannot set "Log" option here.', quiet);
		ok = 0; }

	if (s.value.search(/DataDirectory/) == 0)  {
		ferror.set(s, 'Cannot set "DataDirectory" option here. You can set it in Tomato GUI', quiet);
		ok = 0; }

	if (s.value.search(/TransPort/) == 0)  {
		ferror.set(s, 'Cannot set "TransPort" option here. You can set it in Tomato GUI', quiet);
		ok = 0; }

	if (s.value.search(/TransListenAddress/) == 0)  {
		ferror.set(s, 'Cannot set "TransListenAddress" option here.', quiet);
		ok = 0; }

	if (s.value.search(/DNSPort/) == 0)  {
		ferror.set(s, 'Cannot set "DNSPort" option here. You can set it in Tomato GUI', quiet);
		ok = 0; }

	if (s.value.search(/DNSListenAddress/) == 0)  {
		ferror.set(s, 'Cannot set "DNSListenAddress" option here.', quiet);
		ok = 0; }

	if (s.value.search(/User/) == 0)  {
		ferror.set(s, 'Cannot set "User" option here.', quiet);
		ok = 0; }

	return ok;
}

function save()
{
  if (verifyFields(null, 0)==0) return;
  var fom = E('_fom');
  fom.tor_enable.value = E('_f_tor_enable').checked ? 1 : 0;

  if (fom.tor_enable.value == 0) {
  	fom._service.value = 'tor-stop';
  }
  else {
  	fom._service.value = 'tor-restart,firewall-restart'; 
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
<div class='section-title'>TOR Settings</div>
<div class='section' id='config-section'>
<form id='_fom' method='post' action='tomato.cgi'>
<input type='hidden' name='_nextpage' value='advanced-tor.asp'>
<input type='hidden' name='_service' value='tor-restart'>
<input type='hidden' name='tor_enable'>

<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Enable TOR', name: 'f_tor_enable', type: 'checkbox', value: nvram.tor_enable == '1' },
	null,
	{ title: 'Socks Port', name: 'tor_socksport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_socksport, 9050) },
	{ title: 'Trans Port', name: 'tor_transport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_transport, 9040) },
	{ title: 'DNS Port', name: 'tor_dnsport', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.tor_dnsport, 9053) },
	{ title: 'Data Directory', name: 'tor_datadir', type: 'text', maxlen: 24, size: 28, value: nvram.tor_datadir },
	null,
	{ title: 'Redirect all users from', multi: [
		{ name: 'tor_iface', type: 'select', options: [
			['br0','LAN (br0)'],
			['br1','LAN1 (br1)'],
			['br2','LAN2 (br2)'],
			['br3','LAN3 (br3)'],
			['custom','Only selected IP`s']
				], value: nvram.tor_iface },
		{ name: 'tor_users', type: 'text', maxlen: 512, size: 64, value: nvram.tor_users } ] },
	null,
	{ title: 'Custom Configuration', name: 'tor_custom', type: 'textarea', value: nvram.tor_custom }
]);
</script>
</div>
<div class='section-title'>Notes</div>
<div class='section'>
<ul>
	<li><b>Enable TOR</b> - Be patient. Starting the TOR client can take from several seconds to several minutes.
	<li><b>Only selected IP`s</b> - ex: 1.2.3.4,1.1.0/24,1.2.3.1-1.2.3.4
	<li>•Only connections to destination port 80 are redirected to TOR.
	<li>•Caution! - If your router only has 32MB of RAM, you'll have to use swap.
</ul>
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
