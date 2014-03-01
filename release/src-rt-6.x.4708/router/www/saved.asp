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
<title>[<% ident(); %>] Tomato</title>
<script language='javascript'>
wait = parseInt('<% cgi_get("_nextwait"); %>', 10);
if (isNaN(wait)) wait = 5;
function tick()
{
	clock.innerHTML = wait;
	opacity -= step;
	if (opacity < 0) opacity = 0;
	spin.style.opacity = opacity.toFixed(4);
	if (--wait >= 0) setTimeout(tick, 1000);
		else go();
}
function go()
{
	clock.style.visibility = 'hidden';
	window.location.replace('<% cgi_get("_nextpage"); %>');
}
function setSpin(x)
{
	document.getElementById('spin').style.visibility = x ? 'visible' : 'hidden';
	spun = x;
}
function init()
{
	if (wait > 0) {
		spin = document.getElementById('spin');
		opacity = 1;
		step = 1 / wait;
		clock = document.getElementById('xclock');
		clock.style.visibility = 'visible';
		tick();
		if (!spun) setSpin(0);	// http may be down after this page gets sent
	}
}
</script></head>
<body onload='init()' style='background-color:#fff' onclick='go()'>
<table style='width:100%;height:100%'>
<tr><td style='text-align:center;vertical-align:middle;font:16px sans-serif;width:100%;height:100%'>
<form>
<script type='text/javascript'>
if (wait <= 0) s = '<b>Changes Saved...</b> &nbsp; <input type="button" value="Continue" onclick="go()" style="font:10px sans-serif;vertical-align:top">';
	else s = '<b>Please Wait...</b><span id="xclock" style="font-size:9px;background:#f7f7f7;padding:1px;visibility:hidden">&nbsp;</span> <img src="spin.gif" id="spin" onload="setSpin(1)">';
document.write(s);
</script>
</form>
</td></tr>
</table>
</body></html>
