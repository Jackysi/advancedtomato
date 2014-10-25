<!DOCTYPE html>
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Bandwidth: Monthly</title>
<content>
	<script type="text/javascript" src="js/bwm-hist.js"></script>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,wan_ifname,lan_ifname,rstats_enable"); %>

		try {
			//	<% bandwidth("monthly"); %>
		}
		catch (ex) {
			monthly_history = [];
		}
		rstats_busy = 0;
		if (typeof(monthly_history) == 'undefined') {
			monthly_history = [];
			rstats_busy = 1;
		}

		function genData()
		{
			var w, i, h;

			w = window.open('', 'tomato_data_m');
			w.document.writeln('<pre>');
			for (i = 0; i < monthly_history.length; ++i) {
				h = monthly_history[i];
				w.document.writeln([(((h[0] >> 16) & 0xFF) + 1900), (((h[0] >>> 8) & 0xFF) + 1), h[1], h[2]].join(','));
			}
			w.document.writeln('</pre>');
			w.document.close();
		}

		function save()
		{
			cookie.set('monthly', scale, 31);
		}

		function redraw()
		{
			var h;
			var grid;
			var rows;
			var yr, mo, da;

			rows = 0;
			block = '';
			gn = 0;

			grid = '<table class="line-table td-large">';
			grid += '<tr><td><b>Date</b></td><td><b>Download</b></td><td><b>Upload</b></th><td><b>Total</b></td></tr>';

			for (i = 0; i < monthly_history.length; ++i) {
				h = monthly_history[i];
				yr = (((h[0] >> 16) & 0xFF) + 1900);
				mo = ((h[0] >>> 8) & 0xFF);

				grid += makeRow(((rows & 1) ? 'odd' : 'even'), ymText(yr, mo), rescale(h[1]), rescale(h[2]), rescale(h[1] + h[2]));
				++rows;
			}

			E('bwm-monthly-grid').innerHTML = grid + '</table>';
		}

		function init()
		{
			var s;

			if (nvram.rstats_enable != '1') { $('#rstats').before('<div class="alert">Bandwidth monitoring disabled.</b> <a href="/#admin-bwm.asp">Enable &raquo;</a></div>'); return; }

			if ((s = cookie.get('monthly')) != null) {
				if (s.match(/^([0-2])$/)) {
					E('scale').value = scale = RegExp.$1 * 1;
				}
			}

			initDate('ym');
			monthly_history.sort(cmpHist);
			redraw();
		}
	</script>

	<ul class="nav-tabs">
		<li><a class="ajaxload" href="bwm-realtime.asp"><i class="icon-hourglass"></i> Real-Time</a></li>
		<li><a class="ajaxload" href="bwm-24.asp"><i class="icon-graphs"></i> Last 24 Hours</a></li>
		<li><a class="ajaxload" href="bwm-daily.asp"><i class="icon-clock"></i> Daily</a></li>
		<li><a class="ajaxload" href="bwm-weekly.asp"><i class="icon-week"></i> Weekly</a></li>
		<li><a class="active"><i class="icon-month"></i> Monthly</a></li>
	</ul>

	<div id="rstats" class="box">
		<div class="heading">Monthly Bandwidth <a class="pull-right" href="#" data-toggle="tooltip" title="Reload Information" onclick="reloadPage(); return false;"><i class="icon-reboot"></i></a></div>
		<div class="content">
			<div id="bwm-monthly-grid"></div>
		</div>
	</div>

	<a href="javascript:genData()" class="btn btn-primary">Data <i class="icon-drive"></i></a>
	<a href="admin-bwm.asp" class="btn btn-danger ajaxload">Configure <i class="icon-tools"></i></a>
	<span class="pull-right">
		<b>Date</b> <select onchange="changeDate(this, 'ym')" id="dafm"><option value="0">yyyy-mm</option><option value="1">mm-yyyy</option><option value="2">mmm yyyy</option><option value="3">mm.yyyy</option></select> &nbsp;
		<b>Scale</b> <select onchange="changeScale(this)" id="scale"><option value="0">KB</option><option value="1">MB</option><option value="2" selected>GB</option></select>
	</span>
	<script type="text/javascript">init();</script>
</content>