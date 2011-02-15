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
<title>[<% ident(); %>] Advanced: Conntrack / Netfilter</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("ct_tcp_timeout,ct_udp_timeout,ct_timeout,ct_max,ct_hashsize,nf_l7in,nf_ttl,nf_sip,nf_rtsp,nf_pptp,nf_h323,nf_ftp"); %>

var checker = null;
var timer = new TomatoTimer(check);
var running = 0;

function check()
{
	timer.stop();
	if ((checker) || (!running)) return;

	checker = new XmlHttp();
	checker.onCompleted = function(text, xml) {
		var conntrack, total, i;
		conntrack = null;
		total = 0;
		try {
			eval(text);
		}
		catch (ex) {
			conntrack = [];
		}
		for (i = 1; i < 13; ++i) {
			E('count' + i).innerHTML = '&nbsp; <small>('+ ((conntrack[i] || 0) * 1) + ' in this state)</small>';
		}
		E('count0').innerHTML = '(' + ((conntrack[0] || 0) * 1) + ' connections currently tracked)';
		checker = null;
		timer.start(3000);
	}
	checker.onError = function(x) {
		checker = null;
		timer.start(6000);
	}

	checker.post('update.cgi', 'exec=ctcount&arg0=0');
}

function clicked()
{
	running ^= 1;
	E('spin').style.visibility = running ? 'visible' : 'hidden';
	if (running) check();
}


var expireText;

function expireTimer()
{
	var e = E('expire');

	if (!expireText) expireText = e.value;

	if (--expireTime == 0) {
		e.disabled = false;
		e.value = expireText;
	}
	else {
		setTimeout(expireTimer, 1000);
		e.value = 'Expire Scheduled... ' + expireTime;
	}
}

function expireClicked()
{
	expireTime = 18;
	E('expire').disabled = true;
	(new XmlHttp()).post('expct.cgi', '');
	expireTimer();
}


function verifyFields(focused, quiet)
{
	var i, v;

	for (i = 1; i < 9; ++i) {
		if (!v_range('_f_tcp_' + i, quiet, 1, 432000)) return 0;
	}
	for (i = 0; i < 2; ++i) {
		if (!v_range('_f_udp_' + i, quiet, 1, 432000)) return 0;
	}
	for (i = 0; i < 2; ++i) {
		if (!v_range('_f_ct_' + i, quiet, 1, 432000)) return 0;
	}

	if (!v_range('_ct_max', quiet, 128, 300000)) return 0;

/* LINUX26-BEGIN */
	if (!v_range('_ct_hashsize', quiet, 127, 65535)) return 0;
/* LINUX26-END */

	v = (E('_f_nf_ttl').value == '');
	E('_f_ttl_val').style.display = v ? '' : 'none';
	if ((v) && !v_range('_f_ttl_val', quiet, 0, 255)) return 0;

	return 1;
}

function save()
{
	var i, tcp, udp, ct, fom;

	if (!verifyFields(null, false)) return;

	tcp = [];
	tcp.push('0');
	for (i = 1; i < 9; ++i) {
		tcp.push(E('_f_tcp_' + i).value);
	}
	tcp.push('0');

	udp = [];
	for (i = 0; i < 2; ++i) {
		udp.push(E('_f_udp_' + i).value);
	}

	ct = [];
	for (i = 0; i < 2; ++i) {
		ct.push(E('_f_ct_' + i).value);
	}

	fom = E('_fom');
	fom.ct_tcp_timeout.value = tcp.join(' ');
	fom.ct_udp_timeout.value = udp.join(' ');
	fom.ct_timeout.value = ct.join(' ');
	fom.nf_l7in.value = E('_f_l7in').checked ? 1 : 0;
/* LINUX26-BEGIN */
	fom.nf_sip.value = E('_f_sip').checked ? 1 : 0;
/* LINUX26-END */
	fom.nf_rtsp.value = E('_f_rtsp').checked ? 1 : 0;
	fom.nf_pptp.value = E('_f_pptp').checked ? 1 : 0;
	fom.nf_h323.value = E('_f_h323').checked ? 1 : 0;
	fom.nf_ftp.value = E('_f_ftp').checked ? 1 : 0;

	i = E('_f_nf_ttl').value;
	if (i == '')
		fom.nf_ttl.value = 'c:' + E('_f_ttl_val').value;
	else
		fom.nf_ttl.value = i;

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

<input type='hidden' name='_nextpage' value='advanced-ctnf.asp'>
<input type='hidden' name='_service' value='ctnf-restart'>

<input type='hidden' name='ct_tcp_timeout' value=''>
<input type='hidden' name='ct_udp_timeout' value=''>
<input type='hidden' name='ct_timeout' value=''>
<input type='hidden' name='nf_l7in' value=''>
<input type='hidden' name='nf_ttl'>
<input type='hidden' name='nf_rtsp'>
<input type='hidden' name='nf_pptp'>
<input type='hidden' name='nf_h323'>
<input type='hidden' name='nf_ftp'>
/* LINUX26-BEGIN */
<input type='hidden' name='nf_sip'>
/* LINUX26-END */

<div class='section-title'>Connections</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'Maximum Connections', name: 'ct_max', type: 'text', maxlen: 6, size: 8,
		suffix: '&nbsp; <a href="javascript:clicked()" id="count0">[ count current... ]</a> <img src="spin.gif" style="vertical-align:bottom;padding-left:10px;visibility:hidden" id="spin" onclick="clicked()">',
		value: fixInt(nvram.ct_max || 4096, 128, 300000, 4096) }
