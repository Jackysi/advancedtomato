<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Portions Copyright (C) 2008-2010 Keith Moyer, tomatovpn@keithmoyer.com

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] OpenVPN: Server</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>
<script type='text/javascript' src='vpn.js'></script>
<script type='text/javascript'>

//	<% nvram("vpn_server_eas,vpn_server_dns,vpn_server1_poll,vpn_server1_if,vpn_server1_proto,vpn_server1_port,vpn_server1_firewall,vpn_server1_sn,vpn_server1_nm,vpn_server1_local,vpn_server1_remote,vpn_server1_dhcp,vpn_server1_r1,vpn_server1_r2,vpn_server1_crypt,vpn_server1_comp,vpn_server1_cipher,vpn_server1_reneg,vpn_server1_hmac,vpn_server1_plan,vpn_server1_ccd,vpn_server1_c2c,vpn_server1_ccd_excl,vpn_server1_ccd_val,vpn_server1_pdns,vpn_server1_rgw,vpn_server1_custom,vpn_server1_static,vpn_server1_ca,vpn_server1_crt,vpn_server1_key,vpn_server1_dh,vpn_server2_poll,vpn_server2_if,vpn_server2_proto,vpn_server2_port,vpn_server2_firewall,vpn_server2_sn,vpn_server2_nm,vpn_server2_local,vpn_server2_remote,vpn_server2_dhcp,vpn_server2_r1,vpn_server2_r2,vpn_server2_crypt,vpn_server2_comp,vpn_server2_cipher,vpn_server2_reneg,vpn_server2_hmac,vpn_server2_plan,vpn_server2_ccd,vpn_server2_c2c,vpn_server2_ccd_excl,vpn_server2_ccd_val,vpn_server2_pdns,vpn_server2_rgw,vpn_server2_custom,vpn_server2_static,vpn_server2_ca,vpn_server2_crt,vpn_server2_key,vpn_server2_dh"); %>

function CCDGrid() { return this; }
CCDGrid.prototype = new TomatoGrid;

tabs = [['server1', 'Server 1'],['server2', 'Server 2']];
sections = [['basic', 'Basic'],['advanced', 'Advanced'],['keys','Keys'],['status','Status']];
ccdTables = [];
statusUpdaters = [];
for (i = 0; i < tabs.length; ++i)
{
	ccdTables.push(new CCDGrid());
	statusUpdaters.push(new StatusUpdater());
}
ciphers = [['default','Use Default'],['none','None']];
for (i = 0; i < vpnciphers.length; ++i) ciphers.push([vpnciphers[i],vpnciphers[i]]);

changed = 0;
vpn1up = parseInt('<% psup("vpnserver1"); %>');
vpn2up = parseInt('<% psup("vpnserver2"); %>');

function updateStatus(num)
{
	var xob = new XmlHttp();
	xob.onCompleted = function(text, xml)
	{
		statusUpdaters[num].update(text);
		xob = null;
	}
	xob.onError = function(ex)
	{
		statusUpdaters[num].errors.innerHTML += 'ERROR! '+ex+'<br>';
		xob = null;
	}

	xob.post('/vpnstatus.cgi', 'server=' + (num+1));
}

function tabSelect(name)
{
	tgHideIcons();

	tabHigh(name);

	for (var i = 0; i < tabs.length; ++i)
	{
		var on = (name == tabs[i][0]);
		elem.display(tabs[i][0] + '-tab', on);
	}

	cookie.set('vpn_server_tab', name);
}

