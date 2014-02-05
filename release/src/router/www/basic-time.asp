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
<title>[<% ident(); %>] Basic: Time</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->

<script type='text/javascript' src='debug.js'></script>

<script type='text/javascript'>

//	<% nvram("tm_sel,tm_dst,tm_tz,ntp_updates,ntp_server,ntp_tdod,ntp_kiss"); %>


var ntpList = [
	['custom', 'Custom...'],
	['', 'Default'],
	['africa', 'Africa'],
	['asia', 'Asia'],
	['europe', 'Europe'],
	['oceania', 'Oceania'],
	['north-america', 'North America'],
	['south-america', 'South America'],
	['us', 'US']
];

function ntpString(name)
{
	if (name == '') name = 'pool.ntp.org';
		else name = name + '.pool.ntp.org';
	return '0.' + name + ' 1.' + name + ' 2.' + name;
}

function verifyFields(focused, quiet)
{
	var ok = 1;

	var s = E('_tm_sel').value;
	var f_dst = E('_f_tm_dst');
	var f_tz = E('_f_tm_tz');
	if (s == 'custom') {
		f_dst.disabled = true;
		f_tz.disabled = false;
		PR(f_dst).style.display = 'none';
		PR(f_tz).style.display = '';
	}
	else {
		f_tz.disabled = true;
		PR(f_tz).style.display = 'none';
		PR(f_dst).style.display = '';
		if (s.match(/^([A-Z]+[\d:-]+)[A-Z]+/)) {
			if (!f_dst.checked) s = RegExp.$1;
			f_dst.disabled = false;
		}
		else {
			f_dst.disabled = true;
		}
		f_tz.value = s;
	}

	var a = 1;
	var b = 1;
	switch (E('_ntp_updates').value * 1) {
	case -1:
		b = 0;
	case 0:
		a = 0;
		break;
	}
	elem.display(PR('_f_ntp_tdod'), a);

	elem.display(PR('_f_ntp_server'), b);
	a = (E('_f_ntp_server').value == 'custom');
	elem.display(PR('_f_ntp_1'), PR('_f_ntp_2'), PR('_f_ntp_3'), a && b);

	elem.display(PR('ntp-preset'), !a && b);

	if (a) {
		if ((E('_f_ntp_1').value == '') && (E('_f_ntp_2').value == '') && ((E('_f_ntp_3').value == ''))) {
			ferror.set('_f_ntp_1', 'At least one NTP server is required', quiet);
			return 0;
		}
	}
	else {
		E('ntp-preset').innerHTML = ntpString(E('_f_ntp_server').value).replace(/\s+/, ', ');
	}

	ferror.clear('_f_ntp_1');
	return 1;
}

function save(clearKiss)
{
	if (!verifyFields(null, 0)) return;

	var fom, a, i;

	fom = E('_fom');
	fom.tm_dst.value = fom.f_tm_dst.checked ? 1 : 0;
	fom.tm_tz.value = fom.f_tm_tz.value;

	if (E('_f_ntp_server').value != 'custom') {
		fom.ntp_server.value = ntpString(E('_f_ntp_server').value);
	}
	else {
		a = [fom.f_ntp_1.value, fom.f_ntp_2.value, fom.f_ntp_3.value];
		for (i = 0; i < a.length; ) {
			if (a[i] == '') a.splice(i, 1);
				else ++i;
		}
		fom.ntp_server.value = a.join(' ');
	}

	fom.ntp_tdod.value = fom.f_ntp_tdod.checked ? 1 : 0;
	fom.ntp_kiss.disabled = !clearKiss;
	form.submit(fom);
}

