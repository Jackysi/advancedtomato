<html>
<head>
<meta http-equiv='content-type' content='text/html;charset=utf-8'>
<meta name='robots' content='noindex,nofollow'>
<title>[<% ident(); %>] Rebooting...</title>
<script type='text/javascript'>
var n = 90 + parseInt('0<% nv("wait_time"); %>');
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
	window.location.replace('/');
}
function init()
{
	resmsg = '';
	//<% resmsg(); %>
	if (resmsg.length) {
		e = document.getElementById('msg');
		e.innerHTML = resmsg;
		e.style.display = '';
	}
	tick()
}
</script></head>
<body style='background:#fff' onload='init()'><table style='width:100%;height:100%'>
<tr><td style='text-align:center;vertical-align:middle;font:12px sans-serif'><form>
<div style='width:600px;border-bottom:1px solid #aaa;margin:5px auto;padding:5px 0;font-size:14px;display:none;font-weight:bold' id='msg'></div>
Please wait while the router reboots... &nbsp;
<input type='button' value='' style='font:12px sans-serif;width:80px;height:24px' id='continue' onclick='go()' disabled>
</form></td></tr>
</table></body></html>