/* LINUX26-BEGIN */
	,{ title: 'Hash Table Size', name: 'ct_hashsize', type: 'text', maxlen: 6, size: 8, value: nvram.ct_hashsize || 1023 }
/* LINUX26-END */
]);
</script>
<br>
<input type='button' value='Drop Idle' onclick='expireClicked()' id='expire'>
<br><br>
</div>


<div class='section-title'>TCP Timeout</div>
<div class='section'>
<script type='text/javascript'>
if ((v = nvram.ct_tcp_timeout.match(/^(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)$/)) == null) {
	v = [0,0,1200,120,60,120,120,10,60,30,0];
}
titles = ['-', 'None', 'Established', 'SYN Sent', 'SYN Received', 'FIN Wait', 'Time Wait', 'Close', 'Close Wait', 'Last ACK', 'Listen'];
f = [{ title: ' ', text: '<small>(seconds)</small>' }];
for (i = 1; i < 11; ++i) {
	f.push({ title: titles[i], name: ('f_tcp_' + (i - 1)),
		type: 'text', maxlen: 6, size: 8, value: v[i],
		hidden: (i == 1 || i == 10) ? 1 : 0,
		suffix: '<span id="count' + i + '"></span>' });
}
createFieldTable('', f);
</script>
</div>

<div class='section-title'>UDP Timeout</div>
<div class='section'>
<script type='text/javascript'>
if ((v = nvram.ct_udp_timeout.match(/^(\d+)\s+(\d+)$/)) == null) {
	v = [0,30,180];
}
createFieldTable('', [
	{ title: ' ', text: '<small>(seconds)</small>' },
	{ title: 'Unreplied', name: 'f_udp_0', type: 'text', maxlen: 6, size: 8, value: v[1], suffix: '<span id="count11"></span>' },
	{ title: 'Assured', name: 'f_udp_1', type: 'text', maxlen: 6, size: 8, value: v[2], suffix: '<span id="count12"></span>' }
]);
</script>
</div>

<div class='section-title'>Other Timeouts</div>
<div class='section'>
<script type='text/javascript'>
if ((v = nvram.ct_timeout.match(/^(\d+)\s+(\d+)$/)) == null) {
	v = [0,600,30];
}
createFieldTable('', [
	{ title: ' ', text: '<small>(seconds)</small>' },
	{ title: 'Generic', name: 'f_ct_0', type: 'text', maxlen: 6, size: 8, value: v[1] },
	{ title: 'ICMP', name: 'f_ct_1', type: 'text', maxlen: 6, size: 8, value: v[2] }
]);
</script>
</div>

<div class='section-title'>Tracking / NAT Helpers</div>
<div class='section'>
<script type='text/javascript'>
createFieldTable('', [
	{ title: 'FTP', name: 'f_ftp', type: 'checkbox', value: nvram.nf_ftp != '0' },
	{ title: 'GRE / PPTP', name: 'f_pptp', type: 'checkbox', value: nvram.nf_pptp != '0' },
	{ title: 'H.323', name: 'f_h323', type: 'checkbox', value: nvram.nf_h323 != '0' },
/* LINUX26-BEGIN */
	{ title: 'SIP', name: 'f_sip', type: 'checkbox', value: nvram.nf_sip != '0' },
/* LINUX26-END */
	{ title: 'RTSP', name: 'f_rtsp', type: 'checkbox', value: nvram.nf_rtsp != '0' }
]);
</script>
</div>

<div class='section-title'>Miscellaneous</div>
<div class='section'>
<script type='text/javascript'>
v = [];
for (i = -5; i <= 5; ++i) {
	v.push([i + '', i ? ((i > 0) ? '+' : '') + i : 'None']);
}
v.push(['', 'Custom']);

createFieldTable('', [
	{ title: 'TTL Adjust', multi: [
		{ name: 'f_nf_ttl', type: 'select', options: v, value: nvram.nf_ttl.substr(0, 2) == 'c:' ? '' : nvram.nf_ttl },
		{ name: 'f_ttl_val', type: 'text', maxlen: 3, size: 6, value: nvram.nf_ttl.substr(0, 2) == 'c:' ?  nvram.nf_ttl.substr(2, 5) : '' }
	] },
	{ title: 'Inbound Layer 7', name: 'f_l7in', type: 'checkbox', value: nvram.nf_l7in != '0' }
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
<script type='text/javascript'>verifyFields(null, 1);</script>
</body>
</html>