function earlyInit()
{
	if (nvram.ntp_kiss != '') {
		E('ntpkiss-ip').innerHTML = nvram.ntp_kiss;
		E('ntpkiss').style.display = '';
	}
	verifyFields(null, 1);
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

<input type='hidden' name='_nextpage' value='basic-time.asp'>
<input type='hidden' name='_nextwait' value='5'>
<input type='hidden' name='_service' value='ntpc-restart'>
<input type='hidden' name='_sleep' value='3'>

<input type='hidden' name='tm_dst'>
<input type='hidden' name='tm_tz'>
<input type='hidden' name='ntp_server'>
<input type='hidden' name='ntp_tdod'>
<input type='hidden' name='ntp_kiss' value='' disabled>


<div class='section-title'>Time</div>
<div class='section'>
<script type='text/javascript'>

ntp = nvram.ntp_server.split(/\s+/);

ntpSel = 'custom';
for (i = ntpList.length - 1; i > 0; --i) {
	if (ntpString(ntpList[i][0]) == nvram.ntp_server) ntpSel = ntpList[i][0];
}

/* REMOVE-BEGIN

http://tldp.org/HOWTO/TimePrecision-HOWTO/tz.html
http://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html

Canada
http://www3.sympatico.ca/c.walton/canada_dst.html
http://home-4.tiscali.nl/~t876506/Multizones.html#ca
http://www3.sympatico.ca/c.walton/canada_dst.html

Brazil
http://www.timeanddate.com/worldclock/clockchange.html?n=233
http://www.timeanddate.com/worldclock/city.html?n=479

Finland
http://www.timeanddate.com/worldclock/city.html?n=101

New Zeland
http://www.dia.govt.nz/diawebsite.nsf/wpg_URL/Services-Daylight-Saving-Index

Australia
http://en.wikipedia.org/wiki/Time_in_Australia
http://www.timeanddate.com/library/abbreviations/timezones/au/

REMOVE-END */

createFieldTable('', [
	{ title: 'Router Time', text: '<span id="clock"><% time(); %></span>' },
	null,
	{ title: 'Time Zone', name: 'tm_sel', type: 'select', options: [
		['custom','Custom...'],
		['UTC12','UTC-12:00 Kwajalein'],
		['UTC11','UTC-11:00 Midway Island, Samoa'],
		['UTC10','UTC-10:00 Hawaii'],
		['NAST9NADT,M3.2.0/2,M11.1.0/2','UTC-09:00 Alaska'],
		['PST8PDT,M3.2.0/2,M11.1.0/2','UTC-08:00 Pacific Time'],
		['UTC7','UTC-07:00 Arizona'],
		['MST7MDT,M3.2.0/2,M11.1.0/2','UTC-07:00 Mountain Time'],
		['UTC6','UTC-06:00 Mexico'],
		['CST6CDT,M3.2.0/2,M11.1.0/2','UTC-06:00 Central Time'],
		['UTC5','UTC-05:00 Colombia, Panama'],
		['EST5EDT,M3.2.0/2,M11.1.0/2','UTC-05:00 Eastern Time'],
		['VET4:30','UTC-04:30 Venezuela'],
		['UTC4','UTC-04:00 Aruba, Bermuda, Guyana, Puerto Rico'],
		['BOT4','UTC-04:00 Bolivia'],
		['AST4ADT,M3.2.0/2,M11.1.0/2','UTC-04:00 Atlantic Time'],
		['BRWST4BRWDT,M10.3.0/0,M2.5.0/0','UTC-04:00 Brazil West'],
		['NST3:30NDT,M3.2.0/0:01,M11.1.0/0:01','UTC-03:30 Newfoundland'],
		['WGST3WGDT,M3.5.6/22,M10.5.6/23','UTC-03:00 Greenland'],
		['BRST3BRDT,M10.3.0/0,M2.5.0/0','UTC-03:00 Brazil East'],
		['UTC3','UTC-03:00 Argentina, French Guiana, Surinam'],
		['UTC2','UTC-02:00 Mid-Atlantic'],
		['STD1DST,M3.5.0/2,M10.5.0/2','UTC-01:00 Azores'],
		['UTC0','UTC+00:00 Gambia, Liberia, Morocco'],
		['GMT0BST,M3.5.0/2,M10.5.0/2','UTC+00:00 England'],
		['UTC-1','UTC+01:00 Tunisia'],
		['CET-1CEST,M3.5.0/2,M10.5.0/3','UTC+01:00 France, Germany, Italy, Poland, Spain, Sweden'],
		['EET-2EEST-3,M3.5.0/3,M10.5.0/4','UTC+02:00 Estonia, Finland, Latvia, Lithuania'],
		['UTC-2','UTC+02:00 South Africa, Israel'],
		['STD-2DST,M3.5.0/2,M10.5.0/2','UTC+02:00 Greece, Ukraine, Romania, Turkey, Latvia'],
		['UTC-3','UTC+03:00 Iraq, Jordan, Kuwait'],
		['MSK-3MSD,M3.5.0,M10.5.0/3','UTC+03:00 Moscow'],
		['UTC-4','UTC+04:00 Oman, UAE'],
		['AMT-4AMST,M3.5.0,M10.5.0/3','UTC+04:00 Armenia'],
		['UTC-4:30','UTC+04:30 Kabul'],
		['UTC-5','UTC+05:00 Pakistan'],
		['YEKT-5YEKST,M3.5.0,M10.5.0/3','UTC+05:00 Russia, Yekaterinburg'],
		['UTC-5:30','UTC+05:30 Bombay, Calcutta, Madras, New Delhi'],
		['UTC-6','UTC+06:00 Bangladesh'],
		['NOVT-6NOVST,M3.5.0,M10.5.0/3','UTC+06:00 Russia, Novosibirsk'],
		['UTC-7','UTC+07:00 Thailand'],
		['KRAT-7KRAST,M3.5.0,M10.5.0/3','UTC+07:00 Russia, Krasnoyarsk'],
		['UTC-8','UTC+08:00 China, Hong Kong, Western Australia, Singapore, Taiwan'],
		['IRKT-8IRKST,M3.5.0,M10.5.0/3','UTC+08:00 Russia, Irkutsk'],
		['UTC-9','UTC+09:00 Japan, Korea'],
		['YAKT-9YAKST,M3.5.0,M10.5.0/3','UTC+09:00 Russia, Yakutsk'],
		['ACST-9:30ACDT,M10.1.0/2,M4.1.0/3', 'UTC+09:30 South Australia'],
		['ACST-9:30', 'UTC+09:30 Darwin'],
		['UTC-10','UTC+10:00 Guam, Russia'],
		['AEST-10AEDT,M10.1.0,M4.1.0/3', 'UTC+10:00 Australia'],
		['AEST-10', 'UTC+10:00 Brisbane'],
		['UTC-11','UTC+11:00 Solomon Islands'],
		['UTC-12','UTC+12:00 Fiji'],
		['NZST-12NZDT,M9.5.0/2,M4.1.0/3','UTC+12:00 New Zealand']
	], value: nvram.tm_sel },
	{ title: 'Auto Daylight Savings Time', indent: 2, name: 'f_tm_dst', type: 'checkbox', value: nvram.tm_dst != '0' },
	{ title: 'Custom TZ String', indent: 2, name: 'f_tm_tz', type: 'text', maxlen: 32, size: 34, value: nvram.tm_tz || '' },
	null,
	{ title: 'Auto Update Time', name: 'ntp_updates', type: 'select', options: [[-1,'Never'],[0,'Only at startup'],[1,'Every hour'],[2,'Every 2 hours'],[4,'Every 4 hours'],[6,'Every 6 hours'],[8,'Every 8 hours'],[12,'Every 12 hours'],[24,'Every 24 hours']],
		value: nvram.ntp_updates },
	{ title: 'Trigger Connect On Demand', indent: 2, name: 'f_ntp_tdod', type: 'checkbox', value: nvram.ntp_tdod != '0' },
	{ title: 'NTP Time Server', name: 'f_ntp_server', type: 'select', options: ntpList, value: ntpSel },
	{ title: '&nbsp;', text: '<small><span id="ntp-preset">xx</span></small>', hidden: 1 },
	{ title: '', name: 'f_ntp_1', type: 'text', maxlen: 48, size: 50, value: ntp[0] || 'pool.ntp.org', hidden: 1 },
	{ title: '', name: 'f_ntp_2', type: 'text', maxlen: 48, size: 50, value: ntp[1] || '', hidden: 1 },
	{ title: '', name: 'f_ntp_3', type: 'text', maxlen: 48, size: 50, value: ntp[2] || '', hidden: 1 }
]);
</script>
</div>
<br><br>

<div id='ntpkiss' style='display:none'>
The following NTP servers have been automatically blocked by request from the server:
<b id='ntpkiss-ip'></b>
<div>
	<input type='button' value='Clear' onclick='save(1)'>
</div>
</div>

<!-- / / / -->

</td></tr>
<tr><td id='footer' colspan=2>
	<span id='footer-msg'></span>
	<input type='button' value='Save' id='save-button' onclick='save(0)'>
	<input type='button' value='Cancel' id='cancel-button' onclick='reloadPage();'>
</td></tr>
</table>
</form>
<script type='text/javascript'>earlyInit()</script>
</body>
</html>
