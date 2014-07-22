<!--
Tomato GUI
Copyright (C) 2006-2007 Jonathan Zarate
http://www.polarcloud.com/tomato/
For use with Tomato Firmware only.
No part of this file may be used without permission.
LAN Access admin module by Augusto Bott
-->
<title>IP Traffic Graphs</title>
<content>
	<style type="text/css">
		.color {
			width: 12px;
			height: 25px;
		}
		.title {
			padding: 0 5px;
		}
		.count {
			text-align: right;
		}
		.pct {
			width:55px;
			padding-right: 10px;
			text-align: right;
		}
		.thead {
			font-size: 90%;
			font-weight: bold;
		}
		.total {
			border-top: 1px dashed #bbb;
			font-weight: bold;
			margin-top: 5px;
		}

	</style>
	<script type="text/javascript">
		//	<% nvram('at_update,tomatoanon_answer,cstats_enable,lan_ipaddr,lan1_ipaddr,lan2_ipaddr,lan3_ipaddr,lan_netmask,lan1_netmask,lan2_netmask,lan3_netmask,dhcpd_static,web_svg'); %>
		// <% iptraffic(); %>

		nfmarks = [];
		irates = [];
		orates = [];
		var svgReady = 0;
		var abc = ['', '', '', '', '', '', '', '', '', '', ''];
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
			'ffd700'
		];
		var lock = 0;
		var prevtimestamp = new Date().getTime();
		var thistimestamp;
		var difftimestamp;
		var avgiptraffic = [];
		var lastiptraffic = iptraffic;
		function updateLabels() {
			var i = 0;
			while ((i < abc.length) && (i < iptraffic.length)) {
				abc[i] = iptraffic[i][0]; // IP address
				i++;
			}
		}
		updateLabels();
		i = 0;
		while (i < 11){
			if (iptraffic[i] != null) {
				nfmarks[i] = iptraffic[i][9] + iptraffic[i][10]; // TCP + UDP connections
			} else {
				nfmarks[i] = 0;
			}
			irates[i] = 0;
			orates[i] = 0;
			i++;
		}
		function mClick(n) {
			loadPage ('/#bwm-ipt-details.asp?ipt_filterip=' + abc[n]);
		}
		function showData() {
			var i, n, p;
			var ct, rt, ort;
			ct = rt = ort = 0;
			for (i = 0; i < 11; ++i) {
				if (!nfmarks[i]) nfmarks[i] = 0;
				ct += nfmarks[i];
				if (!irates[i]) irates[i] = 0;
				rt += irates[i];
				if (!orates[i]) orates[i] = 0;
				ort += orates[i];
			}
			for (i = 0; i < 11; ++i) {
				n = nfmarks[i];
				E('ccnt' + i).innerHTML = (abc[i] != '') ? n : '';
				if (ct > 0) p = (n / ct) * 100;
				else p = 0;
				E('cpct' + i).innerHTML = (abc[i] != '') ? p.toFixed(2) + '%' : '';
			}
			E('ccnt-total').innerHTML = ct;
			for (i = 0; i < 11; ++i) {
				n = irates[i];
				E('bcnt' + i).innerHTML = (abc[i] != '') ? (n / 125).toFixed(2) : '';
				E('bcntx' + i).innerHTML = (abc[i] != '') ? (n / 1024).toFixed(2) : '';
				if (rt > 0) p = (n / rt) * 100;
				else p = 0;
				E('bpct' + i).innerHTML = (abc[i] != '') ? p.toFixed(2) + '%' : '';
			}
			E('bcnt-total').innerHTML = (rt / 125).toFixed(2);
			E('bcntx-total').innerHTML = (rt / 1024).toFixed(2);
			for (i = 0; i < 11; ++i) {
				n = orates[i];
				E('obcnt' + i).innerHTML = (abc[i] != '') ? (n / 125).toFixed(2) : '';
				E('obcntx' + i).innerHTML = (abc[i] != '') ? (n / 1024).toFixed(2) : '';
				if (ort > 0) p = (n / ort) * 100;
				else p = 0;
				E('obpct' + i).innerHTML = (abc[i] != '') ? p.toFixed(2) + '%' : '';
			}
			E('obcnt-total').innerHTML = (ort / 125).toFixed(2);
			E('obcntx-total').innerHTML = (ort / 1024).toFixed(2);
		}
		function getArrayPosByElement(haystack, needle, index) {
			for (var i = 0; i < haystack.length; ++i) {
				if (haystack[i][index] == needle) {
					return i;
				}
			}
			return -1;
		}
		var ref = new TomatoRefresh('update.cgi', 'exec=iptraffic', 2, 'ipt_graphs');
		ref.refresh = function(text) {
			var b, i, j, k, l;
			++lock;
			thistimestamp = new Date().getTime();
			nfmarks = [];
			irates = [];
			orates = [];
			iptraffic = [];
			try {
				eval(text);
			}
			catch (ex) {
				nfmarks = [];
				irates = [];
				orates = [];
				iptraffic = [];
			}
			difftimestamp = thistimestamp - prevtimestamp;
			prevtimestamp = thistimestamp;
			for (i = 0; i < iptraffic.length; ++i) {
				b = iptraffic[i];
				j = getArrayPosByElement(avgiptraffic, b[0], 0);
				if (j == -1) {
					j = avgiptraffic.length;
					avgiptraffic[j] = [ b[0], 0, 0, 0, 0, 0, 0, 0, 0, b[9], b[10] ];
				}
				k = getArrayPosByElement(lastiptraffic, b[0], 0);
				if (k == -1) {
					k = lastiptraffic.length;
					lastiptraffic[k] = b;
				}
				for (l = 1; l <= 8; ++l) {
					avgiptraffic[j][l] = ((b[l] - lastiptraffic[k][l]) / difftimestamp * 1000);
					lastiptraffic[k][l] = b[l];
				}
				avgiptraffic[j][9] = b[9];
				avgiptraffic[j][10] = b[10];
				lastiptraffic[k][9] = b[9];
				lastiptraffic[k][10] = b[10];
			}
			-- lock;
			i = 0;
			while (i < 11){
				if (iptraffic[i] != null) {
					nfmarks[i] = avgiptraffic[i][9] + avgiptraffic[i][10]; // TCP + UDP connections
					irates[i] = avgiptraffic[i][1]; // RX bytes
					orates[i] = avgiptraffic[i][2]; // TX bytes
				} else {
					nfmarks[i] = 0;
					irates[i] = 0;
					orates[i] = 0;
				}
				++i;
			}

			showData();
			if (svgReady == 1) {
				updateCD(nfmarks, abc);
				updateBD(irates, abc);
				updateOB(orates, abc);
			}

		}
		function checkSVG() {
			var i, e, d, w;
			try {
				for (i = 2; i >= 0; --i) {
					e = E('svg' + i);
					d = e.getSVGDocument();
					if (d.defaultView) w = d.defaultView;
					else w = e.getWindow();
					if (!w.ready) break;
					if (i == 0) updateCD = w.updateSVG;
					if (i == 1)	updateBD = w.updateSVG;
					if (i == 2)	updateOB = w.updateSVG;
				}
			}
			catch (ex) {
			}
			if (i < 0) {
				svgReady = 1;
				updateCD(nfmarks, abc);
				updateBD(irates, abc);
				updateOB(orates, abc);
			}
			else if (--svgReady > -5) {
				setTimeout(checkSVG, 500);
			}
		}

		function init() {

			if (nvram.cstats_enable != '1') {
				$('.cstats').before('<div class="alert info">IP Traffic monitoring disabled.</b> <a href="/#admin-iptraffic.asp">Enable &raquo;</a>');
				return;
			}

			// SVG's
			if (nvram.web_svg != '0') {

				for (i=0; i < 3; i++) {

					$('#svg-'+i).html('<embed id="svg' + i + '" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/" src="img/ipt-graph.svg?n=' + i + '&v=<% version(); %>" style="width:310px;height:310px;"></embed>').css('text-align', 'center');
				}
			}

			// Data
			for (i = 0; i < 11; ++i) {
				$('#firstTable').prepend('<tr>' +
					'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
					'<td class="title" style="padding-left: 8px;"><a class="ajaxload" href="/#bwm-ipt-details.asp?ipt_filterip=' + abc[i] + '">' + abc[i] + '</a></td>' +
					'<td id="ccnt' + i + '" class="count"></td>' +
					'<td id="cpct' + i + '" class="pct"></td></tr>');
			}

			for (i = 0; i < 11; ++i) {
				$('#secondTable').prepend('<tr>' +
					'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
					'<td class="title" style="padding-left: 8px;"><a class="ajaxload" href="/#bwm-ipt-details.asp?ipt_filterip=' + abc[i] + '">' + abc[i] + '</a></td>' +
					'<td id="bcnt' + i + '" class="count"></td>' +
					'<td id="bcntx' + i + '" class="count"></td>' +
					'<td id="bpct' + i + '" class="pct"></td></tr>');
			}

			for (i = 0; i < 11; ++i) {
				$('#thirdTable').prepend('<tr>' +
					'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
					'<td class="title" style="padding-left: 8px;"><a class="ajaxload" href="/#bwm-ipt-details.asp?ipt_filterip=' + abc[i] + '">' + abc[i] + '</a></td>' +
					'<td id="obcnt' + i + '" class="count"></td>' +
					'<td id="obcntx' + i + '" class="count"></td>' +
					'<td id="obpct' + i + '" class="pct"></td></tr>');
			}

			showData();
			checkSVG();
			ref.initPage(2000, 3);
			if (!ref.running) ref.once = 1;
			ref.start();

		}
	</script>

	<ul class="nav-tabs">
		<li><a class="ajaxload" href="bwm-ipt-realtime.asp"><i class="icon-hourglass"></i> Real-Time</a></li>
		<li><a class="ajaxload" href="bwm-ipt-24.asp"><i class="icon-clock"></i> Last 24 Hours</a></li>
		<li><a class="active"><i class="icon-graphs"></i> View Graphs</a></li>
		<li><a class="ajaxload" href="bwm-ipt-details.asp"><i class="icon-globe"></i> Transfer Rates</a></li>
		<li><a class="ajaxload" href="bwm-ipt-daily.asp"><i class="icon-clock"></i> Daily</a></li>
		<li><a class="ajaxload" href="bwm-ipt-monthly.asp"><i class="icon-month"></i> Monthly</a></li>
	</ul>

	<div class="fluid-grid x3">
		<div class="box graphs">
			<div class="heading">IP Traffic</div>
			<div class="content">
				<div id="svg-0"></div>
				<table id="firstTable">
					<tr><td class="color" style="height:1em"></td><td class="title" style="width:45px">&nbsp;</td><td class="thead count">kbit/s</td><td class="thead count">KB/s</td><td class="pct">&nbsp;</td></tr>
					<tr><td>&nbsp;</td><td class="total">Total</td><td id="ccnt-total" class="total count"></td><td class="total pct">100%</td></tr>
				</table>
			</div>
		</div>

		<div class="box graphs">
			<div class="heading">Bandwidth Distribution (Inbound)</div>
			<div class="content">
				<div id="svg-1"></div>

				<table id="secondTable">
					<tr><td class="color" style="height:1em"></td><td class="title" style="width:45px">&nbsp;</td><td class="thead count">kbit/s</td><td class="thead count">KB/s</td><td class="pct">&nbsp;</td></tr>
					<tr><td>&nbsp;</td><td class="total">Total</td><td id="bcnt-total" class="total count"></td><td id="bcntx-total" class="total count"></td><td class="total pct">100%</td></tr>
				</table>
			</div>
		</div>

		<div class="box graphs">
			<div class="heading">Bandwidth Distribution (Outbound)</div>
			<div class="content">
				<div id="svg-2"></div>

				<table id="thirdTable">
					<tr><td class="color" style="height:1em"></td><td class="title" style="width:45px">&nbsp;</td><td class="thead count">kbit/s</td><td class="thead count">KB/s</td><td class="pct">&nbsp;</td></tr>
					<tr><td>&nbsp;</td><td class="total">Total</td><td id="obcnt-total" class="total count"></td><td id="obcntx-total" class="total count"></td><td class="total pct">100%</td></tr>
				</table>
			</div>
		</div>
	</div>

	<div id="refresh"></div>
	<script type="text/javascript">$('#refresh').html(genStdRefresh(1,1,'ref.toggle()')); init();</script>

</content>