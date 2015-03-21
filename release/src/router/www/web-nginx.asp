<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	NGINX Web Server Management Control
	Ofer Chen (roadkill AT tomatoraf dot com)
	Vicente Soriano (victek AT tomatoraf dot com)
	Copyright (C) 2013 http://www.tomatoraf.com
	
	For use with Tomato Firmware only.
	No part of this file can be used or modified without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Web Server Menu</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<style type='text/css'>
.controls {
 	width: 90px;
	margin-top: 5px;
	margin-bottom: 10px;
}
</style>
<script type='text/javascript'>

//	<% nvram("nginx_enable,nginx_php,nginx_keepconf,nginx_port,nginx_upload,nginx_remote,nginx_fqdn,nginx_docroot,nginx_priority,nginx_custom,nginx_httpcustom,nginx_servercustom,nginx_user,nginx_phpconf,nginx_override,nginx_overridefile"); %>

changed = 0;
nginxup = parseInt ('<% psup("nginx"); %>');

function toggle(service, isup)
{
	if (changed) {
		if (!confirm("Unsaved changes will be lost. Continue anyway?")) return;
	}
	E('_' + service + '_button').disabled = true;
	form.submitHidden('/service.cgi', {
		_redirect: 'web-nginx.asp',
		_sleep: ((service == 'nginxfp') && (!isup)) ? '10' : '5',
		_service: service + (isup ? '-stop' : '-start')
	});
}

function verifyFields(focused, quiet)
{
	var ok = 1;

	var a = E('_f_nginx_enable').checked;
	var b = E('_f_nginx_override').checked;

	E('_f_nginx_php').disabled = !a ;
	E('_f_nginx_keepconf').disabled = !a || b;
	E('_nginx_port').disabled = !a || b;
	E('_nginx_upload').disabled = !a || b;
	E('_f_nginx_remote').disabled = !a;
	E('_nginx_fqdn').disabled = !a || b;
	E('_nginx_docroot').disabled = !a || b;
	E('_nginx_priority').disabled = !a || b;
	E('_nginx_custom').disabled = !a || b;
	E('_nginx_httpcustom').disabled = !a || b;
	E('_nginx_servercustom').disabled = !a || b;
	E('_nginx_user').disabled = !a;
	E('_nginx_phpconf').disabled = !a || b;
	E('_f_nginx_override').disabled = !a;
	E('_nginx_overridefile').disabled = !a || !b;

	return ok;
}

function save()
{
	if (verifyFields(null, 0)==0) return;
	var fom = E('_fom');

	fom.nginx_enable.value = E('_f_nginx_enable').checked ? 1 : 0;
	if (fom.nginx_enable.value) {
		fom.nginx_php.value = fom.f_nginx_php.checked ? 1 : 0;
		fom.nginx_keepconf.value = fom.f_nginx_keepconf.checked ? 1 : 0;
		fom.nginx_remote.value = fom.f_nginx_remote.checked ? 1 : 0;
		fom.nginx_override.value = fom.f_nginx_override.checked ? 1 : 0;
		fom._service.value = 'nginx-restart';
	} else {
		fom._service.value = 'nginx-stop';
	}
	form.submit(fom, 1);
}

