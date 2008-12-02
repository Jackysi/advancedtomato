<html>
<!--
	Tomato GUI
	Copyright (C) 2006-2008 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<script type='text/javascript'>
function init()
{
	window.location.replace('/logout.cgi?_http_id=<% nv(http_id); %>');
}
</script>
</head>
<body style='background-color:#fff;font:13px sans-serif;color:#000' onload='setTimeout("init()", 500)'>
<div style='width:350px;padding:10em 1em;background:#eee;text-align:center'>
<b>Logout</b><br>Select cancel when prompted for a username/password.
</div>
</body></html>
