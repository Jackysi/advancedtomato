<html lang="en">
	<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
	-->
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<meta name="viewport" content="width=820">
	</head>
	
	<body style="background-color:#f9f9f9;font:13px sans-serif;color: #3C3C3C" onload="setTimeout('go.submit()', 1200)">

		<div style="width:300px; margin: 10% auto; padding: 5px 25px 25px 25px; background:#fff; border: 1px solid #C3C3C3; text-align: center; border-radius: 5px;">
			<h3 style="text-align: left; color: #353535; border-bottom: 1px solid #E5E5E5; padding-bottom: 4px;">Logout</h3>
			<div style="text-align: left;">
				To clear the credentials cached by the browser:<br>
				<br>
				<b>Firefox, Internet Explorer, Opera, Safari</b><br>
				- Leave the password field blank.<br>
				- Click OK/Login<br>
				<br>
				<b>Chrome</b><br>
				- Select Cancel.
			</div>
		</div>

		<form name="go" method="post" action="logout">
			<input type="hidden" name="_http_id" value="<% nv(http_id); %>">
		</form>
		
	</body>
</html>