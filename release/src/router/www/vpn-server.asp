<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2008 Keith Moyer, tomato@keithmoyer.com

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] VPN: Server</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>
<script type='text/javascript'>

//	<% nvram("vpn_server1_if,vpn_server1_proto,vpn_server1_port,vpn_server1_sn,vpn_server1_nm,vpn_server1_local,vpn_server1_remote,vpn_server1_dhcp,vpn_server1_r1,vpn_server1_r2,vpn_server1_crypt,vpn_server1_comp,vpn_server1_cipher,vpn_server1_hmac,vpn_server1_custom,vpn_server1_static,vpn_server1_ca,vpn_server1_crt,vpn_server1_key,vpn_server1_dh,vpn_server2_if,vpn_server2_proto,vpn_server2_port,vpn_server2_sn,vpn_server2_nm,vpn_server2_local,vpn_server2_remote,vpn_server2_dhcp,vpn_server2_r1,vpn_server2_r2,vpn_server2_crypt,vpn_server2_comp,vpn_server2_cipher,vpn_server2_hmac,vpn_server2_custom,vpn_server2_static,vpn_server2_ca,vpn_server2_crt,vpn_server2_key,vpn_server2_dh"); %>

tabs = [['server1', 'Server 1'],['server2', 'Server 2']];
ciphers = [['default','Use Default'],['none','None']<% vpnciphers(); %>];

changed = 0;
vpn1up = parseInt('<% psup("vpnserver1"); %>');
vpn2up = parseInt('<% psup("vpnserver2"); %>');

function tabSelect(name)
{
	tabHigh(name);

	for (var i = 0; i < tabs.length; ++i)
	{
		var on = (name == tabs[i][0]);
		elem.display(tabs[i][0] + '-tab', on);
	}

	cookie.set('vpn_server_tab', name);
}

function toggle(service, isup)
{
	if (changed && !confirm("Unsaved changes will be lost. Continue anyway?")) return;

	E('_' + service + '_button').disabled = true;
	form.submitHidden('service.cgi', {
		_redirect: 'vpn-server.asp',
		_sleep: '3',
		_service: service + (isup ? '-stop' : '-start')
	});
}

function verifyFields(focused, quiet)
{
	var ret = 1;

	// When settings change, make sure we restart the right server
	if (focused)
	{
		changed = 1;

		var fom = E('_fom');
		var serverindex = focused.name.indexOf("server");
		if (serverindex >= 0)
		{
			var servernumber = focused.name.substring(serverindex+6,serverindex+7);
			if (eval('vpn'+servernumber+'up') && fom._service.value.indexOf('server'+servernumber) < 0)
			{
				if ( fom._service.value != "" ) fom._service.value += ",";
				fom._service.value += 'vpnserver'+servernumber+'-restart';
			}
		}
	}

	// Element varification
	for (i = 0; i < tabs.length; ++i)
	{
		t = tabs[i][0];

		if (!v_port('_vpn_'+t+'_port', quiet)) ret = 0;
		if (!v_ip('_vpn_'+t+'_sn', quiet, 0)) ret = 0;
		if (!v_netmask('_vpn_'+t+'_nm', quiet)) ret = 0;
		if (!v_ip('_vpn_'+t+'_r1', quiet, 1)) ret = 0;
		if (!v_ip('_vpn_'+t+'_r2', quiet, 1)) ret = 0;
		if (!v_ip('_vpn_'+t+'_local', quiet, 1)) ret = 0;
		if (!v_ip('_vpn_'+t+'_remote', quiet, 1)) ret = 0;
		if (!v_length('_vpn_'+t+'_custom', quiet, 0, 1024)) ret = 0;
		if (!v_length('_vpn_'+t+'_static', quiet, 0, 1024)) ret = 0;
		if (!v_length('_vpn_'+t+'_ca', quiet, 0, 1648)) ret = 0;
		if (!v_length('_vpn_'+t+'_crt', quiet, 0, 1536)) ret = 0;
		if (!v_length('_vpn_'+t+'_key', quiet, 0, 1024)) ret = 0;
		if (!v_length('_vpn_'+t+'_dh', quiet, 0, 256)) ret = 0;
	}

	// Visability changes
	for (i = 0; i < tabs.length; ++i)
	{
		t = tabs[i][0];

		auth = E('_vpn_'+t+'_crypt');
		iface = E('_vpn_'+t+'_if');
		hmac = E('_vpn_'+t+'_hmac');
		dhcp = E('_f_vpn_'+t+'_dhcp');

		elem.display(PR('_vpn_'+t+'_ca'), PR('_vpn_'+t+'_crt'), PR('_vpn_'+t+'_dh'), PR('_vpn_'+t+'_key'), PR('_vpn_'+t+'_hmac'), auth.value == "tls");
		elem.display(PR('_vpn_'+t+'_static'), auth.value == "secret" || (auth.value == "tls" && hmac.value >= 0));
		elem.display(E(t+'_custom_crypto_text'), auth.value == "custom");
		elem.display(PR('_vpn_'+t+'_sn'), auth.value == "tls" && iface.value == "tun");
		elem.display(PR('_f_vpn_'+t+'_dhcp'), auth.value == "tls" && iface.value == "tap");
		elem.display(E(t+'_range'), !dhcp.checked);
		elem.display(PR('_vpn_'+t+'_local'), auth.value == "secret" && iface.value == "tun");
	}

	return ret;
}