function sectSelect(tab, section)
{
	tgHideIcons();

	for (var i = 0; i < sections.length; ++i)
	{
		if (section == sections[i][0])
		{
			elem.addClass(tabs[tab][0]+'-'+sections[i][0]+'-tab', 'active');
			elem.display(tabs[tab][0]+'-'+sections[i][0], true);
		}
		else
		{
			elem.removeClass(tabs[tab][0]+'-'+sections[i][0]+'-tab', 'active');
			elem.display(tabs[tab][0]+'-'+sections[i][0], false);
		}
	}

	cookie.set('vpn_server'+tab+'_section', section);
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
	tgHideIcons();

	var ret = 1;

	// When settings change, make sure we restart the right services
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
			
			if ((focused.name.indexOf("_dns")>=0 || (focused.name.indexOf("_if")>=0 && E('_f_vpn_server'+servernumber+'_dns').checked)) &&
			    fom._service.value.indexOf('dnsmasq') < 0)
			{
				if ( fom._service.value != "" ) fom._service.value += ",";
				fom._service.value += 'dnsmasq-restart';
			}

			if (focused.name.indexOf("_c2c") >= 0)
				ccdTables[servernumber-1].reDraw();
		}
	}

	// Element varification
	for (i = 0; i < tabs.length; ++i)
	{
		t = tabs[i][0];

		if (!v_range('_vpn_'+t+'_poll', quiet, 0, 1440)) ret = 0;
		if (!v_port('_vpn_'+t+'_port', quiet)) ret = 0;
		if (!v_ip('_vpn_'+t+'_sn', quiet, 0)) ret = 0;
		if (!v_netmask('_vpn_'+t+'_nm', quiet)) ret = 0;
		if (!v_ip('_vpn_'+t+'_r1', quiet, 1)) ret = 0;
		if (!v_ip('_vpn_'+t+'_r2', quiet, 1)) ret = 0;
		if (!v_ip('_vpn_'+t+'_local', quiet, 1)) ret = 0;
		if (!v_ip('_vpn_'+t+'_remote', quiet, 1)) ret = 0;
		if (!v_range('_vpn_'+t+'_reneg', quiet, -1, 2147483647)) ret = 0;
	}

	// Visability changes
	for (i = 0; i < tabs.length; ++i)
	{
		t = tabs[i][0];

		auth = E('_vpn_'+t+'_crypt');
		iface = E('_vpn_'+t+'_if');
		hmac = E('_vpn_'+t+'_hmac');
		dhcp = E('_f_vpn_'+t+'_dhcp');
		ccd = E('_f_vpn_'+t+'_ccd');
		dns = E('_f_vpn_'+t+'_dns');

		elem.display(PR('_vpn_'+t+'_ca'), PR('_vpn_'+t+'_crt'), PR('_vpn_'+t+'_dh'), PR('_vpn_'+t+'_key'),
		             PR('_vpn_'+t+'_hmac'), PR('_f_vpn_'+t+'_rgw'), PR('_vpn_'+t+'_reneg'), auth.value == "tls");
		elem.display(PR('_vpn_'+t+'_static'), auth.value == "secret" || (auth.value == "tls" && hmac.value >= 0));
		elem.display(E(t+'_custom_crypto_text'), auth.value == "custom");
		elem.display(PR('_vpn_'+t+'_sn'), PR('_f_vpn_'+t+'_plan'), auth.value == "tls" && iface.value == "tun");
		elem.display(PR('_f_vpn_'+t+'_dhcp'), auth.value == "tls" && iface.value == "tap");
		elem.display(E(t+'_range'), !dhcp.checked);
		elem.display(PR('_vpn_'+t+'_local'), auth.value == "secret" && iface.value == "tun");
		elem.display(PR('_f_vpn_'+t+'_ccd'), auth.value == "tls");
		elem.display(PR('_f_vpn_'+t+'_c2c'),PR('_f_vpn_'+t+'_ccd_excl'),PR('table_'+t+'_ccd'), auth.value == "tls" && ccd.checked);
		elem.display(PR('_f_vpn_'+t+'_pdns'), auth.value == "tls" && dns.checked );

		keyHelp = E(t+'-keyhelp');
		switch (auth.value)
		{
		case "tls":
			keyHelp.href = helpURL['TLSKeys'];
			break;
		case "secret":
			keyHelp.href = helpURL['staticKeys'];
			break;
		default:
			keyHelp.href = helpURL['howto'];
			break;
		}
	}

	return ret;
}

CCDGrid.prototype.verifyFields = function(row, quiet)
{
	var ret = 1;

	// When settings change, make sure we restart the right server
	var fom = E('_fom');
	var servernum = 1;
	for (i = 0; i < tabs.length; ++i)
	{
		if (ccdTables[i] == this)
		{
			servernum = i+1;
			if (eval('vpn'+(i+1)+'up') && fom._service.value.indexOf('server'+(i+1)) < 0)
			{
				if ( fom._service.value != "" ) fom._service.value += ",";
				fom._service.value += 'vpnserver'+(i+1)+'-restart';
			}
		}
	}

	var f = fields.getAll(row);

	// Verify fields in this row of the table
	if (f[1].value == "") { ferror.set(f[1], "Common name is mandatory.", quiet); ret = 0; }
	if (f[1].value.indexOf('>') >= 0 || f[1].value.indexOf('<') >= 0) { ferror.set(f[1], "Common name cannot contain '<' or '>' characters.", quiet); ret = 0; }
	if (f[2].value != "" && !v_ip(f[2],quiet,0)) ret = 0;
	if (f[3].value != "" && !v_netmask(f[3],quiet)) ret = 0;
	if (f[2].value == "" && f[3].value != "" ) { ferror.set(f[2], "Either both or neither subnet and netmask must be provided.", quiet); ret = 0; }
	if (f[3].value == "" && f[2].value != "" ) { ferror.set(f[3], "Either both or neither subnet and netmask must be provided.", quiet); ret = 0; }
	if (f[4].checked && (f[2].value == "" || f[3].value == "")) { ferror.set(f[4], "Cannot push routes if they're not given. Please provide subnet/netmask.", quiet); ret = 0; }

	return ret;
}

