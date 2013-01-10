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
<title>[<% ident(); %>] QoS: View Graphs</title>
<link rel='stylesheet' type='text/css' href='tomato.css'>
<link rel='stylesheet' type='text/css' href='color.css'>
<script type='text/javascript' src='tomato.js'></script>

<!-- / / / -->
<script type='text/javascript' src='debug.js'></script>

<style type='text/css'>
.color {
	width: 12px;
	height: 25px;
}
.title {
}
.count {
	text-align: right;
}
.pct {
	width:55px;
	text-align: right;
}
.thead {
	font-size: 90%;
	font-weight: bold;
}
.total {
	border-top: 1px dashed #bbb;
	font-weight: bold;
}
</style>

<script type='text/javascript'>
// <% nvram("qos_classnames,web_svg,qos_enable,qos_obw,qos_ibw"); %>

//<% qrate(); %>

var svgReady = 0;

var Unclassified = ['Unclassified'];
var Unused = ['Unused'];
var classNames = nvram.qos_classnames.split(' ');		//Toastman Class Labels
var abc = Unclassified.concat(classNames,Unused);

var colors = [
	'c6e2ff',
	'b0c4de',
	'9ACD32',
	'3cb371',
	'6495ed',
	'8FBC8F',
	'a0522d',
	'deb887',
	'F08080',
	'ffa500',
	'ffd700',
	'D8D8D8'
];

var toggle=true;

function mClick(n)
{
	location.href = 'qos-detailed.asp?class=' + n;
}

function showData()
{
	var i, n, p;
	var totalConnections, totalOutgoingBandwidth, totalIncomingBandwidth;

	totalConnections = totalOutgoingBandwidth = totalIncomingBandwidth = 0;
	
	for (i = 0; i < 11; ++i)
	{
		if (!nfmarks[i]) nfmarks[i] = 0;
		totalConnections += nfmarks[i];
		if (!qrates_out[i]) qrates_out[i] = 0;
		totalOutgoingBandwidth += qrates_out[i];
		if (!qrates_in[i]) qrates_in[i] = 0;
		totalIncomingBandwidth += qrates_in[i];
	}

	for (i = 0; i < 11; ++i) {
		n = nfmarks[i];
		E('ccnt' + i).innerHTML = n;
		if (totalConnections > 0) p = (n / totalConnections) * 100;
			else p = 0;
		E('cpct' + i).innerHTML = p.toFixed(2) + '%';
	}
	E('ccnt-total').innerHTML = totalConnections;
		
	obwrate = nvram.qos_obw * 1000;
	ibwrate = nvram.qos_ibw * 1000;
	
	if(toggle == false)
	{
		totalorate = totalOutgoingBandwidth;
		totalirate = totalIncomingBandwidth;
		totalrateout = '100%';
		totalratein = '100%';
	} else 
	{
		FreeOutgoing = (obwrate - totalOutgoingBandwidth);
		qrates_out.push(FreeOutgoing);
		FreeIncoming = (ibwrate - totalIncomingBandwidth);
		qrates_in.push(FreeIncoming);
		totalorate = obwrate;
		totalirate = ibwrate;
		totalrateout = ((totalOutgoingBandwidth / totalorate) * 100).toFixed(2) + '%';
		totalratein = ((totalIncomingBandwidth / totalirate) * 100).toFixed(2) + '%';
	}

	for (i = 1; i < 11; ++i) {
		n = qrates_out[i];
		E('bocnt' + i).innerHTML = (n / 1000).toFixed(2)
		E('bocntx' + i).innerHTML = (n / 8192).toFixed(2)
		if (totalOutgoingBandwidth > 0) p = (n / totalorate) * 100;
			else p = 0;
		E('bopct' + i).innerHTML = p.toFixed(2) + '%';
	}
	E('bocnt-total').innerHTML = (totalOutgoingBandwidth / 1000).toFixed(2)
	E('bocntx-total').innerHTML = (totalOutgoingBandwidth / 8192).toFixed(2)
	E('rateout').innerHTML = totalrateout;

	for (i = 1; i < 11; ++i) {
		n = qrates_in[i];
		E('bicnt' + i).innerHTML = (n / 1000).toFixed(2)
		E('bicntx' + i).innerHTML = (n / 8192).toFixed(2)
		if (totalIncomingBandwidth > 0) p = (n / totalirate) * 100;
			else p = 0;
		E('bipct' + i).innerHTML = p.toFixed(2) + '%';
	}
	E('bicnt-total').innerHTML = (totalIncomingBandwidth / 1000).toFixed(2)
	E('bicntx-total').innerHTML = (totalIncomingBandwidth / 8192).toFixed(2)
	E('ratein').innerHTML = totalratein;
}

