<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

IP Traffic enhancements
Copyright (C) 2011 Augusto Bott
http://code.google.com/p/tomato-sdhc-vlan/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>IP Traffic: Details</title>
<content><style type="text/css">
		#grid .co2,
		#grid .co3,
		#grid .co4,
		#grid .co5,
		#grid .co6,
		#grid .co7,
		#grid .co8 {
			text-align: right;
		}
	</style>
	<script type="text/javascript" src="js/protocols.js"></script>
	<script type="text/javascript" src="js/bwm-hist.js"></script>
	<script type="text/javascript" src="js/bwm-common.js"></script>
	<script type="text/javascript" src="js/interfaces.js"></script>
	<script type="text/javascript">
		// <% nvram('at_update,tomatoanon_answer,cstats_enable,lan_ipaddr,lan1_ipaddr,lan2_ipaddr,lan3_ipaddr,lan_netmask,lan1_netmask,lan2_netmask,lan3_netmask,dhcpd_static'); %>
		//<% devlist(); %>

		var cstats_busy = 0;

		try {
			// <% iptraffic(); %>
		}
		catch (ex) {
			iptraffic = [];
			cstats_busy = 1;
		}

		if (typeof(iptraffic) == 'undefined') {
			iptraffic = [];
			cstats_busy = 1;
		}

		var grid = new TomatoGrid();

		var scale = -1;
		var lock = 0;

		var filterip = [];
		var filteripe = [];
		var filteripe_before = [];

		var prevtimestamp = new Date().getTime();
		var thistimestamp;
		var difftimestamp;
		var avgiptraffic = [];
		var lastiptraffic = iptraffic;

		hostnamecache = [];

		var ref = new TomatoRefresh('/update.cgi', '', 0, 'ipt_details');
		ref.refresh = function(text) {

			++lock;

			var i, b, j, k, l;

			thistimestamp = new Date().getTime();

			try {
				eval(text);
			}
			catch (ex) {
				iptraffic = [];
				cstats_busy = 1;
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

			setTimeout(function() { E('loading').style.display = 'none'; }, 100);

			--lock;

			grid.populate();
		}

		grid.populate = function() {
			if ((lock) || (cstats_busy)) return;

			var hostslisted = [];

			var i, b, x;
			var fskip;

			var tx = 0;
			var rx = 0;
			var tcpi = 0;
			var tcpo = 0;
			var udpi = 0;
			var udpo = 0;
			var icmpi = 0;
			var icmpo = 0;
			var tcpconn = 0;
			var udpconn = 0;

			grid.removeAllData();

			for (i = 0; i < avgiptraffic.length; ++i) {
				fskip = 0;
				b = avgiptraffic[i];

				if(E('_f_onlyactive').checked) {
					if ((b[2] < 10) && (b[3] < 10))
						continue;
				}

				if (filteripe.length>0) {
					fskip = 0;
					for (x = 0; x < filteripe.length; ++x) {
						if (b[0] == filteripe[x]){
							fskip=1;
							break;
						}
					}
					if (fskip == 1) continue;
				}

				if (filterip.length>0) {
					fskip = 1;
					for (x = 0; x < filterip.length; ++x) {
						if (b[0] == filterip[x]){
							fskip=0;
							break;
						}
					}
					if (fskip == 1) continue;
				}

				rx += b[1];
				tx += b[2];
				tcpi += b[3];
				tcpo += b[4];
				udpi += b[5];
				udpo += b[6];
				icmpi += b[7];
				icmpo += b[8];
				tcpconn += b[9];
				udpconn += b[10];
				hostslisted.push(b[0]);

				var h = b[0];
				if (E('_f_hostnames').checked) {
					if(hostnamecache[b[0]] != null) {
						h = hostnamecache[b[0]] + ((b[0].indexOf(':') != -1) ? '<br>' : ' ') + '<small>(' + b[0] + ')</small>';
					}
				}

				if (E('_f_shortcuts').checked) {
					h = h + '<br><small>';
					h = h + '<a href="javascript:viewQosDetail(' + i + ')" title="View QoS Details">[qosdetails]</a>';
					h = h + '<a href="javascript:viewQosCTrates(' + i + ')" title="View transfer rates per connection">[qosrates]</a>';
					h = h + '<a href="javascript:viewIptHistory(' + i + ')" title="View IP Traffic History">[history]</a>';
					h = h + '<a href="javascript:addExcludeList(' + i + ')" title="Filter out this address">[hide]</a>';
					h = h + '</small>';
				}

				d = [h,
					rescale((b[1]/1024).toFixed(2)).toString(),
					rescale((b[2]/1024).toFixed(2)).toString(),
					b[3].toFixed(0).toString(),
					b[4].toFixed(0).toString(),
					b[5].toFixed(0).toString(),
					b[6].toFixed(0).toString(),
					b[7].toFixed(0).toString(),
					b[8].toFixed(0).toString(),
					b[9].toString(),
					b[10].toString()];

				grid.insertData(-1, d);
			}

			grid.resort();
			grid.recolor();
			grid.footerSet([ 'Total ' + ('<small><i>(' + ((hostslisted.length > 0) ? (hostslisted.length + ' hosts') : 'no data') + ')</i></small>'),
				rescale((rx/1024).toFixed(2)).toString(),
				rescale((tx/1024).toFixed(2)).toString(),
				tcpi.toFixed(0).toString() + '/' + tcpo.toFixed(0).toString(),
				udpi.toFixed(0).toString() + '/' + udpo.toFixed(0).toString(),
				icmpi.toFixed(0).toString() + '/' + icmpo.toFixed(0).toString(),
				tcpconn.toString(), udpconn.toString() ]);
		}

		grid.sortCompare = function(a, b) {
			var col = this.sortColumn;
			var da = a.getRowData();
			var db = b.getRowData();
			var r = 0;

			switch (col) {
				case 0:	// host
					r = cmpText(da[col], db[col]);
					break;
				case 1:	// Download
				case 2:	// Upload
					r = cmpFloat(da[col], db[col]);
					break;
				case 3:	// TCP pkts
					r = cmpInt(da[3]+da[4], db[3]+db[4]);
					break;
				case 4:	// UDP pkts
					r = cmpInt(da[5]+da[6], db[5]+db[6]);
					break;
				case 5:	// ICMP pkts
					r = cmpInt(da[7]+da[8], db[7]+db[8]);
					break;
				case 6:	// TCP connections
					r = cmpInt(da[9], db[9]);
					break;
				case 7:	// UDP connections
					r = cmpInt(da[10], db[10]);
					break;
			}
			return this.sortAscending ? r : -r;
		}

		function popupWindow(v) {
			window.open(v, '', 'width=1280,height=600,toolbar=no,menubar=no,scrollbars=yes,resizable=yes');
		}

		function viewQosDetail(n) {
			cookie.set('qos_filterip', [avgiptraffic[n][0]], 1);
			popupWindow('#qos-detailed.asp');
		}

		function viewQosCTrates(n) {
			cookie.set('qos_filterip', [avgiptraffic[n][0]], 1);
			popupWindow('#qos-ctrate.asp');
		}

		function viewIptHistory(n) {
			cookie.set('ipt_filterip', [avgiptraffic[n][0]], 1);
			popupWindow('#bwm-ipt-daily.asp');
		}

		function addExcludeList(n) {
			if (E('_f_filter_ipe').value.length<6) {
				E('_f_filter_ipe').value = avgiptraffic[n][0];
			} else {
				if (E('_f_filter_ipe').value.indexOf(avgiptraffic[n][0]) < 0) {
					E('_f_filter_ipe').value = E('_f_filter_ipe').value + ',' + avgiptraffic[n][0];
				}
			}
			dofilter();
		}

		grid.dataToView = function(data) {
			return [data[0].toString(),
				data[1].toString(),
				data[2].toString(),
				data[3] + '/' + data[4],
				data[5] + '/' + data[6],
				data[7] + '/' + data[8],
				data[9].toString(),
				data[10].toString() ];
		}

		grid.setup = function() {
			this.init('grid', 'sort');
			this.headerSet(['Host', 'Download (bytes/s)', 'Upload (bytes/s)', 'TCP IN/OUT (pkt/s)', 'UDP IN/OUT (pkt/s)', 'ICMP IN/OUT (pkt/s)', 'TCP Connections', 'UDP Connections']);
		}

		function init() {

			if (nvram.cstats_enable != '1') {
				$('.cstats').before('<div class="alert info">IP Traffic monitoring disabled.</b> <a href="/#admin-iptraffic.asp">Enable &raquo;</a>');
				return;
			}

			if ((c = '<% cgi_get("ipt_filterip"); %>') != '') {
				c = c.replace(/\?_http(.*)/, '');
				if (c.length>6) {
					E('_f_filter_ip').value = c;
					filterip = c.split(',');
				}
			}

			if ((c = cookie.get('ipt_filterip')) != null) {
				cookie.set('ipt_filterip', '', 0);
				if (c.length>6) {
					E('_f_filter_ip').value = E('_f_filter_ip').value + ((E('_f_filter_ip').value.length > 0) ? ',' : '') + c;
					filterip.push(c.split(','));
				}
			}

			if ((c = cookie.get('ipt_addr_hidden')) != null) {
				if (c.length>6) {
					E('_f_filter_ipe').value = c;
					filteripe = c.split(',');
				}
			}

			filteripe_before = filteripe;

			if (((c = cookie.get('ipt_details_options_vis')) != null) && (c == '1')) {
				toggleVisibility("options");
			}

			scale = fixInt(cookie.get('ipt_details_scale'), 0, 2, 0);

			E('_f_scale').value = scale;

			E('_f_onlyactive').checked = (((c = cookie.get('ipt_details_onlyactive')) != null) && (c == '1'));

			E('_f_hostnames').checked = (((c = cookie.get('ipt_details_hostnames')) != null) && (c == '1'));

			E('_f_shortcuts').checked = (((c = cookie.get('ipt_details_shortcuts')) != null) && (c == '1'));

			populateCache();
			grid.setup();
			ref.postData = 'exec=iptraffic';
			ref.initPage(250);
			if (!ref.running) ref.once = 1;
			ref.start();
		}

		function getArrayPosByElement(haystack, needle, index) {
			for (var i = 0; i < haystack.length; ++i) {
				if (haystack[i][index] == needle) {
					return i;
				}
			}
			return -1;
		}

		function dofilter() {
			var i;

			if (E('_f_filter_ip').value.length>0) {
				filterip = E('_f_filter_ip').value.split(',');
				for (i = 0; i < filterip.length; ++i) {
					if ((filterip[i] = fixIP(filterip[i])) == null) {
						filterip.splice(i,1);
					}
				}
				E('_f_filter_ip').value = (filterip.length > 0) ? filterip.join(',') : '';
			} else {
				filterip = [];
			}

			if (E('_f_filter_ipe').value.length>0) {
				filteripe = E('_f_filter_ipe').value.split(',');
				for (i = 0; i < filteripe.length; ++i) {
					if ((filteripe[i] = fixIP(filteripe[i])) == null) {
						filteripe.splice(i,1);
					}
				}
				E('_f_filter_ipe').value = (filteripe.length > 0) ? filteripe.join(',') : '';
			} else {
				filteripe = [];
			}

			if (filteripe_before != filteripe) {
				cookie.set('ipt_addr_hidden', (filteripe.length > 0) ? filteripe.join(',') : '', 1);
				filteripe_before = filteripe;
			}

			grid.populate();
		}

		function verifyFields(focused, quiet) {
			scale = E('_f_scale').value * 1;
			cookie.set('ipt_details_scale', E('_f_scale').value, 2);

			cookie.set('ipt_details_onlyactive', (E('_f_onlyactive').checked ? '1' : '0'), 1);

			cookie.set('ipt_details_hostnames', (E('_f_hostnames').checked ? '1' : '0'), 1);

			cookie.set('ipt_details_shortcuts', (E('_f_shortcuts').checked ? '1' : '0'), 1);

			dofilter();
			return 1;
		}

		function toggleVisibility(whichone) {
			if(E('sesdiv' + whichone).style.display=='') {
				E('sesdiv' + whichone).style.display='none';
				E('sesdiv' + whichone + 'showhide').innerHTML='<i class="icon-chevron-up"></i>';
				cookie.set('ipt_details_' + whichone + '_vis', 0);
			} else {
				E('sesdiv' + whichone).style.display='';
				E('sesdiv' + whichone + 'showhide').innerHTML='<i class="icon-chevron-down"></i>';
				cookie.set('ipt_details_' + whichone + '_vis', 1);
			}
		}

	</script>

	<ul class="nav-tabs">
		<li><a class="ajaxload" href="bwm-ipt-realtime.asp"><i class="icon-hourglass"></i> Real-Time</a></li>
		<li><a class="ajaxload" href="bwm-ipt-24.asp"><i class="icon-clock"></i> Last 24 Hours</a></li>
		<li><a class="ajaxload" href="bwm-ipt-graphs.asp"><i class="icon-graphs"></i> View Graphs</a></li>
		<li><a class="active"><i class="icon-globe"></i> Transfer Rates</a></li>
		<li><a class="ajaxload" href="bwm-ipt-daily.asp"><i class="icon-clock"></i> Daily</a></li>
		<li><a class="ajaxload" href="bwm-ipt-monthly.asp"><i class="icon-month"></i> Monthly</a></li>
	</ul>

	<div id="cstats" class="box">
		<div class="heading">IP Traffic Details</div>
		<div class="content">
			<table id="grid" class="line-table"></table><br />
			<div id="loading"><br><b>Loading... </b></div>

			<h4><a href="javascript:toggleVisibility('options');">Options <span id="sesdivoptionsshowhide"><i class="icon-chevron-up"></i></span></a></h4>
			<div class="section" id="sesdivoptions" style="display:none"></div>
		</div>
	</div>

	<div class="pull-right refreshier">
		<script type="text/javascript">$('.refreshier').html(genStdRefresh(1,1,'ref.toggle()'));</script>
	</div>
	<a href="admin-iptraffic.asp" class="btn btn-danger ajaxload">Configure <i class="icon-tools"></i></a>

	<script type="text/javascript">
		var c;
		c = [];
		c.push({ title: 'Only these IPs', name: 'f_filter_ip', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)</small>' });
		c.push({ title: 'Exclude these IPs', name: 'f_filter_ipe', size: 50, maxlen: 255, type: 'text', suffix: ' <small>(Comma separated list)</small>' });
		c.push({ title: 'Scale', name: 'f_scale', type: 'select', options: [['0', 'KB'], ['1', 'MB'], ['2', 'GB']] });
		c.push({ title: 'Ignore inactive hosts', name: 'f_onlyactive', type: 'checkbox' });
		c.push({ title: 'Show hostnames', name: 'f_hostnames', type: 'checkbox' });
		c.push({ title: 'Show shortcuts', name: 'f_shortcuts', type: 'checkbox' });
		$('#sesdivoptions').forms(c);
	</script>

	<script type="text/javascript">checkCstats(); init();</script>
</content>