CCDGrid.prototype.fieldValuesToData = function(row)
{
	var f = fields.getAll(row);
	return [f[0].checked?1:0, f[1].value, f[2].value, f[3].value, f[4].checked?1:0];
}

CCDGrid.prototype.dataToView = function(data)
{
	var c2c = false;
	for (i = 0; i < tabs.length; ++i)
	{
		if (ccdTables[i] == this && E('_f_vpn_server'+(i+1)+'_c2c').checked )
			c2c = true;
	}

	var temp = ['<input type=\'checkbox\' style="opacity:1" disabled'+(data[0]!=0?' checked':'')+'>',
	            data[1],
	            data[2],
	            data[3],
	            c2c?'<input type=\'checkbox\' style="opacity:1" disabled'+(data[4]!=0?' checked':'')+'>':'N/A'];

	var v = [];
	for (var i = 0; i < temp.length; ++i)
		v.push(i==0||i==4?temp[i]:escapeHTML('' + temp[i]));

	return v;
}

CCDGrid.prototype.dataToFieldValues = function(data)
{
	return [data[0] == 1, data[1], data[2], data[3], data[4] == 1];
}

CCDGrid.prototype.reDraw = function()
{
	var i, j, header, data, view;
	data = this.getAllData();
	header = this.header ? this.header.rowIndex + 1 : 0;
	for (i = 0; i < data.length; ++i)
	{
		view = this.dataToView(data[i]);
		for (j = 0; j < view.length; ++j)
			this.tb.rows[i+header].cells[j].innerHTML = view[j];
	}
}

function save()
{
	if (!verifyFields(null, false)) return;

	var fom = E('_fom');

	E('vpn_server_eas').value = '';
	E('vpn_server_dns').value = '';

	for (i = 0; i < tabs.length; ++i)
	{
		if (ccdTables[i].isEditing()) return;

		t = tabs[i][0];

		if ( E('_f_vpn_'+t+'_eas').checked )
			E('vpn_server_eas').value += ''+(i+1)+',';

		if ( E('_f_vpn_'+t+'_dns').checked )
			E('vpn_server_dns').value += ''+(i+1)+',';

		var data = ccdTables[i].getAllData();
		var ccd = '';

		for (j = 0; j < data.length; ++j)
			ccd += data[j].join('<') + '>';

		E('vpn_'+t+'_dhcp').value = E('_f_vpn_'+t+'_dhcp').checked ? 1 : 0;
		E('vpn_'+t+'_plan').value = E('_f_vpn_'+t+'_plan').checked ? 1 : 0;
		E('vpn_'+t+'_ccd').value = E('_f_vpn_'+t+'_ccd').checked ? 1 : 0;
		E('vpn_'+t+'_c2c').value = E('_f_vpn_'+t+'_c2c').checked ? 1 : 0;
		E('vpn_'+t+'_ccd_excl').value = E('_f_vpn_'+t+'_ccd_excl').checked ? 1 : 0;
		E('vpn_'+t+'_ccd_val').value = ccd;
		E('vpn_'+t+'_pdns').value = E('_f_vpn_'+t+'_pdns').checked ? 1 : 0;
		E('vpn_'+t+'_rgw').value = E('_f_vpn_'+t+'_rgw').checked ? 1 : 0;
	}

	form.submit(fom, 1);

	changed = 0;
}

function init()
{
	tabSelect(cookie.get('vpn_server_tab') || tabs[0][0]);

	for (i = 0; i < tabs.length; ++i)
	{
		sectSelect(i, cookie.get('vpn_server'+i+'_section') || sections[0][0]);

		t = tabs[i][0];

		ccdTables[i].init('table_'+t+'_ccd', 'sort', 0, [{ type: 'checkbox' }, { type: 'text' }, { type: 'text', maxlen: 15 }, { type: 'text', maxlen: 15 }, { type: 'checkbox' }]);
		ccdTables[i].headerSet(['Enable', 'Common Name', 'Subnet', 'Netmask', 'Push']);
		var ccdVal = eval( 'nvram.vpn_'+t+'_ccd_val' );
		if (ccdVal.length)
		{
			var s = ccdVal.split('>');
			for (var j = 0; j < s.length; ++j)
			{
				if (!s[j].length) continue;
				var row = s[j].split('<');
				if (row.length == 5)
					ccdTables[i].insertData(-1, row);
			}
		}
		ccdTables[i].showNewEditor();
		ccdTables[i].resetNewEditor();

		statusUpdaters[i].init(t+'-status-clients-table',t+'-status-routing-table',t+'-status-stats-table',t+'-status-time',t+'-status-content',t+'-no-status',t+'-status-errors');
		updateStatus(i);
	}

	verifyFields(null, true);
}
</script>

