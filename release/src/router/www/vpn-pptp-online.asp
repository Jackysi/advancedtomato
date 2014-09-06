<!--
Tomato PPTPd GUI
Copyright (C) 2012 Augusto Bott
http://code.google.com/p/tomato-sdhc-vlan/

Tomato GUI
Copyright (C) 2006-2007 Jonathan Zarate
http://www.polarcloud.com/tomato/
For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>PPTP Online</title>
<content>
	<style type="text/css">
		#dev-grid .co1 {
			width: 10%;
		}
		#dev-grid .co2 {
			width: 18%;
		}
		#dev-grid .co3 {
			width: 12%;
		}
		#dev-grid .co4 {
			width: 12%;
		}
		#dev-grid .co5 {
			width: 18%;
		}
		#dev-grid .co6 {
			width: 10%;
			text-align: center;
		}
		#dev-grid .header {
			text-align: left;
		}
	</style>
	<script type="text/javascript">
		//	<% nvram('at_update,tomatoanon_answer'); %>	// http_id
		//	<% pptpd_userol(); %>

		list = [];

		function find(pid) {
			var e, i;
			for (i = list.length - 1; i >= 0; --i) {
				e = list[i];
				if (e.pid == pid) {
					return e;
				}
			}
			return null;
		}

		function get(pid) {
			var e, i;
			if ((e = find(pid)) != null) {
				return e;
			}
			e = {
				pid: pid,
				sourceip: '',
				ip: '',
				ifname: '',
				username: '',
				uptime: ''
			};
			list.push(e);
			return e;
		}

		var xob = null;
		function _disconnect(pid) {
			form.submitHidden('/pptpd.cgi', { disconnect: pid });
		}

		function disconnect(a, pid) {
			if (xob) return;
			if ((xob = new XmlHttp()) == null) {
				_disconnect(ip);
				return;
			}
			a = E(a);
			a.innerHTML = 'Disconnecting...';

			xob.onCompleted = function(text, xml) {
				a.innerHTML = '';
				xob = null;
				/* REMOVE-BEGIN */
				//		ref.refresh(text);
				/* REMOVE-END */
				if (!ref.running) {
					setTimeout(
						function() {
							if (!ref.running) {
								ref.once = 1;
								ref.start();
							}
						}, 3000);
				}

			}

			xob.onError = function() {
				_disconnect(pid);
			}

			xob.post('/pptpd.cgi', 'disconnect=' + pid);
		}

		var ref = new TomatoRefresh('/update.cgi', 'exec=pptpd_userol', 0, 'pptpd_userol_refresh');

		ref.refresh = function(text) {
			eval(text);
			dg.removeAllData();
			dg.populate();
			dg.resort();
		}

		var dg = new TomatoGrid();

		dg.sortCompare = function(a, b) {
			var col = this.sortColumn;
			var ra = a.getRowData();
			var rb = b.getRowData();
			var r;
			switch (col) {
				case 3:
				case 4:
					r = cmpIP(ra.ip, rb.ip);
					break;
				default:
					r = cmpText(a.cells[col].innerHTML, b.cells[col].innerHTML);
			}
			if (r == 0) {
				r = cmpIP(ra.ip, rb.ip);
				if (r == 0) r = cmpText(ra.ifname, rb.ifname);
			}
			return this.sortAscending ? r : -r;
		}

		dg.populate = function() {
			var i, a, b, c, e;
			var hangup;
			list = [];
			for (i = 0; i < list.length; ++i) {
				list[i].pid = '';
				list[i].ifname = '';
				list[i].username = '';
				list[i].uptime = '';
				list[i].ip = '';
				list[i].sourceip = '';
			}

			for (i = pptpd_online.length - 1; i >= 0; --i) {
				a = pptpd_online[i];
				e = get(a[0]);
				e.ifname   = a[1];
				e.username = a[4];
				e.ip       = a[2];
				e.sourceip = a[3];
				e.uptime   = a[5];
			}

			for (i = list.length - 1; i >= 0; --i) {
				e = list[i];
				hangup = '<div id="div_'+e.pid+'"><a href="javascript:disconnect(\'div_'+e.pid + '\',' + e.pid + ')" title="Disconnect" id="pid_' + e.pid + '">Disconnect</a></div>';
				/* REMOVE-BEGIN */
				//		this.insert(-1, e, [
				//			e.ifname, e.username, e.uptime, e.ip, e.sourceip, hangup], false);
				/* REMOVE-END */
				dg.insertData(-1, [ e.ifname, e.username, e.uptime, e.ip, e.sourceip, hangup ]);
			}
		}

		dg.setup = function() {
			this.init('dev-grid', 'sort');
			this.headerSet(['Interface', '<b>Username', 'Online Since', 'VPN IP Address', 'Source IP Address', 'Action']);
			this.populate();
			this.sort(1);
		}

		function dateTimeString(timestamp) {
			var x = new Date(timestamp*1000);
			var year = x.getFullYear();
			var month = (x.getMonth() + 1).pad(2);
			var day = (x.getDate()).pad(2);
			var time = timeString((x.getHours() * 60) + x.getMinutes());
			return (year + '-' + month + '-' + day + ' ' + time);
		}

		dg.dataToView = function(data) {
			var l;
			if (data[2] < 946684800) {
				l = 'Not Available';
			} else {
				/* REMOVE-BEGIN */
				//		l = new Date(data[2] *1000);
				//		l = l.toDateString() + ' ' + l.toTimeString();
				/* REMOVE-END */
				l = dateTimeString(data[2]);
			}
			return [data[0],
				data[1],
				l,
				data[3],
				data[4],
				data[5]];
		}

		function earlyInit() {
			dg.setup();
			init();
		}

		function init() {
			dg.recolor();
			ref.initPage(3000, 3);
		}
	</script>

	<div class="box">
		<div class="heading">PPTP Users Online</div>
		<div class="content">
			<table id="dev-grid" class="line-table"></table><br />
		</div>
	</div>

	<a href="vpn-pptp-server.asp" class="btn btn-danger ajaxload">Configure <i class="icon-tools"></i></a>
	<script type="text/javascript">$('.btn-danger.ajaxload').after(genStdRefresh(1,1,"ref.toggle()"));</script>

	<script type="text/javascript">earlyInit();</script>
</content>