var ref = new TomatoRefresh('update.cgi', 'exec=qrate', 2, 'qos_graphs');

ref.refresh = function(text)
{
	nfmarks = [];
	qrates_out = [];
	qrates_in = [];
	
	try 
	{
		eval(text);
	}
	catch (ex)
	{
		nfmarks = [];
		qrates_out = [];
		qrates_in = [];
	}

	showData();
	if (svgReady == 1) 
	{
		updateConnectionDistribution(nfmarks, abc);
		updateBandwidthOutgoing(qrates_out, abc);
		updateBandwidthIncoming(qrates_in, abc);
	}
}

function checkSVG()
{
	var i, e, d, w;

	try
	{
		for (i = 2; i >= 0; --i) 
		{
			e = E('svg' + i);
			d = e.getSVGDocument();
			
			if (d.defaultView)
			{
				w = d.defaultView;
			}
			else
			{
				w = e.getWindow();
			}
			
			if (!w.ready) break;
			
			switch(i)
			{
				case 0:
				{
					updateConnectionDistribution = w.updateSVG;
					break;
				}
				
				case 1:
				{
					updateBandwidthOutgoing = w.updateSVG;
					break;
				}
				
				case 2:
				{
					updateBandwidthIncoming = w.updateSVG;
					break;
				}
			}
		}
	}
	catch (ex) 
	{
	}

	if (i < 0) 
	{
		svgReady = 1;
		updateConnectionDistribution(nfmarks, abc);
		updateBandwidthOutgoing(qrates_out, abc);
		updateBandwidthIncoming(qrates_in, abc);
	}
	else if (--svgReady > -5) 
	{
		setTimeout(checkSVG, 500);
	}
}

function showGraph()
{
	if(toggle == true)
	{
		toggle=false;
		qrates_out = qrates_out.slice(0, -1);	
		qrates_in = qrates_in.slice(0, -1);
		showData();
		checkSVG();
	} else 
	{
		toggle=true;
		showData();
		checkSVG();
	}
}

function init()
{
	nbase = fixInt(cookie.get('qnbase'), 0, 1, 0);
	showData();
	checkSVG();
	showGraph();
	ref.initPage(2000, 3);
}
</script>
</head>
<body onload='init()'>
<form id='_fom' action='javascript:{}'>
<table id='container' cellspacing=0>
<tr><td colspan=3 id='header'>
	<div class='title'>Tomato</div>
	<div class='version'>Version <% version(); %></div>
</td></tr>
<tr id='body'><td id='navi'><script type='text/javascript'>navi()</script></td>
<td id='content'colspan=2>
<div id='ident'><% ident(); %></div>

<!-- / / / -->

<div class="section-title">Connections Distribution</div>
<div class="section">
<table border=0 width="100%"><tr><td>
	<table style="width:250px">
<script type='text/javascript'>
for (i = 0; i < 11; ++i) {
	W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
		'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
		'<td class="title" style="width:60px"><a href="qos-detailed.asp?class=' + i + '">' + abc[i] + '</a></td>' +
		'<td id="ccnt' + i + '" class="count" style="width:90px"></td>' +
		'<td id="cpct' + i + '" class="pct"></td></tr>');
}
</script>
	<tr><td>&nbsp;</td><td class="total">Total</a></td><td id="ccnt-total" class="total count"></td><td class="total pct">100%</td></tr>
	</table>
