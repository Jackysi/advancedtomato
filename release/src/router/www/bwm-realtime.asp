<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Real-Time Bandwidth</title>
<content>
	<style type="text/css">
		table tr td:nth-child(even) { width: 25%; }
		table tr td:nth-child(odd) { width: 5%; }
	</style>
	<script type="text/javascript" src="js/wireless.jsx?_http_id=<% nv(http_id); %>"></script>
	<script type="text/javascript" src="js/bwm-common.js"></script>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,wan_ifname,lan_ifname,wl_ifname,wan_proto,wan_iface,web_svg,rstats_colors"); %>

		var cprefix = 'bw_r';
		var updateInt = 2;
		var updateDiv = updateInt;
		var updateMaxL = 150;
		var updateReTotal = 1;
		var prev = [];
		var debugTime = 0;
		var avgMode = 0;
		var wdog = null;
		var wdogWarn = null;


		var ref = new TomatoRefresh('update.cgi', 'exec=netdev', 1);

		ref.stop = function() {
			this.timer.start(1000);
		}

		ref.refresh = function(text) {
			var c, i, h, n, j, k;

			watchdogReset();

			++updating;
			try {
				netdev = null;
				eval(text);

				n = (new Date()).getTime();
				if (this.timeExpect) {
					if (debugTime) $('#dtime').show().html((this.timeExpect - n) + ' ' + ((this.timeExpect + 2000) - n));
					this.timeExpect += 2000;
					this.refreshTime = MAX(this.timeExpect - n, 500);
				}
				else {
					this.timeExpect = n + 2000;
				}

				for (i in netdev) {
					c = netdev[i];
					if ((p = prev[i]) != null) {
						h = speed_history[i];

						h.rx.splice(0, 1);
						h.rx.push((c.rx < p.rx) ? (c.rx + (0xFFFFFFFF - p.rx)) : (c.rx - p.rx));

						h.tx.splice(0, 1);
						h.tx.push((c.tx < p.tx) ? (c.tx + (0xFFFFFFFF - p.tx)) : (c.tx - p.tx));
					}
					else if (!speed_history[i]) {
						speed_history[i] = {};
						h = speed_history[i];
						h.rx = [];
						h.tx = [];
						for (j = 300; j > 0; --j) {
							h.rx.push(0);
							h.tx.push(0);
						}
						h.count = 0;
					}
					prev[i] = c;
				}
				loadData();
			}
			catch (ex) {
			}
			--updating;
		}

		function watchdog()
		{
			watchdogReset();
			ref.stop();
			wdogWarn.style.display = '';
		}

		function watchdogReset() {
			if (wdog) clearTimeout(wdog)
			wdog = setTimeout(watchdog, 5000*updateInt);
			wdogWarn.style.display = 'none';
		}

		function init()
		{
			speed_history = [];

			initCommon(2, 1, 1);

			wdogWarn = E('warnwd');
			watchdogReset();

			$('.updatetime').html(5*updateInt);
			$('.interval').html(updateInt);

			ref.start();
		}
	</script>

	<ul class="nav-tabs">
		<li><a class="active"><i class="icon-hourglass"></i> Real-Time</a></li>
		<li><a class="ajaxload" href="bwm-24.asp"><i class="icon-graphs"></i> Last 24 Hours</a></li>
		<li><a class="ajaxload" href="bwm-daily.asp"><i class="icon-clock"></i> Daily</a></li>
		<li><a class="ajaxload" href="bwm-weekly.asp"><i class="icon-week"></i> Weekly</a></li>
		<li><a class="ajaxload" href="bwm-monthly.asp"><i class="icon-month"></i> Monthly</a></li>
	</ul>

	<div class="box">
		<div class="heading">Real Time Bandwidth &nbsp;  <div class="spinner" id="refresh-spinner" onclick="javascript:debugTime=1"></div></div>
		<div class="content">
			<div id="rstats">
				<div id="tab-area" class="btn-toolbar"></div>

				<script type="text/javascript">
					if (nvram.web_svg != '0') {
						$('#tab-area').after('<embed id="graph" type="image/svg+xml" src="img/bwm-graph.svg?<% version(); %>" style="height: 300px; width:100%;"></embed>');
					}
				</script>

				<div id="bwm-controls">
					<small>(<span class="updatetime"></span> minute window, <span class="interval"></span> second interval)</small> -
					<b>Avg</b>:
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
					<small><a href="javascript:switchColor(1)" id="drawrev">[reverse]</a></small>
					| <a class="ajaxload" href="admin-bwm.asp"><b>Configure</b></a>
				</div><br />

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
						<td><b style="border-bottom:blue 1px solid" id="tx-name">TX</b> <i class="icon-arrow-up"></i></td>
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
			</div>

			<span id="dtime" style="display:none;"></span>
			<div id="warnwd" class="alert warning" style="display:none">Warning: 10 second session timeout, restarting...&nbsp;</div>
		</div>
	</div>
	<script type="text/javascript">init();</script>
</content>