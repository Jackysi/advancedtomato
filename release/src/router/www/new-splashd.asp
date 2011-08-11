<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	Copyright (C) 2011 Ofer Chen (Roadkill), Vicente Soriano (Victek)
	Adapted & Modified from Dual WAN Tomato Firmware.

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] RAF: <% translate("Captive Portal"); %></title>
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
//	<% nvram("NC_enable,NC_Verbosity,NC_GatewayName,NC_GatewayPort,NC_ForcedRedirect,NC_HomePage,NC_DocumentRoot,NC_LoginTimeout,NC_IdleTimeout,NC_MaxMissedARP,NC_ExcludePorts,NC_IncludePorts,NC_AllowedWebHosts,NC_MACWhiteList"); %>
function fix(name)
{
 var i;
 if (((i = name.lastIndexOf('/')) > 0) || ((i = name.lastIndexOf('\\')) > 0))
 name = name.substring(i + 1, name.length);
 return name;
}

function uploadButton()
{
 var fom;
 var name;
 var i;
 name = fix(E('upload-name').value);
 name = name.toLowerCase();
 if ((name.length <= 5) || (name.substring(name.length - 5, name.length).toLowerCase() != '.html')) {
 alert('<% translate("Wrong filename, the correct extension is"); %> ".html".');
 return;
 }
 if (!confirm('<% translate("Are you sure the file"); %>' + name + '<% translate("must be uploaded to the device"); %>?')) return;
 E('upload-button').disabled = 1;
 fields.disableAll(E('config-section'), 1);
 fields.disableAll(E('footer'), 1);
 E('upload-form').submit();
}

function verifyFields(focused, quiet)
{
	var a = E('_f_NC_enable').checked;

	E('_NC_Verbosity').disabled = !a;
	E('_NC_GatewayName').disabled = !a;
	E('_NC_GatewayPort').disabled = !a;
	E('_f_NC_ForcedRedirect').disabled = !a;
	E('_NC_HomePage').disabled = !a;
	E('_NC_DocumentRoot').disabled = !a;
	E('_NC_LoginTimeout').disabled = !a;
	E('_NC_IdleTimeout').disabled = !a;
	E('_NC_MaxMissedARP').disabled = !a;
	E('_NC_ExcludePorts').disabled = !a;
	E('_NC_IncludePorts').disabled = !a;
	E('_NC_AllowedWebHosts').disabled = !a;
	E('_NC_MACWhiteList').disabled = !a;

	if ( (E('_f_NC_ForcedRedirect').checked) && (!v_length('_NC_HomePage', quiet, 1, 255))) return 0;
	if (!v_length('_NC_GatewayName', quiet, 1, 255)) return 0;	
	if ( (E('_NC_IdleTimeout').value != '0') && (!v_range('_NC_IdleTimeout', quiet, 300))) return 0;
	return 1;
}

function save()
{
  if (verifyFields(null, 0)==0) return;
  var fom = E('_fom');
  fom.NC_enable.value = E('_f_NC_enable').checked ? 1 : 0;
  fom.NC_ForcedRedirect.value = E('_f_NC_ForcedRedirect').checked ? 1 : 0;

  // blank spaces with commas
  e = E('_NC_ExcludePorts');
  e.value = e.value.replace(/\,+/g, ' ');

  e = E('_NC_IncludePorts');
  e.value = e.value.replace(/\,+/g, ' ');

  e = E('_NC_AllowedWebHosts');
  e.value = e.value.replace(/\,+/g, ' ');
  
  e = E('_NC_MACWhiteList');
  e.value = e.value.replace(/\,+/g, ' ');

  fields.disableAll(E('upload-section'), 1);
  if (fom.NC_enable.value == 0) {
	fom._service.value = 'splashd-stop';
  }
	else {
	fom._service.value = 'splashd-restart';
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
<div class='section-title'><% translate("Captive Portal Management"); %></div>
<div class='section' id='config-section'>
<form id='_fom' method='post' action='tomato.cgi'>
<input type='hidden' name='_nextpage' value='new-splashd.asp'>
<input type='hidden' name='_service' value='splashd-restart'>
<input type='hidden' name='NC_enable'>
<input type='hidden' name='NC_ForcedRedirect'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: '<% translate("Enable Function"); %>', name: 'f_NC_enable', type: 'checkbox', value: nvram.NC_enable == '1' },
	{ title: '<% translate("Gateway Name"); %>', name: 'NC_GatewayName', type: 'text', maxlen: 255, size: 34, value: nvram.NC_GatewayName },
	{ title: '<% translate("Captive Site Forwarding"); %>', name: 'f_NC_ForcedRedirect', type: 'checkbox', value: (nvram.NC_ForcedRedirect == '1') },
	{ title: '<% translate("Home Page"); %>', name: 'NC_HomePage', type: 'text', maxlen: 255, size: 34, value: nvram.NC_HomePage },
	{ title: '<% translate("Welcome html Path"); %>', name: 'NC_DocumentRoot', type: 'text', maxlen: 255, size: 20, value: nvram.NC_DocumentRoot, suffix: '<span>&nbsp;/splash.html</span>' },
	{ title: '<% translate("Logged Timeout"); %>', name: 'NC_LoginTimeout', type: 'text', maxlen: 8, size: 4, value: nvram.NC_LoginTimeout },
	{ title: '<% translate("Idle Timeout"); %>', name: 'NC_IdleTimeout', type: 'text', maxlen: 8, size: 4, value: nvram.NC_IdleTimeout },
	{ title: '<% translate("Max Missed ARP"); %>', name: 'NC_MaxMissedARP', type: 'text', maxlen: 10, size: 2, value: nvram.NC_MaxMissedARP },
	null,
	{ title: '<% translate("Log Info Level"); %>', name: 'NC_Verbosity', type: 'text', maxlen: 10, size: 2, value: nvram.NC_Verbosity },
	{ title: '<% translate("Gateway Port"); %>', name: 'NC_GatewayPort', type: 'text', maxlen: 10, size: 7, value: fixPort(nvram.NC_GatewayPort, 5280) },
	{ title: '<% translate("Excluded Ports to be redirected"); %>', name: 'NC_ExcludePorts', type: 'text', maxlen: 255, size: 34, value: nvram.NC_ExcludePorts },
	{ title: '<% translate("Included Ports to be redirected"); %>', name: 'NC_IncludePorts', type: 'text', maxlen: 255, size: 34, value: nvram.NC_IncludePorts },
	{ title: '<% translate("URL Excluded off Captive Portal"); %>', name: 'NC_AllowedWebHosts', type: 'text', maxlen: 255, size: 34, value: nvram.NC_AllowedWebHosts },
	{ title: '<% translate("MAC Address Whitelist"); %>', name: 'NC_MACWhiteList', type: 'text', maxlen: 255, size: 34, value: nvram.NC_MACWhiteList }
]);
</script>
</form>
</div>
<br>
<div class='section-title'><% translate("Customized Splash File Path"); %></div>
<div class='section' id='upload-section'>
 <form id='upload-form' method='post' action='uploadsplash.cgi?_http_id=<% nv(http_id); %>' encType='multipart/form-data'>
 <input type='file' size='40' id='upload-name' name='upload_name'>
 <input type='button' name='f_upload_button' id='upload-button' value='<% translate("Ok"); %>' onclick='uploadButton()'>
 <br>
 </form>