function init()
{
	verifyFields(null, 1);
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

<!-- / / / -->
<div class='section-title'>Status</div>
<div class='section' id='status-section'>
<script type='text/javascript'>
	W('NGINX is currently '+(!nginxup ? 'stopped' : 'running')+' ');
	W('<input type="button" value="' + (nginxup ? 'Stop' : 'Start') + ' Now" onclick="toggle(\'nginxfp\', nginxup)" id="_nginxfp_button">');
</script>
<br>
</div>

<div class='section-title'>Basic Settings</div>
<div class='section' id='config-section'>
<form id='_fom' method='post' action='tomato.cgi'>
<input type='hidden' name='_nextpage' value='web-nginx.asp'>
<input type='hidden' name='_service' value='enginex-restart'>
<input type='hidden' name='_nextwait' value='10'>
<input type='hidden' name='_reboot' value='0'>

<input type='hidden' name='nginx_enable'>
<input type='hidden' name='nginx_php'>
<input type='hidden' name='nginx_keepconf'>
<input type='hidden' name='nginx_remote'>
<input type='hidden' name='nginx_override'>

<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Enable Server on Start', name: 'f_nginx_enable', type: 'checkbox', value: nvram.nginx_enable == '1'},
	{ title: 'Enable PHP support', name: 'f_nginx_php', type: 'checkbox', value: nvram.nginx_php == '1' },
	{ title: 'Run As', name: 'nginx_user', type: 'select',
		options: [['root','Root'],['nobody','Nobody']], value: nvram.nginx_user },
	{ title: 'Keep Config Files', name: 'f_nginx_keepconf', type: 'checkbox', value: nvram.nginx_keepconf == '1' },
	{ title: 'Web Server Port', name: 'nginx_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.nginx_port, 85), suffix: '<small> default: 85</small>' },
	{ title: 'Upload file size limit', name: 'nginx_upload', type: 'text', maxlen: 5, size: 7, value: nvram.nginx_upload, suffix: '<small> MB</small>'},
	{ title: 'Allow Remote Access', name: 'f_nginx_remote', type: 'checkbox', value: nvram.nginx_remote == '1' },
	{ title: 'Web Server Name', name: 'nginx_fqdn', type: 'text', maxlen: 255, size: 20, value: nvram.nginx_fqdn },
	{ title: 'Server Root Path', name: 'nginx_docroot', type: 'text', maxlen: 255, size: 40, value: nvram.nginx_docroot, suffix: '<small>&nbsp;/index.html / index.htm / index.php</small>' },
	{ title: 'Server Priority', name: 'nginx_priority', type: 'text', maxlen: 8, size:3, value: nvram.nginx_priority, suffix:'<small> Max. Perfor: -20, Min.Perfor: 19, default: 10</small>' }
]);
</script>
</div>
<div class='section-title'>Advanced Settings</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: '<a href="http://wiki.nginx.org/Configuration" target="_new">NGINX</a><br>HTTP Section<br>Custom configuration', name: 'nginx_httpcustom', type: 'textarea', value: nvram.nginx_httpcustom },
	{ title: '<a href="http://wiki.nginx.org/Configuration" target="_new">NGINX</a><br>SERVER Section<br>Custom configuration', name: 'nginx_servercustom', type: 'textarea', value: nvram.nginx_servercustom },
	{ title: '<a href="http://wiki.nginx.org/Configuration" target="_new">NGINX</a><br>Custom configuration', name: 'nginx_custom', type: 'textarea', value: nvram.nginx_custom },
	{ title: '<a href="http://php.net/manual/en/ini.php" target="_new">PHP</a><br>Custom configuration', name: 'nginx_phpconf', type: 'textarea', value: nvram.nginx_phpconf },
	null,
	{ title: 'Use user config file', name: 'f_nginx_override', type: 'checkbox', value: nvram.nginx_override == '1', suffix: '<small> User config file will be used, some of GUI settings will be ignored</small>' },
	{ title: 'User config file path', name: 'nginx_overridefile', type: 'text', maxlen: 255, size: 40, value: nvram.nginx_overridefile }
]);
</script>
</div>
<div class='section-title'>Notes</div>
<div class='section'>
<ul>
<li><b> Status Button:</b> Quick Start-Stop Service. Enable Web Server must be checked to modify settings.<br>
<li><b> Enable Server on Start:</b> To activate the Web Server tick and save this screen.<br>
<li><b> Enable PHP support:</b> To activate the PHP support (php-cgi) tick and save this screen.<br>
<li><b> Run As:</b> Select user used to start nginx and php-cgi daemon.<br>
<li><b> Keep Config Files:</b> Have you modified the configuration file manually? Tick this box and changes will be maintained.<br> 
<li><b> Web Server Port:</b> The Port used by the Web Server to be accessed. Check conflict when the port is used by other services.<br>
<li><b> Allow remote access:</b> This option will open the Web Server GUI port from the WAN side. Service will be accessed from the internet. <br>
<li><b> Web Server Name:</b> Name that will appear on top of your Internet Browser.<br>0
<li><b> Document Root Path:</b> The path in your router where documents are stored.<br>
<li><b> Examples:<br></b>
/tmp/mnt/HDD/www as you can find in USB mount path.<br>
<li><b> NGINX Custom Configuration:</b> You can add other values to nginx.conf to suit your needs.<br>
<li><b> NGINX HTTP Section Custom Configuration:</b> You can add other values to nginx.conf in declaration of http {} to suit your needs.<br>
<li><b> NGINX SERVER Section Custom Configuration:</b> You can add other values to nginx.conf in declaration of server {} to suit your needs.<br>
<li><b> PHP Custom Configuration:</b> You can add other values to php.ini to suit your needs.<br>
<li><b> Server Priority:</b> Sets the service priority over other processes running on the router.<br><br>
The operating system kernel has priority -5.<br>
Never select a lower value than the kernel uses. Do not use the service test page to adjust the<br>
server performance, it's performance is lower than the definitive media where files will be <br>
located, i.e; USB Stick, Hard Drive or SSD.<br>
</ul>
</div>
</form>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
<form>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save()'>
	<input type='button' value='Cancel' id='cancel-button' onclick='javascript:reloadPage();'>
</form>
</td></tr>
</table>
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