function save()
{
	if (!verifyFields(null, false)) return;

	var fom = E('_fom');

	for (i = 0; i < tabs.length; ++i)
	{
		t = tabs[i][0];

		E('vpn_'+t+'_dhcp').value = E('_f_vpn_'+t+'_dhcp').checked ? 1 : 0;
	}

	form.submit(fom, 1);

	changed = 0;
}
</script>

<style type='text/css'>
textarea {
	width: 98%;
	height: 10em;
}
</style>

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

<input type='hidden' name='_nextpage' value='vpn-server.asp'>
<input type='hidden' name='_nextwait' value='5'>
<input type='hidden' name='_service' value=''>

<div class='section-title'>VPN Server Configuration</div>
<div class='section'>
<script type='text/javascript'>
tabCreate.apply(this, tabs);

for (i = 0; i < tabs.length; ++i)
{
	t = tabs[i][0];
	W('<div id=\''+t+'-tab\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_dhcp\' name=\'vpn_'+t+'_dhcp\'>');
	createFieldTable('', [
		{ title: 'Interface Type', name: 'vpn_'+t+'_if', type: 'select', options: [ ['tap','TAP'], ['tun','TUN'] ], value: eval( 'nvram.vpn_'+t+'_if' ) },
		{ title: 'Protocol', name: 'vpn_'+t+'_proto', type: 'select', options: [ ['udp','UDP'], ['tcp-server','TCP'] ], value: eval( 'nvram.vpn_'+t+'_proto' ) },
		{ title: 'Port', name: 'vpn_'+t+'_port', type: 'text', value: eval( 'nvram.vpn_'+t+'_port' ) },
		{ title: 'Authorization Mode', name: 'vpn_'+t+'_crypt', type: 'select', options: [ ['tls', 'TLS'], ['secret', 'Static Key'], ['custom', 'Custom'] ], value: eval( 'nvram.vpn_'+t+'_crypt' ),
			suffix: '<span id=\''+t+'_custom_crypto_text\'>&nbsp;<small>(configured below...)</small></span>' },
		{ title: 'Extra HMAC authorization (tls-auth)', name: 'vpn_'+t+'_hmac', type: 'select', options: [ [-1, 'Disabled'], [2, 'Bi-directional'], [0, 'Incoming (0)'], [1, 'Outgoing (1)'] ], value: eval( 'nvram.vpn_'+t+'_hmac' ) },
		{ title: 'VPN subnet/netmask', multi: [
			{ name: 'vpn_'+t+'_sn', type: 'text', maxlen: 15, size: 17, value: eval( 'nvram.vpn_'+t+'_sn' ) },
			{ name: 'vpn_'+t+'_nm', type: 'text', maxlen: 15, size: 17, value: eval( 'nvram.vpn_'+t+'_nm' ) } ] },
		{ title: 'Client address pool', multi: [
			{ name: 'f_vpn_'+t+'_dhcp', type: 'checkbox', value: eval( 'nvram.vpn_'+t+'_dhcp' ) != 0, suffix: ' DHCP ' },
			{ name: 'vpn_'+t+'_r1', type: 'text', maxlen: 15, size: 17, value: eval( 'nvram.vpn_'+t+'_r1' ), prefix: '<span id=\''+t+'_range\'>', suffix: '-' },
			{ name: 'vpn_'+t+'_r2', type: 'text', maxlen: 15, size: 17, value: eval( 'nvram.vpn_'+t+'_r2' ), suffix: '</span>' } ] },
		{ title: 'Local/remote endpoint addresses', multi: [
			{ name: 'vpn_'+t+'_local', type: 'text', maxlen: 15, size: 17, value: eval( 'nvram.vpn_'+t+'_local' ) },
			{ name: 'vpn_'+t+'_remote', type: 'text', maxlen: 15, size: 17, value: eval( 'nvram.vpn_'+t+'_remote' ) } ] },
		{ title: 'Encryption cipher', name: 'vpn_'+t+'_cipher', type: 'select', options: ciphers, value: eval( 'nvram.vpn_'+t+'_cipher' ) },
		{ title: 'Compression', name: 'vpn_'+t+'_comp', type: 'select', options: [ ['yes', 'Enabled'], ['no', 'Disabled'], ['adaptive', 'Adaptive'] ], value: eval( 'nvram.vpn_'+t+'_comp' ) },
		{ title: 'Custom Configuration', name: 'vpn_'+t+'_custom', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_custom' ) },
		{ title: 'Static Key', name: 'vpn_'+t+'_static', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_static' ) },
		{ title: 'Certificate Authority', name: 'vpn_'+t+'_ca', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_ca' ) },
		{ title: 'Server Certificate', name: 'vpn_'+t+'_crt', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_crt' ) },
		{ title: 'Server Key', name: 'vpn_'+t+'_key', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_key' ) },
		{ title: 'Diffie Hellman parameters', name: 'vpn_'+t+'_dh', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_dh' ) }
	]);
	W('<input type="button" value="' + (eval('vpn'+(i+1)+'up') ? 'Stop' : 'Start') + ' Now" onclick="toggle(\'vpn'+t+'\', vpn'+(i+1)+'up)" id="_vpn'+t+'_button">');
	W('</div>');
}

</script>
</div>

</td></tr>
	<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>tabSelect(cookie.get('vpn_server_tab') || tabs[0][0]); verifyFields(null, 1);</script>
</body>
</html>