</div>
<hr>
<span style='color:blue'>
<b><% translate("Captive Portal. User Guide"); %>.</b><br>
<br>
<b>*- <% translate("Enable Function"); %>:</b> <% translate("When you tick and save the router will show a Welcome Banner when a computer access the Internet"); %>.<br>
<b>*- <% translate("Gateway Name"); %>:</b> <% translate("The name of the Gateway appearing in the welcome banner"); %><br>
<b>*- <% translate("Captive Site Forwarding"); %>:</b> <% translate("When active, the 'Home Page' (read next line) will appear after you Agree in Welcome Banner"); %>.<br>
<b>*- <% translate("Home Page"); %>:</b> <% translate("The URL that will appear after you Agree the Welcome Banner"); %>.<br>
<b>*- <% translate("Welcome html Path"); %>:</b> <% translate("The location where the Welcome banner is located"); %><br>
<b>*- <% translate("Logged Timeout"); %>:</b> <% translate("During this period of time no Welcome banner will appear when you access to the device. Default"); %>=3600 <% translate("sec.(1 Hour)"); %>.<br>
<b>*- <% translate("Idle Timeout"); %>:</b> <% translate("Expired time where you can't access the device again.Default value"); %>=0.<br>
<b>*- <% translate("Max Missed ARP"); %>:</b> <% translate("Number of lost ARP before considering the client has leaved the connection. Default"); %> = 5<br>
<b>*- <% translate("Log Info Level"); %>:</b> <% translate("Messages from this module stored internally for better trace. Level"); %> 0=<% translate("Silent"); %>, 10=<% translate("Parrot"); %>, 2=<% translate("Default"); %>.<br>
<b>*- <% translate("Gateway Port"); %>:</b> <% translate("Port to be used by the Captive Portal for page redirection. Port 1 to 65534. Default"); %>=5280.<br>
<b>*- <% translate("Excluded/Included ports to be redirected"); %>:</b> <% translate("When setting any port (included or excluded) leave a blank space between each port number, i.e"); %>; 25 110 4662 4672. <% translate("Use prefereable one of the two option to avoid conflicts"); %>.<br>
<b>*- <% translate("URL Excluded off Captive Portal"); %>:</b> <% translate("URL that will be accessed without Welcome banner screen appearing. When you set allowed url's also leave a blank space between each url. i.e"); %>; http://www.google.com http://www.google.es<br>
<b>*- <% translate("MAC Address Whitelist"); %>:</b> <% translate("MAC addresses excluded of the feature. Leave a blank space between each MAC Address, i.e"); %>; 11:22:33:44:55:66 11:22:33:44:55:67<br>
<b>*- <% translate("Customized Splash File Path"); %>:</b> <% translate("Here you can upload your personal Welcome banner that will overwrite the default one"); %>.<br><br>
</span>
<br>
<span style='color:red'>
<b> <% translate("Note: If Login Time is expired you should re-enter again into the splash page to get a new lease period. Be aware, there is no notice about expired period so, you can loss Internet Access"); %>.</b><br>
</span>
<br>
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
