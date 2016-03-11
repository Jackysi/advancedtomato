<!DOCTYPE html>
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Bandwidth: Daily</title>
<content>
	<style>
		#last-dn,#last-up,#last-total { text-align: right; }
	</style>
	<script type="text/javascript" src="js/bwm-hist.js"></script>
	<script type="text/javascript">
		//<% nvram("wan_ifname,wan2_ifname,wan3_ifname,wan4_ifname,lan_ifname,rstats_enable"); %>

		try {
			//	<% bandwidth("daily"); %>
		}
		catch (ex) {
			daily_history = [];
		}
		rstats_busy = 0;
		if (typeof(daily_history) == 'undefined') {
			daily_history = [];
			rstats_busy = 1;
		}

		function save()
		{
			cookie.set('daily', scale, 31);
		}

		function genData()
		{
			var w, i, h, t;

			w = window.open('', 'tomato_data_d');
			w.document.writeln('<pre>');
			for (i = 0; i < daily_history.length; ++i) {
				h = daily_history[i];
				t = getYMD(h[0]);
				w.document.writeln([t[0], t[1] + 1, t[2], h[1], h[2]].join(','));
			}
			w.document.writeln('</pre>');
			w.document.close();

		}

		function getYMD(n)
		{
			// [y,m,d]
			return [(((n >> 16) & 0xFF) + 1900), ((n >>> 8) & 0xFF), (n & 0xFF)];
		}

		function redraw()
		{
			var h;
			var grid;
			var rows;
			var ymd;
			var d;
			var lastt;
			var lastu, lastd;

			if (daily_history.length > 0) {
				ymd = getYMD(daily_history[0][0]);
				d = new Date((new Date(ymd[0], ymd[1], ymd[2], 12, 0, 0, 0)).getTime() - ((30 - 1) * 86400000));
				E('last-dates').innerHTML = '(' + ymdText(ymd[0], ymd[1], ymd[2]) + ' to ' + ymdText(d.getFullYear(), d.getMonth(), d.getDate()) + ')';

				lastt = ((d.getFullYear() - 1900) << 16) | (d.getMonth() << 8) | d.getDate();
			}

			lastd = 0;
			lastu = 0;
			rows = 0;
			block = '';
			gn = 0;

			grid = '<table class="line-table td-large">';
			grid += '<tr><td style="width:25%;"><b>Date</b></td><td style="width:25%;"><b>Download</b></td><td style="width:25%;"><b>Upload</b></th><td style="width:25%;"><b>Total</b></td></tr>';


			for (i = 0; i < daily_history.length; ++i) {
				h = daily_history[i];
				ymd = getYMD(h[0]);
				grid += makeRow(((rows & 1) ? 'odd' : 'even'), ymdText(ymd[0], ymd[1], ymd[2]), rescale(h[1]), rescale(h[2]), rescale(h[1] + h[2]));
				++rows;

				if (h[0] >= lastt) {
					lastd += h[1];
					lastu += h[2];
				}
			}

			E('bwm-daily-grid').innerHTML = grid + '</table>';

			E('last-dn').innerHTML = rescale(lastd);
			E('last-up').innerHTML = rescale(lastu);
			E('last-total').innerHTML = rescale(lastu + lastd);
		}

		function init() {
			var s;

			if (nvram.rstats_enable != '1') { $('#rstats').before('<div class="alert alert-warning">Bandwidth monitoring disabled.</b> <a href="/#admin-bwm.asp">Enable &raquo;</a></div>'); return; }
			checkRstats();

			if ((s = cookie.get('daily')) != null) {
				if (s.match(/^([0-2])$/)) {
					E('scale').value = scale = RegExp.$1 * 1;
				}
			}

			initDate('ymd');
			daily_history.sort(cmpHist);
			redraw();
		}
	</script>

	<ul class="nav-tabs">
		<li><a class="ajaxload" href="bwm-realtime.asp"><i class="icon-hourglass"></i> Real-Time</a></li>
		<li><a class="ajaxload" href="bwm-24.asp"><i class="icon-graphs"></i> Last 24 Hours</a></li>
		<li><a class="active"><i class="icon-clock"></i> Daily</a></li>
		<li><a class="ajaxload" href="bwm-weekly.asp"><i class="icon-week"></i> Weekly</a></li>
		<li><a class="ajaxload" href="bwm-monthly.asp"><i class="icon-month"></i> Monthly</a></li>
	</ul>

	<div id="rstats" class="box">
		<div class="heading">Daily Bandwidth <a class="pull-right" href="#" data-toggle="tooltip" title="Reload Information" onclick="reloadPage(); return false;"><i class="icon-reboot"></i></a></div>
		<div class="content">
			<div id="bwm-daily-grid" class="span7" style="float: left;"></div>
			<div class="span2" style="float: left; margin-left: 20px;">
				<table class="data-table">
					<thead>
						<tr><th colspan=2 style="text-align:center">Last 30 Days<br><span style="font-weight:normal" id="last-dates"></span></th></tr>
					</thead>
					<tbody>
						<tr><td>Down</td><td id="last-dn">-</td></tr>
						<tr><td>Up</td><td id="last-up">-</td></tr>
						<tr><td>Total</td><td id="last-total">-</td></tr>
					</tbody>
				</table>
				<hr/>

				<b>Date</b>: &nbsp; <select onchange="changeDate(this, 'ymd')" id="dafm"><option value=0>yyyy-mm-dd</option><option value=1>mm-dd-yyyy</option><option value=2>mmm dd, yyyy</option><option value=3>dd.mm.yyyy</option></select><br>
				<b>Scale</b>: &nbsp; <select onchange="changeScale(this)" id="scale"><option value=0>KB</option><option value=1>MB</option><option value=2 selected>GB</option></select><br>

			</div>
		</div>
	</div>

	<a class="btn btn-primary" href="javascript:genData();">Data <i class="icon-drive"></i></a>
	<a class="btn btn-danger ajaxload" href="admin-bwm.asp">Configure <i class="icon-tools"></i></a>
	<script type="text/javascript">init();</script>
</content>