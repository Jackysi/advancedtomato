 HTML PUBLIC '-//W3C//DTD HTML 4.0//EN'>
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
<title>[<% ident(); %>] Set QOS Class Names</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("qos_classnames"); %>

var checker = null;
var timer = new TomatoTimer(check);
var running = 0;



function verifyFields(focused, quiet)
{
	return 1;
}


function save()
{
	var i, qos, fom;

	if (!verifyFields(null, false)) return;

	qos = [];
	for (i = 1; i < 11; ++i) {
		qos.push(E('_f_qos_' + (i - 1)).value);
	}

	fom = E('_fom');
	fom.qos_classnames.value = qos.join(' ');
	form.submit(fom, 1);
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

<input type='hidden' name='_nextpage' value='qos-classnames.asp'>
<input type='hidden' name='_service' value='qos-restart'>
<input type='hidden' name='qos_classnames' value=''>


<div class='section-title'>Set QOS Class Names</div>
<div class='section'>
<script type='text/javascript'>
if ((v = nvram.qos_classnames.match(/^(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)$/)) == null) {
	v = ["-","Highest","High","Medium","Low","Lowest","A","B","C","D","E"];
}
titles = ['-','Class 1', 'Class 2', 'Class 3', 'Class 4', 'Class 5', 'Class 6', 'Class 7', 'Class 8', 'Class 9', 'Class 10'];
f = [{ title: ' ', text: '<small>(seconds)</small>' }];
for (i = 1; i < 11; ++i) {
	f.push({ title: titles[i], name: ('f_qos_' + (i - 1)),
		type: 'text', maxlen: 9, size: 9, value: v[i],
		suffix: '<span id="count' + i + '"></span>' });
}
createFieldTable('', f);
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
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
