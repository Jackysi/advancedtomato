<html>
<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
-->
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
</head>
<body style='background-color:#fff;font:13px sans-serif;color:#000' onload='setTimeout("go.submit()", 1200)'>

<div style='width:300px;padding:50px;background:#eee'>
<b>Logout</b><br>
<hr size=1><br>
To clear the credentials cached by the browser:<br>
<br>
<b>Firefox, Internet Explorer, Opera, Safari</b><br>
- Leave the password field blank.<br>
- Click OK/Login<br>
<br>
<b>Chrome</b><br>
- Select Cancel.<br>
</div>

<form name='go' method='post' action='logout'>
<input type='hidden' name='_http_id' value='<% nv(http_id); %>'>
</form>
</body></html>
