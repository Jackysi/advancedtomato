<!DOCTYPE html>
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Bandwidth: Last 24 Hours</title>
<content>
	<script type="text/javascript" src="js/wireless.jsx?_http_id=<% nv(http_id); %>"></script>
	<script type="text/javascript" src="js/bwm-common.js"></script>
	<script type="text/javascript">

		//	<% nvram("wan_ifname,lan_ifname,wl_ifname,wan_proto,wan_iface,web_svg,rstats_enable,rstats_colors"); %>

		var cprefix = 'bw_24';
		var updateInt = 120;
		var updateDiv = updateInt;
		var updateMaxL = 720;
		var updateReTotal = 1;
		var hours = 24;
		var lastHours = 0;
		var debugTime = 0;

		function showHours()
		{
			if (hours == lastHours) return;
			showSelectedOption('hr', lastHours, hours);
			lastHours = hours;
		}

		function switchHours(h)
		{
			if ((!svgReady) || (updating)) return;

			hours = h;
			updateMaxL = (720 / 24) * hours;
			showHours();
			loadData();
			cookie.set(cprefix + 'hrs', hours);
		}

		var ref = new TomatoRefresh('update.cgi', 'exec=bandwidth&arg0=speed');

		ref.refresh = function(text)
		{
			++updating;
			try {
				this.refreshTime = 1500;
				speed_history = {};
				try {
					eval(text);
					if (rstats_busy) {
						E('rbusy').style.display = 'none';
						rstats_busy = 0;
					}
					this.refreshTime = (fixInt(speed_history._next, 1, 120, 60) + 2) * 1000;
				}
				catch (ex) {
					speed_history = {};
				}
				if (debugTime) E('dtime').innerHTML = (new Date()) + ' ' + (this.refreshTime / 1000);
				loadData();
			}
			catch (ex) {
				//		alert('ex=' + ex);
			}
			--updating;
		}

		ref.showState = function()
		{
			$('#refresh-but').html('<i class="icon-' + (this.running ? 'stop' : 'reboot') + '"></i>');
		}

		ref.toggleX = function()
		{
			this.toggle();
			this.showState();
			cookie.set(cprefix + 'refresh', this.running ? 1 : 0);
		}

		ref.initX = function()
		{
			var a;

			a = fixInt(cookie.get(cprefix + 'refresh'), 0, 1, 1);
			if (a) {
				ref.refreshTime = 100;
				ref.toggleX();
			}
		}

		function init()
		{
			if (nvram.rstats_enable != '1') { $('#rstats').before('<div class="alert">Bandwidth monitoring disabled.</b> <a href="/#admin-bwm.asp">Enable &raquo;</a></div>'); return; }

			try {
				//<% bandwidth("speed"); %>
			}
			catch (ex) {
				speed_history = {};
			}
			rstats_busy = 0;
			if (typeof(speed_history) == 'undefined') {
				speed_history = {};
				rstats_busy = 1;
				E('rbusy').style.display = '';
			}

			hours = fixInt(cookie.get(cprefix + 'hrs'), 1, 24, 24);
			updateMaxL = (720 / 24) * hours;
			showHours();

			initCommon(1, 0, 0);
			ref.initX();
		}
	</script>

	<ul class="nav-tabs">
		<li><a class="ajaxload" href="bwm-realtime.asp"><i class="icon-hourglass"></i> Real-Time</a></li>
		<li><a class="active"><i class="icon-graphs"></i> Last 24 Hours</a></li>
		<li><a class="ajaxload" href="bwm-daily.asp"><i class="icon-clock"></i> Daily</a></li>
		<li><a class="ajaxload" href="bwm-weekly.asp"><i class="icon-week"></i> Weekly</a></li>
		<li><a class="ajaxload" href="bwm-monthly.asp"><i class="icon-month"></i> Monthly</a></li>
	</ul>

	<div id="rstats" class="box">
		<div class="heading">
			24h Bandwidth History &nbsp; <div class="spinner" id="refresh-spinner" style="visibility:hidden;" onclick="debugTime=1"></div>
			<a href="#" data-toggle="tooltip" onclick="ref.toggleX(); return false;" title="Auto refresh graphs" class="pull-right" id="refresh-but"><i class="icon-reboot"></i></a>
		</div>
		<div class="content">
			<div id="tab-area" class="btn-toolbar"></div>

			<script type="text/javascript">
				if (nvram.web_svg != '0') {
					$('#tab-area').after('<embed id="graph" type="image/svg+xml" src="img/bwm-graph.svg?<% version(); %>" style="height: 300px; width:100%;"></embed>');
				}
			</script>

			<div id="bwm-controls">
				<small>(2 minute interval)</small> -
				<b>Hours</b>:
				<a href="javascript:switchHours(1);" id="hr1">1</a>,
				<a href="javascript:switchHours(2);" id="hr2">2</a>,
				<a href="javascript:switchHours(4);" id="hr4">4</a>,
				<a href="javascript:switchHours(6);" id="hr6">6</a>,
				<a href="javascript:switchHours(12);" id="hr12">12</a>,
				<a href="javascript:switchHours(18);" id="hr18">18</a>,
				<a href="javascript:switchHours(24);" id="hr24">24</a>
				| <b>Avg</b>:
				<a href="javascript:switchAvg(1)" id="avg1">Off</a>,
				<a href="javascript:switchAvg(2)" id="avg2">2x</a>,
				<a href="javascript:switchAvg(4)" id="avg4">4x</a>,
				<a href="javascript:switchAvg(6)" id="avg6">6x</a>,
				<a href="javascript:switchAvg(8)" id="avg8">8x</a>
				| <b>Max</b>:
				<a href="javascript:switchScale(0)" id="scale0">Uniform</a> or
				<a href="javascript:switchScale(1)" id="scale1">Per IF</a>
				| <b>Display</b>:
				<a href="javascript:switchDraw(0)" id="draw0">Solid</a> or
				<a href="javascript:switchDraw(1)" id="draw1">Line</a>
				| <b>Color</b>: <a href="javascript:switchColor()" id="drawcolor">-</a>
				<small><a href="javascript:switchColor(1)" id="drawrev">[reverse]</a></small> |
				<a class="ajaxload" href="admin-bwm.asp"><b>Configure</b></a>
			</div>

			<table id="txt" class="data-table bwm-info">
				<tr>
					<td><b style="border-bottom:blue 1px solid" id="rx-name">RX</b> <i class="icon-arrow-down"></i></td>
					<td><span id="rx-current"></span></td>
					<td><b>Avg</b></td>
					<td id="rx-avg"></td>
					<td><b>Peak</b></td>
					<td id="rx-max"></td>
					<td><b>Total</b></td>
					<td id="rx-total"></td>
					<td>&nbsp;</td>
				</tr>
				<tr>
					<td><b style="border-bottom:blue 1px solid" id="tx-name">TX</b>
						<i class="icon-arrow-up"></i></td>
					<td><span id="tx-current"></span></td>
					<td><b>Avg</b></td>
					<td id="tx-avg"></td>
					<td><b>Peak</b></td>
					<td id="tx-max"></td>
					<td><b>Total</b></td>
					<td id="tx-total"></td>
					<td>&nbsp;</td>
				</tr>
			</table>

			<div id="rbusy" class="alert warning" style="display:none">Warning: 10 second session timeout, restarting...&nbsp;</div>

		</div>
	</div>

	<script type="text/javascript">init();</script>
</content>