</td><td style="margin-right:150px">
<script type='text/javascript'>
if (nvram.web_svg != '0') {
	W('<embed src="qos-graph.svg?n=0&v=<% version(); %>" style="width:310px;height:310px;margin:0;padding:0" id="svg0" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/"></embed>');
}
</script>
</td></tr>
</table>
</div>

<div class="section-title">Bandwidth Distribution (Outbound)</div>
<div class="section">
<table border=0 width="100%"><tr><td>
	<table style="width:250px">
	<tr><td class='color' style="height:1em"></td><td class='title' style="width:45px">&nbsp;</td><td class='thead count'>kbit/s</td><td class='thead count'>KB/s</td><td class='thead pct'>Rate</td></tr>
<script type='text/javascript'>
for (i = 1; i < 11; ++i) {
	W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
		'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
		'<td class="title" style="width:45px"><a href="qos-detailed.asp?class=' + i + '">' + abc[i] + '</a></td>' +
		'<td id="bocnt' + i + '" class="count" style="width:60px"></td>' +
		'<td id="bocntx' + i + '" class="count" style="width:50px"></td>' +
		'<td id="bopct' + i + '" class="pct"></td></tr>');
}
</script>
	<tr><td>&nbsp;</td><td class="total">Total</a></td><td id="bocnt-total" class="total count"></td><td id="bocntx-total" class="total count"></td><td id="rateout" class="total pct"></td></tr>
	</table>
</td><td style="margin-right:150px">
<script type='text/javascript'>
if (nvram.web_svg != '0') {
	W('<embed src="qos-graph.svg?n=1&v=<% version(); %>" style="width:310px;height:310px;margin:0;padding:0" id="svg1" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/"></embed>');
}
</script>
</td></tr>
</table>
</div>

<div class="section-title">Bandwidth Distribution (Inbound)</div>
<div class="section">
<table border=0 width="100%"><tr><td>
	<table style="width:250px">
	<tr><td class='color' style="height:1em"></td><td class='title' style="width:45px">&nbsp;</td><td class='thead count'>kbit/s</td><td class='thead count'>KB/s</td><td class='thead pct'>Rate</td></tr>
<script type='text/javascript'>
for (i = 1; i < 11; ++i) {
	W('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
		'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
		'<td class="title" style="width:45px"><a href="qos-detailed.asp?class=' + i + '">' + abc[i] + '</a></td>' +
		'<td id="bicnt' + i + '" class="count" style="width:60px"></td>' +
		'<td id="bicntx' + i + '" class="count" style="width:50px"></td>' +
		'<td id="bipct' + i + '" class="pct"></td></tr>');
}
</script>
	<tr><td>&nbsp;</td><td class="total">Total</a></td><td id="bicnt-total" class="total count"></td><td id="bicntx-total" class="total count"></td><td id="ratein" class="total pct"></td></tr>
	</table>
</td><td style="margin-right:150px">
<script type='text/javascript'>
if (nvram.web_svg != '0') {
	W('<embed src="qos-graph.svg?n=2&v=<% version(); %>" style="width:310px;height:310px;margin:0;padding:0" id="svg2" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/"></embed>');
}
</script>
</td></tr>
</table>
</div>

<script type='text/javascript'>
if (nvram.qos_enable != '1') {
	W('<div class="note-disabled"><b>QoS disabled.</b> &nbsp; <a href="qos-settings.asp">Enable &raquo;</a></div>');
}
</script>

<!-- / / / -->

</td></tr>
<tr><td id='footer'></td>
	<td id='footer' width="528"><input name="mybtn" style="width:100px" value="Zoom Graphs" type="button" onclick="showGraph()" ></td>
	<td id='footer' width="237"><script type='text/javascript'>genStdRefresh(1,2,'ref.toggle()');</script></td>
	</tr>
</table>
</form>
</body>
</html>
