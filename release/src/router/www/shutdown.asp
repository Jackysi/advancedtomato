<!DOCTYPE html>
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
-->
<html lang="en">
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<meta name="viewport" content="width=820">

		<title>[<% ident(); %>] Shutting down...</title>
		<script type="text/javascript">
			var n = 16;
			function tick()
			{
				if (--n > 0) {
					document.getElementById('sptime').innerHTML = n;
					setTimeout(tick, 1000);
				}
				else {
					document.getElementById('msg').innerHTML = 'You can now unplug the router.';
				}
			}
		</script>
	</head>
	<body style="background:#fff"><table style="width:100%;height:100%">
			<tr><td style="text-align:center;vertical-align:middle;font:15px sans-serif">
					<span id="msg">Please wait while the router shuts down... <span id="sptime" style="font-size:80%;background:#eee"></span></span>
				</td></tr>
		</table>
		<script type="text/javascript">tick();</script>
	</body>
</html>