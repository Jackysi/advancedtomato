<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Basic: Identification</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("router_name,wan_hostname,wan_domain"); %>


function verifyFields(focused, quiet)
{
	if (!v_hostname('_wan_hostname', quiet)) return 0;
	return v_length('_router_name', quiet, 1) && v_length('_wan_hostname', quiet, 0) && v_length('_wan_domain', quiet, 0);
}

function save()
{
	if (!verifyFields(null, false)) return;
	form.submit('_fom', 1);
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

<input type='hidden' name='_nextpage' value='basic.asp'>
<input type='hidden' name='_service' value='*'>


<div class='section-title'>Router Identification</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Router Name', name: 'router_name', type: 'text', maxlen: 32, size: 34, value: nvram.router_name },
	{ title: 'Hostname', name: 'wan_hostname', type: 'text', maxlen: 63, size: 34, value: nvram.wan_hostname },
	{ title: 'Domain Name', name: 'wan_domain', type: 'text', maxlen: 32, size: 34, value: nvram.wan_domain }
]);
</script>
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
<script type='text/javascript'>verifyFields(null, true);</script>
</body>
</html>
