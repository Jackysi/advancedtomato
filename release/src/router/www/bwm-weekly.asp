<!DOCTYPE html>
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
-->
<title>Bandwidth: Weekly</title>
<content>
	<style>
		table tr td { width: 25%; }
		.total { font-weight: bold; }
	</style>
	<script type="text/javascript" src="js/bwm-hist.js"></script>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,wan_ifname,lan_ifname,rstats_enable"); %>

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

		var weeks = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
		var weeksShort = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
		var startwk = 0;
		var summary = 1;

		function save()
		{
			cookie.set('weekly', scale + ',' + startwk + ',' + summary, 31);
		}

		function changeStart(e)
		{
			startwk = e.value * 1;
			redraw();
			save();
		}

		function changeMode(e)
		{
			summary = e.value * 1;
			redraw();
			save();
		}

		function nth(n)
		{
			n += '';
			switch (n.substr(n.length - 1, 1)) {
				case '1':
					return n + 'st';
				case '2':
					return n + 'nd';
				case '3':
					return n + 'rd';
			}
			return n + 'th';
		}

		function redraw()
		{
			var h;
			var grid;
			var block;
			var rows;
			var dend;
			var dbeg;
			var dl, ul;
			var d, diff, ds;
			var tick, lastSplit;
			var yr, mo, da, wk;
			var gn;
			var swk;

			rows = 0;
			block = [];
			gn = 0;
			w = 0;
			lastSplit = 0;
			ul = dl = 0;
			dend = dbeg = '';

			swk	= startwk - 1;
			if (swk < 0) swk = 6;

			if (summary) {
				grid = '<table class="line-table">';
				grid += '<tr><td><b>Date</b></td><td><b>Download</b></td><td><b>Upload</b></td><td><b>Total</b></td></tr>';
			}
			else {
				grid = '';
			}

			function flush_block()
			{
				grid += '<h5>' + dbeg + ' to ' + dend + '</h5>' +
				'<table class="line-table">' +
				'<tr><td><b>Date</b></td><td><b>Download</b></td><td><b>Upload</b></td><td><b>Total</b></td></tr>' +
				block.join('') +
				makeRow('bold', 'Total', rescale(dl), rescale(ul), rescale(dl + ul)) +
				'</table><br>';
			}

			for (i = 0; i < daily_history.length; ++i) {
				h = daily_history[i];
				yr = (((h[0] >> 16) & 0xFF) + 1900);
				mo = ((h[0] >>> 8) & 0xFF);
				da = (h[0] & 0xFF);
				d = new Date(yr, mo, da);
				wk = d.getDay();

				tick = d.getTime();
				diff = lastSplit - tick;

				ds = ymdText(yr, mo, da) + ' <small>(' + weeksShort[wk] + ')</small>';

				/*	REMOVE-BEGIN

				Jan 2007
				SU MO TU WE TH FR SA
				01 02 03 04 05 06
				07 08 09 10 11 12 13
				14 15 16 17 18 19 20
				21 22 23 24 25 26 27
				28 29 30 31

				Feb 2007
				SU MO TU WE TH FR SA
				01 02 03
				04 05 06 07 08 09 10
				11 12 13 14 15 16 17
				18 19 20 21 22 23 24
				25 26 27 28

				Mar 2007
				SU MO TU WE TH FR SA
				01 02 03
				04 05 06 07 08 09 10
				11 12 13 14 15 16 17
				18 19 20 21 22 23 24
				25 26 27 28 29 30 31

				REMOVE-END */

				if ((wk == swk) || (diff >= (7 * 86400000)) || (lastSplit == 0)) {
					if (summary) {
						if (i > 0) {
							grid += makeRow(((rows & 1) ? 'odd' : 'even'),
								dend + '<br>' + dbeg, rescale(dl), rescale(ul), rescale(dl + ul));
							++rows;
							++gn;
						}
					}
					else {
						if (rows) {
							flush_block();
							++gn;
						}
						block = [];
						rows = 0;
					}
					dl = ul = 0;
					dend = ds;
					lastSplit = tick;
				}

				dl += h[1];
				ul += h[2];
				if (!summary) {
					block.unshift(makeRow(((rows & 1) ? 'odd' : 'even'), weeks[wk] + ' <small>' + (mo + 1) + '-' + da + '</small>', rescale(h[1]), rescale(h[2]), rescale(h[1] + h[2])))
					++rows;
				}

				dbeg = ds;
			}

			if (summary) {
				if (gn < 9) {
					grid += makeRow(((rows & 1) ? 'odd' : 'even'),
						dend + '<br>' + dbeg, rescale(dl), rescale(ul), rescale(dl + ul));
				}
				grid += '</table>';
			}
			else {
				if ((rows) && (gn < 9)) {
					flush_block();
				}
			}
			E('bwm-weekly-grid').innerHTML = grid;
		}

		function init()
		{
			var s;

			if (nvram.rstats_enable != '1') { $('#rstats').before('<div class="alert">Bandwidth monitoring disabled.</b> <a href="/#admin-bwm.asp">Enable &raquo;</a></div>'); return; }

			if ((s = cookie.get('weekly')) != null) {
				if (s.match(/^([0-2]),([0-6]),([0-1])$/)) {
					E('scale').value = scale = RegExp.$1 * 1;
					E('startwk').value = startwk = RegExp.$2 * 1
					E('shmode').value = summary = RegExp.$3 * 1;
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
		<li><a class="ajaxload" href="bwm-daily.asp"><i class="icon-clock"></i> Daily</a></li>
		<li><a class="active"><i class="icon-week"></i> Weekly</a></li>
		<li><a class="ajaxload" href="bwm-monthly.asp"><i class="icon-month"></i> Monthly</a></li>
	</ul>

	<div id="rstats" class="box">
		<div class="heading">Weekly Bandwidth <a class="pull-right" href="#" data-toggle="tooltip" title="Reload Information" onclick="reloadPage(); return false;"><i class="icon-reboot"></i></a></div>
		<div class="content">
			<div id="bwm-weekly-grid"></div>

		</div>
	</div>

	<a href="admin-bwm.asp" class="btn btn-danger ajaxload">Configure <i class="icon-tools"></i></a> &nbsp;
	<b>Show</b> <select onchange="changeMode(this)" id="shmode"><option value=1 selected>Summary<option value=0>Full</select> &nbsp;
	<b>Date</b> <select onchange="changeDate(this, 'ymd')" id="dafm"><option value=0>yyyy-mm-dd</option><option value=1>mm-dd-yyyy</option><option value=2>mmm dd, yyyy</option><option value=3>dd.mm.yyyy</option></select>  &nbsp;
	<b>Start</b> <select onchange="changeStart(this)" id="startwk"><option value=0 selected>Sun<option value=1>Mon<option value=2>Tue<option value=3>Wed<option value=4>Thu<option value=5>Fri<option value=6>Sat</select>  &nbsp;
	<b>Scale</b> <select onchange="changeScale(this)" id="scale"><option value=0>KB</option><option value=1>MB</option><option value=2 selected>GB</option></select> &nbsp;


	<script type="text/javascript">init()</script>
</content>