<style type='text/css'>
textarea
{
	width: 98%;
	height: 10em;
}
p.keyhelp
{
	font-size: smaller;
	font-style: italic;
}
div.status-header p
{
	font-weight: bold;
	padding-bottom: 4px;
}
table.status-table
{
	width: auto;
	margin-left: auto;
	margin-right: auto;
	text-align: center;
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
<input type='hidden' name='vpn_server_eas' id='vpn_server_eas' value=''>
<input type='hidden' name='vpn_server_dns' id='vpn_server_dns' value=''>

<div class='section-title'>OpenVPN Server Configuration</div>
<div class='section'>
<script type='text/javascript'>
tabCreate.apply(this, tabs);

for (i = 0; i < tabs.length; ++i)
{
	t = tabs[i][0];
	W('<div id=\''+t+'-tab\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_dhcp\' name=\'vpn_'+t+'_dhcp\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_plan\' name=\'vpn_'+t+'_plan\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_ccd\' name=\'vpn_'+t+'_ccd\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_c2c\' name=\'vpn_'+t+'_c2c\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_ccd_excl\' name=\'vpn_'+t+'_ccd_excl\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_ccd_val\' name=\'vpn_'+t+'_ccd_val\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_pdns\' name=\'vpn_'+t+'_pdns\'>');
	W('<input type=\'hidden\' id=\'vpn_'+t+'_rgw\' name=\'vpn_'+t+'_rgw\'>');

	W('<ul class="tabs">');
	for (j = 0; j < sections.length; j++)
	{
		W('<li><a href="javascript:sectSelect('+i+', \''+sections[j][0]+'\')" id="'+t+'-'+sections[j][0]+'-tab">'+sections[j][1]+'</a></li>');
	}
	W('</ul><div class=\'tabs-bottom\'></div>');

	W('<div id=\''+t+'-basic\'>');
	createFieldTable('', [
		{ title: 'Start with WAN', name: 'f_vpn_'+t+'_eas', type: 'checkbox', value: nvram.vpn_server_eas.indexOf(''+(i+1)) >= 0 },
		{ title: 'Interface Type', name: 'vpn_'+t+'_if', type: 'select', options: [ ['tap','TAP'], ['tun','TUN'] ], value: eval( 'nvram.vpn_'+t+'_if' ) },
		{ title: 'Protocol', name: 'vpn_'+t+'_proto', type: 'select', options: [ ['udp','UDP'], ['tcp-server','TCP'] ], value: eval( 'nvram.vpn_'+t+'_proto' ) },
		{ title: 'Port', name: 'vpn_'+t+'_port', type: 'text', value: eval( 'nvram.vpn_'+t+'_port' ) },
		{ title: 'Firewall', name: 'vpn_'+t+'_firewall', type: 'select', options: [ ['auto', 'Automatic'], ['external', 'External Only'], ['custom', 'Custom'] ], value: eval( 'nvram.vpn_'+t+'_firewall' ) },
		{ title: 'Authorization Mode', name: 'vpn_'+t+'_crypt', type: 'select', options: [ ['tls', 'TLS'], ['secret', 'Static Key'], ['custom', 'Custom'] ], value: eval( 'nvram.vpn_'+t+'_crypt' ),
			suffix: '<span id=\''+t+'_custom_crypto_text\'>&nbsp;<small>(must configure manually...)</small></span>' },
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
			{ name: 'vpn_'+t+'_remote', type: 'text', maxlen: 15, size: 17, value: eval( 'nvram.vpn_'+t+'_remote' ) } ] }
	]);
	W('</div>');
	W('<div id=\''+t+'-advanced\'>');
	createFieldTable('', [
		{ title: 'Poll Interval', name: 'vpn_'+t+'_poll', type: 'text', maxlen: 4, size: 5, value: eval( 'nvram.vpn_'+t+'_poll' ), suffix: '&nbsp;<small>(in minutes, 0 to disable)</small>' }, 
		{ title: 'Push LAN to clients', name: 'f_vpn_'+t+'_plan', type: 'checkbox', value: eval( 'nvram.vpn_'+t+'_plan' ) != 0 },
		{ title: 'Direct clients to<br>redirect Internet traffic', name: 'f_vpn_'+t+'_rgw', type: 'checkbox', value: eval( 'nvram.vpn_'+t+'_rgw' ) != 0 },
		{ title: 'Respond to DNS', name: 'f_vpn_'+t+'_dns', type: 'checkbox', value: nvram.vpn_server_dns.indexOf(''+(i+1)) >= 0 },
		{ title: 'Advertise DNS to clients', name: 'f_vpn_'+t+'_pdns', type: 'checkbox', value: eval( 'nvram.vpn_'+t+'_pdns' ) != 0 },
		{ title: 'Encryption cipher', name: 'vpn_'+t+'_cipher', type: 'select', options: ciphers, value: eval( 'nvram.vpn_'+t+'_cipher' ) },
		{ title: 'Compression', name: 'vpn_'+t+'_comp', type: 'select', options: [ ['-1', 'Disabled'], ['no', 'None'], ['yes', 'Enabled'], ['adaptive', 'Adaptive'] ], value: eval( 'nvram.vpn_'+t+'_comp' ) },
		{ title: 'TLS Renegotiation Time', name: 'vpn_'+t+'_reneg', type: 'text', maxlen: 10, size: 7, value: eval( 'nvram.vpn_'+t+'_reneg' ),
			suffix: '&nbsp;<small>(in seconds, -1 for default)</small>' },
		{ title: 'Manage Client-Specific Options', name: 'f_vpn_'+t+'_ccd', type: 'checkbox', value: eval( 'nvram.vpn_'+t+'_ccd' ) != 0 },
		{ title: 'Allow Client<->Client', name: 'f_vpn_'+t+'_c2c', type: 'checkbox', value: eval( 'nvram.vpn_'+t+'_c2c' ) != 0 },
		{ title: 'Allow Only These Clients', name: 'f_vpn_'+t+'_ccd_excl', type: 'checkbox', value: eval( 'nvram.vpn_'+t+'_ccd_excl' ) != 0 },
		{ title: '', suffix: '<table class=\'tomato-grid\' id=\'table_'+t+'_ccd\'></table>' },
		{ title: 'Custom Configuration', name: 'vpn_'+t+'_custom', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_custom' ) }
	]);
	W('</div>');
	W('<div id=\''+t+'-keys\'>');
	W('<p class=\'keyhelp\'>For help generating keys, refer to the OpenVPN <a id=\''+t+'-keyhelp\'>HOWTO</a>.</p>');
	createFieldTable('', [
		{ title: 'Static Key', name: 'vpn_'+t+'_static', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_static' ) },
		{ title: 'Certificate Authority', name: 'vpn_'+t+'_ca', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_ca' ) },
		{ title: 'Server Certificate', name: 'vpn_'+t+'_crt', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_crt' ) },
		{ title: 'Server Key', name: 'vpn_'+t+'_key', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_key' ) },
		{ title: 'Diffie Hellman parameters', name: 'vpn_'+t+'_dh', type: 'textarea', value: eval( 'nvram.vpn_'+t+'_dh' ) }
	]);
	W('</div>');
	W('<div id=\''+t+'-status\'>');
		W('<div id=\''+t+'-no-status\'><p>Server is not running or status could not be read.</p></div>');
		W('<div id=\''+t+'-status-content\' style=\'display:none\' class=\'status-content\'>');
			W('<div id=\''+t+'-status-header\' class=\'status-header\'><p>Data current as of <span id=\''+t+'-status-time\'></span>.</p></div>');
			W('<div id=\''+t+'-status-clients\'><div class=\'section-title\'>Client List</div><table class=\'tomato-grid status-table\' id=\''+t+'-status-clients-table\'></table><br></div>');
			W('<div id=\''+t+'-status-routing\'><div class=\'section-title\'>Routing Table</div><table class=\'tomato-grid status-table\' id=\''+t+'-status-routing-table\'></table><br></div>');
			W('<div id=\''+t+'-status-stats\'><div class=\'section-title\'>General Statistics</div><table class=\'tomato-grid status-table\' id=\''+t+'-status-stats-table\'></table><br></div>');
			W('<div id=\''+t+'-status-errors\' class=\'error\'></div>');
		W('</div>');
		W('<div style=\'text-align:right\'><a href=\'javascript:updateStatus('+i+')\'>Refresh Status</a></div>');
	W('</div>');
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
<script type='text/javascript'>init();</script>
</body>
</html>

