<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
-->
<title>View Graphs</title>
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
			text-align: right;
		}
		.thead {
			font-size: 90%;
			font-weight: 500;
		}
		.total {
			border-top: 1px dashed #bbb;
			font-weight: 500;
			margin-top: 5px;
		}

		.embedGraph {
			text-align: center;
		}

	</style>
	<script type="text/javascript">
		//<% nvram("qos_classnames,web_svg,qos_enable,wan_qos_obw,wan_qos_ibw,wan2_qos_obw,wan2_qos_ibw,wan3_qos_obw,wan3_qos_ibw,wan4_qos_obw,wan4_qos_ibw"); %>
		//<% qrate(); %>

		qrates_out = [0,0,0,0,0,0,0,0,0,0,0];
		qrates_in = [0,0,0,0,0,0,0,0,0,0,0];
		for ( var i = 0; i < 10; i++ ) {
			/* MULTIWAN-BEGIN */
			qrates_out[ i ] = qrates1_out[ i ] + qrates2_out[ i ] + qrates3_out[ i ] + qrates4_out[ i ];
			qrates_in[ i ]  = qrates1_in[ i ] + qrates2_in[ i ] + qrates3_in[ i ] + qrates4_in[ i ];
			/* MULTIWAN-END */
			/* DUALWAN-BEGIN */
			qrates_out[ i ] = qrates1_out[ i ] + qrates2_out[ i ];
			qrates_in[ i ]  = qrates1_in[ i ] + qrates2_in[ i ];
			/* DUALWAN-END */
		}

		var svgReady = 0;

		var Unclassified = ['Unclassified'];
		var Unused = ['Unused'];
		var classNames = nvram.qos_classnames.split(' ');		//Toastman Class Labels
		var abc = Unclassified.concat(classNames,Unused);

		//var abc = ['Unclassified', 'Highest', 'High', 'Medium', 'Low', 'Lowest', 'Class A', 'Class B', 'Class C', 'Class D', 'Class E'];

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
			// One level back or SVG will think we're in IMG folder and pop 500 ERROR
			loadPage('qos-detailed.asp', 'class=' + n);
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

		ref.refresh = function( text ) {

			nfmarks    = [];
			qrates_out = [];
			qrates_in  = [];

			for ( var i = 0; i < 10; i++ ) {
				/* MULTIWAN-BEGIN */
				qrates_out[ i ] = qrates1_out[ i ] + qrates2_out[ i ] + qrates3_out[ i ] + qrates4_out[ i ];
				qrates_in[ i ]  = qrates1_in[ i ] + qrates2_in[ i ] + qrates3_in[ i ] + qrates4_in[ i ];
				/* MULTIWAN-END */
				/* DUALWAN-BEGIN */
				qrates_out[ i ] = qrates1_out[ i ] + qrates2_out[ i ];
				qrates_in[ i ]  = qrates1_in[ i ] + qrates2_in[ i ];
				/* DUALWAN-END */
			}

			try {
				eval( text );
			}
			catch ( ex ) {
				nfmarks    = [];
				qrates_out = [];
				qrates_in  = [];
			}

			showData();
			if ( svgReady == 1 ) {
				updateConnectionDistribution( nfmarks, abc );
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

		function init() {

			// Write Graphs to content
			for (i=0; i < 3; i++) {
				$('#svg-'+i).html('<embed id="svg' + i + '" type="image/svg+xml" pluginspage="http://www.adobe.com/svg/viewer/install/" src="img/qos-graph.svg?n=' + i + '&v=<% version(); %>" style="width:310px;height:310px;"></embed>').css('text-align', 'center');
			}

			nbase = fixInt(cookie.get('qnbase'), 0, 1, 0);
			showData();
			checkSVG();
			showGraph();
			ref.initPage(2000, 3);
		}
	</script>

	<script type="text/javascript">
		if (nvram.qos_enable != '1') {
			$('.container .ajaxwrap').prepend('<div class="alert alert-info"><b>QoS is disabled.</b>&nbsp; <a class="ajaxload" href="qos-settings.asp">Enable &raquo;</a> <a class="close"><i class="icon-cancel"></i></a></div>');
		}
	</script>

	<div class="box graphs">
		<div class="heading">Connections Distribution</div>
		<div class="content">
			<div id="svg-0" class="embedGraph"></div>

			<table id="firstTable">
				<tr><td>&nbsp;</td><td class="total">Total</td><td id="ccnt-total" class="total count"></td><td class="total pct">100%</td></tr>
			</table>

			<script type="text/javascript">
				for (i = 0; i < 11; ++i) {
					$('#firstTable').prepend('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
						'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
						'<td class="title" style="width:60px">' + abc[i] + '</td>' +
						'<td id="ccnt' + i + '" class="count" style="width:90px"></td>' +
						'<td id="cpct' + i + '" class="pct"></td></tr>');
				}
			</script>
		</div>
	</div>

	<div class="fluid-grid x2">
		<div class="box graphs">
			<div class="heading">Bandwidth Distribution (Outbound)</div>
			<div class="content">
				<div id="svg-1" class="embedGraph"></div>

				<table id="secondTable">
					<tr><td class="color" style="height:1em; margin-right: 5px;"></td><td class="title">&nbsp;</td><td class="thead count">kbit/s</td><td class="thead count">KB/s</td><td class="thead pct">&nbsp;</td></tr>
					<tr><td>&nbsp;</td><td class="total">Total</a></td><td id="bocnt-total" class="total count"></td><td id="bocntx-total" class="total count"></td><td id="rateout" class="total pct"></td></tr>
				</table>

				<script type='text/javascript'>
					for (i = 1; i < 11; ++i) {
						$('#secondTable').prepend('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
							'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
							'<td class="title" style="width:45px">' + abc[i] + '</td>' +
							'<td id="bocnt' + i + '" class="count" style="width:60px"></td>' +
							'<td id="bocntx' + i + '" class="count" style="width:50px"></td>' +
							'<td id="bopct' + i + '" class="pct"></td></tr>');
					}
				</script>
			</div>
		</div>

		<div class="box graphs">
			<div class="heading">Bandwidth Distribution (Inbound)</div>
			<div class="content">
				<div id="svg-2" class="embedGraph"></div>

				<table id="thirdTable">
					<tr><td class="color" style="height:1em; margin-right: 5px;"></td><td class="title">&nbsp;</td><td class="thead count">kbit/s</td><td class="thead count">KB/s</td><td class="thead pct">&nbsp;</td></tr>
					<tr><td>&nbsp;</td><td class="total">Total</a></td><td id="bicnt-total" class="total count"></td><td id="bicntx-total" class="total count"></td><td id="ratein" class="total pct"></td></tr>
				</table>

				<script type="text/javascript">
					for (i = 1; i < 11; ++i) {
						$('#thirdTable').prepend('<tr style="cursor:pointer" onclick="mClick(' + i + ')">' +
							'<td class="color" style="background:#' + colors[i] + '" onclick="mClick(' + i + ')">&nbsp;</td>' +
							'<td class="title">' + abc[i] + '</td>' +
							'<td id="bicnt' + i + '" class="count"></td>' +
							'<td id="bicntx' + i + '" class="count"></td>' +
							'<td id="bipct' + i + '" class="pct"></td></tr>');
					}
				</script>

			</div>
		</div>
	</div>

	<button name="mybtn" value="Zoom Graphs" type="button" onclick="showGraph()" class="btn">Zoom Graphs <i class="icon-search"></i></button>
	<script type="text/javascript">$('button[name="mybtn"]').before(genStdRefresh(1,2,"ref.toggle()"));</script>

	<script type="text/javascript">init();</script>
</content>