<!--
Tomato GUI
Copyright (C) 2006-2008 Jonathan Zarate
http://www.polarcloud.com/tomato/

Copyright (C) 2011 Deon 'PrinceAMD' Thomas
rate limit & connection limit from Conanxu,
adapted by Victek, Shibby, PrinceAMD, Phykris

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Bandwidth Limiter</title>
<content>
	<script type="text/javascript">
		// <% nvram("qos_enable,at_update,tomatoanon_answer,new_qoslimit_enable,qos_ibw,qos_obw,new_qoslimit_rules,lan_ipaddr,lan_netmask,qosl_enable,qosl_dlr,qosl_dlc,qosl_ulr,qosl_ulc,qosl_udp,qosl_tcp,limit_br1_enable,limit_br1_dlc,limit_br1_dlr,limit_br1_ulc,limit_br1_ulr,limit_br1_prio,limit_br2_enable,limit_br2_dlc,limit_br2_dlr,limit_br2_ulc,limit_br2_ulr,limit_br2_prio,limit_br3_enable,limit_br3_dlc,limit_br3_dlr,limit_br3_ulc,limit_br3_ulr,limit_br3_prio"); %>

		var class_prio = [['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']];
		var class_tcp = [['0','nolimit']];
		var class_udp = [['0','nolimit']];
		for (var i = 1; i <= 100; ++i) {
			class_tcp.push([i*10, i*10+'']);
			class_udp.push([i, i + '/s']);
		}
		var qosg = new TomatoGrid();

		qosg.setup = function() {
			this.init('qosg-grid', '', 80, [
				{ type: 'text', maxlen: 31 },
				{ type: 'text', maxlen: 6 },
				{ type: 'text', maxlen: 6 },
				{ type: 'text', maxlen: 6 },
				{ type: 'text', maxlen: 6 },
				{ type: 'select', options: class_prio },
				{ type: 'select', options: class_tcp },
				{ type: 'select', options: class_udp }]);
			this.headerSet(['IP | IP Range | MAC Address', 'DLRate', 'DLCeil', 'ULRate', 'ULCeil', 'Priority', 'TCP Limit', 'UDP Limit']);
			var qoslimitrules = nvram.new_qoslimit_rules.split('>');
			for (var i = 0; i < qoslimitrules.length; ++i) {
				var t = qoslimitrules[i].split('<');
				if (t.length == 8) this.insertData(-1, t);
			}
			this.showNewEditor();
			this.resetNewEditor();
		}

		qosg.dataToView = function(data) {
			return [data[0],data[1]+'kbps',data[2]+'kbps',data[3]+'kbps',data[4]+'kbps',class_prio[data[5]*1][1],class_tcp[data[6]*1/10][1],class_udp[data[7]*1][1]];
		}

		qosg.resetNewEditor = function() {
			var f, c, n;

			var f = fields.getAll(this.newEditor);
			ferror.clearAll(f);
			if ((c = cookie.get('addbwlimit')) != null) {
				cookie.set('addbwlimit', '', 0);
				c = c.split(',');
				if (c.length == 2) {
					f[0].value = c[0];
					f[1].value = '';
					f[2].value = '';
					f[3].value = '';
					f[4].value = '';
					f[5].selectedIndex = '2';
					f[6].selectedIndex = '0';
					f[7].selectedIndex = '0';
					return;
				}
			}

			f[0].value = '';
			f[1].value = '';
			f[2].value = '';
			f[3].value = '';
			f[4].value = '';
			f[5].selectedIndex = '2';
			f[6].selectedIndex = '0';
			f[7].selectedIndex = '0';

		}

		qosg.exist = function(f, v)
		{
			var data = this.getAllData();
			for (var i = 0; i < data.length; ++i) {
				if (data[i][f] == v) return true;
			}
			return false;
		}

		qosg.existID = function(id)
		{
			return this.exist(0, id);
		}

		qosg.existIP = function(ip)
		{
			if (ip == "0.0.0.0") return true;
			return this.exist(1, ip);
		}

		qosg.checkRate = function(rate)
		{
			var s = parseInt(rate, 10);
			if( isNaN(s) || s <= 0 || a >= 100000 ) return true;
			return false;
		}

		qosg.checkRateCeil = function(rate, ceil)
		{
			var r = parseInt(rate, 10);
			var c = parseInt(ceil, 10);
			if( r > c ) return true;
			return false;
		}

		qosg.verifyFields = function(row, quiet)
		{
			var ok = 1;
			var f = fields.getAll(row);
			var s;

			/*
			if (v_ip(f[0], quiet)) {
			if(this.existIP(f[0].value)) {
			ferror.set(f[0], 'duplicate IP address', quiet);
			ok = 0;
			}
			}
			*/
			if(v_macip(f[0], quiet, 0, nvram.lan_ipaddr, nvram.lan_netmask)) {
				if(this.existIP(f[0].value)) {
					ferror.set(f[0], 'duplicate IP or MAC address', quiet);
					ok = 0;
				}
			}

			if( this.checkRate(f[1].value)) {
				ferror.set(f[1], 'DLRate must between 1 and 99999', quiet);
				ok = 0;
			}

			if( this.checkRate(f[2].value)) {
				ferror.set(f[2], 'DLCeil must between 1 and 99999', quiet);
				ok = 0;
			}

			if( this.checkRateCeil(f[1].value, f[2].value)) {
				ferror.set(f[2], 'DLCeil must be greater than DLRate', quiet);
				ok = 0;
			}

			if( this.checkRate(f[3].value)) {
				ferror.set(f[3], 'ULRate must between 1 and 99999', quiet);
				ok = 0;
			}

			if( this.checkRate(f[4].value)) {
				ferror.set(f[4], 'ULCeil must between 1 and 99999', quiet);
				ok = 0;
			}

			if( this.checkRateCeil(f[3].value, f[4].value)) {
				ferror.set(f[4], 'ULCeil must be greater than ULRate', quiet);
				ok = 0;
			}

			return ok;
		}

		function verifyFields(focused, quiet)
		{
			var a = !E('_f_new_qoslimit_enable').checked;
			var b = !E('_f_qosl_enable').checked;
			var b1 = !E('_f_limit_br1_enable').checked;
			var b2 = !E('_f_limit_br2_enable').checked;
			var b3 = !E('_f_limit_br3_enable').checked;

			E('_qos_ibw').disabled = a;
			E('_qos_obw').disabled = a;
			E('_f_qosl_enable').disabled = a;
			E('_f_limit_br1_enable').disabled = a;
			E('_f_limit_br2_enable').disabled = a;
			E('_f_limit_br3_enable').disabled = a;

			E('_qosl_dlr').disabled = b || a;
			E('_qosl_dlc').disabled = b || a;
			E('_qosl_ulr').disabled = b || a;
			E('_qosl_ulc').disabled = b || a;
			E('_qosl_tcp').disabled = b || a;
			E('_qosl_udp').disabled = b || a;

			elem.display(PR('_qos_ibw'), PR('_qos_obw'), !a);
			elem.display(PR('_qosl_dlr'), PR('_qosl_dlc'), PR('_qosl_ulr'), PR('_qosl_ulc'), PR('_qosl_tcp'), PR('_qosl_udp'), !a && !b);

			E('_limit_br1_dlr').disabled = b1 || a;
			E('_limit_br1_dlc').disabled = b1 || a;
			E('_limit_br1_ulr').disabled = b1 || a;
			E('_limit_br1_ulc').disabled = b1 || a;
			E('_limit_br1_prio').disabled = b1 || a;
			elem.display(PR('_limit_br1_dlr'), PR('_limit_br1_dlc'), PR('_limit_br1_ulr'), PR('_limit_br1_ulc'), PR('_limit_br1_prio'), !a && !b1);

			E('_limit_br2_dlr').disabled = b2 || a;
			E('_limit_br2_dlc').disabled = b2 || a;
			E('_limit_br2_ulr').disabled = b2 || a;
			E('_limit_br2_ulc').disabled = b2 || a;
			E('_limit_br2_prio').disabled = b2 || a;
			elem.display(PR('_limit_br2_dlr'), PR('_limit_br2_dlc'), PR('_limit_br2_ulr'), PR('_limit_br2_ulc'), PR('_limit_br2_prio'), !a && !b2);

			E('_limit_br3_dlr').disabled = b3 || a;
			E('_limit_br3_dlc').disabled = b3 || a;
			E('_limit_br3_ulr').disabled = b3 || a;
			E('_limit_br3_ulc').disabled = b3 || a;
			E('_limit_br3_prio').disabled = b3 || a;
			elem.display(PR('_limit_br3_dlr'), PR('_limit_br3_dlc'), PR('_limit_br3_ulr'), PR('_limit_br3_ulc'), PR('_limit_br3_prio'), !a && !b3);

			return 1;
		}

		function save()
		{
			if (qosg.isEditing()) return;

			var data = qosg.getAllData();
			var qoslimitrules = '';
			var i;

			if (data.length != 0) qoslimitrules += data[0].join('<');
			for (i = 1; i < data.length; ++i) {
				qoslimitrules += '>' + data[i].join('<');
			}

			var fom = E('_fom');
			fom.new_qoslimit_enable.value = E('_f_new_qoslimit_enable').checked ? 1 : 0;
			fom.qosl_enable.value = E('_f_qosl_enable').checked ? 1 : 0;
			fom.limit_br1_enable.value = E('_f_limit_br1_enable').checked ? 1 : 0;
			fom.limit_br2_enable.value = E('_f_limit_br2_enable').checked ? 1 : 0;
			fom.limit_br3_enable.value = E('_f_limit_br3_enable').checked ? 1 : 0;
			fom.new_qoslimit_rules.value = qoslimitrules;
			form.submit(fom, 1);
		}

		function init()
		{
			qosg.recolor();
		}

	</script>

	<script type="text/javascript">
		if (nvram.qos_enable != '1') {
			$('.container .ajaxwrap').prepend('<div class="alert alert-warning"><b>QoS is disabled.</b>&nbsp; <a class="ajaxload" href="#qos-settings.asp">Enable &raquo;</a> <a class="close"><i class="icon-cancel"></i></a></div>');
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#qos-qoslimit.asp">
		<input type="hidden" name="_nextwait" value="10">
		<input type="hidden" name="_service" value="qoslimit-restart">

		<input type="hidden" name="new_qoslimit_enable">
		<input type="hidden" name="new_qoslimit_rules">
		<input type="hidden" name="qosl_enable">
		<input type="hidden" name="limit_br1_enable">
		<input type="hidden" name="limit_br2_enable">
		<input type="hidden" name="limit_br3_enable">

		<div class="box" data-box="qos-lan-limit">
			<div class="heading">Bandwidth Limiter for LAN (br0)</div>
			<div class="content">
				<div class="br0-set"></div><hr>
				<script type="text/javascript">
					$('.br0-set').forms([
						{ title: 'Enable Limiter', name: 'f_new_qoslimit_enable', type: 'checkbox', value: nvram.new_qoslimit_enable != '0' },
						{ title: 'Max Available Download <br><small>(same as used in QoS)</small>', indent: 2, name: 'qos_ibw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qos_ibw },
						{ title: 'Max Available Upload <br><small>(same as used in QoS)</small>', indent: 2, name: 'qos_obw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qos_obw }
					]);
				</script>

				<table class="line-table" id="qosg-grid"></table><br /><hr>

				<h4>Notes</h4>
				<ul>
					<li><b>IP Address / IP Range:</b>
					<li>Example: 192.168.1.5 for one IP.
					<li>Example: 192.168.1.4-7 for IP 192.168.1.4 to 192.168.1.7
					<li>Example: 4-7 for IP Range .4 to .7
					<li><b>The IP Range devices will share the Bandwidth</b>
					<li><b>MAC Address</b> Example: 00:2E:3C:6A:22:D8
				</ul>
			</div>
		</div>

		<div class="box" id="qoslimitbr0" data-box="qos-br0-limit">
			<div class="heading">Default Class for unlisted MAC / IP's in LAN (br0)</div>
			<div class="content">
				<div id="unlistedmac"></div><hr>
				<script type="text/javascript">
					$('#unlistedmac').forms([
						{ title: 'Enable', name: 'f_qosl_enable', type: 'checkbox', value: nvram.qosl_enable == '1'},
						{ title: 'Download rate', indent: 2, name: 'qosl_dlr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_dlr },
						{ title: 'Download ceil', indent: 2, name: 'qosl_dlc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_dlc },
						{ title: 'Upload rate', indent: 2, name: 'qosl_ulr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_ulr },
						{ title: 'Upload ceil', indent: 2, name: 'qosl_ulc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.qosl_ulc },
						{ title: 'TCP Limit', indent: 2, name: 'qosl_tcp', type: 'select', options:
							[['0', 'no limit'],
								['1', '1'],
								['2', '2'],
								['5', '5'],
								['10', '10'],
								['20', '20'],
								['50', '50'],
								['100', '100'],
								['200', '200'],
								['500', '500'],
								['1000', '1000']], value: nvram.qosl_tcp },
						{ title: 'UDP limit', indent: 2, name: 'qosl_udp', type: 'select', options:
							[['0', 'no limit'],
								['1', '1/s'],
								['2', '2/s'],
								['5', '5/s'],
								['10', '10/s'],
								['20', '20/s'],
								['50', '50/s'],
								['100', '100/s']], value: nvram.qosl_udp }
					]);
				</script>

				<ul>
					<li>Default Class - IP / MAC's non included in the list will take the Default Rate/Ceiling setting</li>
					<li>The bandwitdh will be shared by all unlisted hosts in br0</li>
				</ul>
			</div>
		</div>

		<div class="box" id="qoslimitbr1" data-box="qos-limit-br1">
			<div class="heading">Default Class for LAN1 (br1)</div>
			<div class="content">
				<div class="table-set1"></div><hr>
				<script type="text/javascript">
					$('.table-set1').forms([
						{ title: 'Enable', name: 'f_limit_br1_enable', type: 'checkbox', value: nvram.limit_br1_enable == '1'},
						{ title: 'Download rate', indent: 2, name: 'limit_br1_dlr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br1_dlr },
						{ title: 'Download ceil', indent: 2, name: 'limit_br1_dlc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br1_dlc },
						{ title: 'Upload rate', indent: 2, name: 'limit_br1_ulr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br1_ulr },
						{ title: 'Upload ceil', indent: 2, name: 'limit_br1_ulc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br1_ulc },
						{ title: 'Priority', indent: 2, name: 'limit_br1_prio', type: 'select', options:
							[['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']], value: nvram.limit_br1_prio }
					]);
				</script>

				<ul>
					<li>The bandwitdh will be shared by all hosts in br1.</li>
				</ul>
			</div>
		</div>

		<div class="box" id="qoslimitbr2" data-box="qos-limit-br2">
			<div class="heading">Default Class for LAN2 (br2)</div>
			<div class="content">
				<div class="table-set2"></div><hr>
				<script type="text/javascript">
					$('.table-set2').forms([
						{ title: 'Enable', name: 'f_limit_br2_enable', type: 'checkbox', value: nvram.limit_br2_enable == '1'},
						{ title: 'Download rate', indent: 2, name: 'limit_br2_dlr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br2_dlr },
						{ title: 'Download ceil', indent: 2, name: 'limit_br2_dlc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br2_dlc },
						{ title: 'Upload rate', indent: 2, name: 'limit_br2_ulr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br2_ulr },
						{ title: 'Upload ceil', indent: 2, name: 'limit_br2_ulc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br2_ulc },
						{ title: 'Priority', indent: 2, name: 'limit_br2_prio', type: 'select', options:
							[['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']], value: nvram.limit_br2_prio }
					]);
				</script>

				<ul>
					<li>The bandwitdh will be shared by all hosts in br2.</li>
				</ul>
			</div>
		</div>

		<div class="box" id="qoslimitbr3" data-box="qos-limit-br3">
			<div class="heading">Default Class for LAN3 (br3)</div>
			<div class="content">
				<div class="table-set3"></div><hr>
				<script type="text/javascript">
					$('.table-set3').forms([
						{ title: 'Enable', name: 'f_limit_br3_enable', type: 'checkbox', value: nvram.limit_br3_enable == '1'},
						{ title: 'Download rate', indent: 2, name: 'limit_br3_dlr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br3_dlr },
						{ title: 'Download ceil', indent: 2, name: 'limit_br3_dlc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br3_dlc },
						{ title: 'Upload rate', indent: 2, name: 'limit_br3_ulr', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br3_ulr },
						{ title: 'Upload ceil', indent: 2, name: 'limit_br3_ulc', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s</small>', value: nvram.limit_br3_ulc },
						{ title: 'Priority', indent: 2, name: 'limit_br3_prio', type: 'select', options:
							[['0','Highest'],['1','High'],['2','Normal'],['3','Low'],['4','Lowest']], value: nvram.limit_br3_prio }
					]);
				</script>

				<ul>
					<li>The bandwitdh will be shared by all hosts in br3.</li>
				</ul>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert info" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">qosg.setup(); init(); verifyFields(null, 1);</script>
</content>