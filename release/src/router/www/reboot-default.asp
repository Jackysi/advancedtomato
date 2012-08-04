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
<title>[<% ident(); %>] Restoring Defaults...</title>
<script type='text/javascript'>
var n = 90;
function tick()
{
	var e = document.getElementById('continue');
	e.value = n--;
	if (n < 0) {
		e.value = 'Continue';
		return;
	}
	if (n == 19) e.disabled = false;
	setTimeout(tick, 1000);
}
function go()
{
	window.location = 'http://192.168.1.1/';
}
function init()
{
	tick()
}
</script></head>
<body style='background:#fff' onload='init()'><table style='width:100%;height:100%'>
<tr><td style='text-align:center;vertical-align:middle;font:12px sans-serif'><form>
Please wait while the defaults are restored... &nbsp;
<input type='button' value='' style='font:12px sans-serif;width:80px;height:24px' id='continue' onclick='go()' disabled>
<div style='width:600px;border-top:1px dashed #888;margin:5px auto;padding:5px 0;font-size:14px;' id='msg'>The router will reset its address back to 192.168.1.1. You may need to renew your computer's DHCP or reboot your computer before continuing.</div>
</form></td></tr>
</table></body></html>
