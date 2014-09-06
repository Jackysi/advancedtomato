<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Conntrack / Netfilter</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,ct_tcp_timeout,ct_udp_timeout,ct_timeout,ct_max,ct_hashsize,nf_l7in,nf_ttl,nf_sip,nf_rtsp,nf_pptp,nf_h323,nf_ftp"); %>

		var checker = null;
		var timer = new TomatoTimer(check);
		var running = 0;

		function check()
		{
			timer.stop();
			if ((checker) || (!running)) return;

			checker = new XmlHttp();
			checker.onCompleted = function(text, xml) {
				var conntrack, total, i;
				conntrack = null;
				total = 0;
				try {
					eval(text);
				}
				catch (ex) {
					conntrack = [];
				}
				for (i = 1; i < 13; ++i) {
					E('count' + i).innerHTML = '&nbsp; <small>('+ ((conntrack[i] || 0) * 1) + ' in this state)</small>';
				}
				E('count0').innerHTML = '(' + ((conntrack[0] || 0) * 1) + ' connections currently tracked)';
				checker = null;
				timer.start(3000);
			}
			checker.onError = function(x) {
				checker = null;
				timer.start(6000);
			}

			checker.post('/update.cgi', 'exec=ctcount&arg0=0');
		}

		function clicked()
		{
			running ^= 1;
			E('spin').style.visibility = running ? 'visible' : 'hidden';
			if (running) check();
		}


		var expireText;

		function expireTimer()
		{
			var e = E('expire');

			if (!expireText) expireText = e.value;

			if (--expireTime == 0) {
				e.disabled = false;
				e.value = expireText;
			}
			else {
				setTimeout(expireTimer, 1000);
				e.value = 'Expire Scheduled... ' + expireTime;
			}
		}

		function expireClicked()
		{
			expireTime = 18;
			E('expire').disabled = true;
			(new XmlHttp()).post('expct.cgi', '');
			expireTimer();
		}


		function verifyFields(focused, quiet)
		{
			var i, v;

			for (i = 1; i < 9; ++i) {
				if (!v_range('_f_tcp_' + i, quiet, 1, 432000)) return 0;
			}
			for (i = 0; i < 2; ++i) {
				if (!v_range('_f_udp_' + i, quiet, 1, 432000)) return 0;
			}
			for (i = 0; i < 2; ++i) {
				if (!v_range('_f_ct_' + i, quiet, 1, 432000)) return 0;
			}

			if (!v_range('_ct_max', quiet, 128, 300000)) return 0;

			/* LINUX26-BEGIN */
			if (!v_range('_ct_hashsize', quiet, 127, 65535)) return 0;
			/* LINUX26-END */

			v = (E('_f_nf_ttl').value == '');
			E('_f_ttl_val').style.display = v ? '' : 'none';
			if ((v) && !v_range('_f_ttl_val', quiet, 0, 255)) return 0;

			return 1;
		}

		function save()
		{
			var i, tcp, udp, ct, fom;

			if (!verifyFields(null, false)) return;

			tcp = [];
			tcp.push('0');
			for (i = 1; i < 9; ++i) {
				tcp.push(E('_f_tcp_' + i).value);
			}
			tcp.push('0');

			udp = [];
			for (i = 0; i < 2; ++i) {
				udp.push(E('_f_udp_' + i).value);
			}

			ct = [];
			for (i = 0; i < 2; ++i) {
				ct.push(E('_f_ct_' + i).value);
			}

			fom = E('_fom');
			fom.ct_tcp_timeout.value = tcp.join(' ');
			fom.ct_udp_timeout.value = udp.join(' ');
			fom.ct_timeout.value = ct.join(' ');
			fom.nf_l7in.value = E('_f_l7in').checked ? 1 : 0;
			/* LINUX26-BEGIN */
			fom.nf_sip.value = E('_f_sip').checked ? 1 : 0;
			/* LINUX26-END */
			fom.nf_rtsp.value = E('_f_rtsp').checked ? 1 : 0;
			fom.nf_pptp.value = E('_f_pptp').checked ? 1 : 0;
			fom.nf_h323.value = E('_f_h323').checked ? 1 : 0;
			fom.nf_ftp.value = E('_f_ftp').checked ? 1 : 0;

			i = E('_f_nf_ttl').value;
			if (i == '')
				fom.nf_ttl.value = 'c:' + E('_f_ttl_val').value;
			else
				fom.nf_ttl.value = i;

			form.submit(fom, 1);
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
	<input type="hidden" name="_nextpage" value="/#advanced-ctnf.asp">
	<input type="hidden" name="_service" value="ctnf-restart">
	<input type="hidden" name="ct_tcp_timeout" value="">
	<input type="hidden" name="ct_udp_timeout" value="">
	<input type="hidden" name="ct_timeout" value="">
	<input type="hidden" name="nf_l7in" value="">
	<input type="hidden" name="nf_ttl">
	<input type="hidden" name="nf_rtsp">
	<input type="hidden" name="nf_pptp">
	<input type="hidden" name="nf_h323">
	<input type="hidden" name="nf_ftp">
	<input type="hidden" name="nf_sip">

	<div class="box" data-box="ctnf-cons">
		<div class="heading">Connections</div>
		<div class="content">
			<div class="conectionsfirst"></div>
			<script type="text/javascript">
				$('.conectionsfirst').forms([
					{ title: 'Maximum Connections', name: 'ct_max', type: 'text', maxlen: 6, size: 8,
						suffix: '&nbsp; <a href="javascript:clicked()" id="count0">[ count current... ]</a> <div class="spinner" style="visibility:hidden" id="spin" onclick="clicked()"></div>',
						value: fixInt(nvram.ct_max || 4096, 128, 300000, 4096) }
					/* LINUX26-BEGIN */
					,{ title: 'Hash Table Size', name: 'ct_hashsize', type: 'text', maxlen: 6, size: 8, value: nvram.ct_hashsize || 1023 }
					/* LINUX26-END */
				], { grid: ['col-sm-2', 'col-sm-10'] });
			</script>
			<button type="button" value="Drop Idle" onclick="expireClicked()" id="expire" class="btn">Drop Idle <i class="icon-disable"></i></button>
		</div>
	</div>

	<div class="box" data-box="ctnf-tcp-time">
		<div class="heading">TCP Timeout</div>
		<div class="section tcptimeout content"></div>
		<script type="text/javascript">
			if ((v = nvram.ct_tcp_timeout.match(/^(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)$/)) == null) {
				v = [0,0,1200,120,60,120,120,10,60,30,0];
			}
			titles = ['-', 'None', 'Established', 'SYN Sent', 'SYN Received', 'FIN Wait', 'Time Wait', 'Close', 'Close Wait', 'Last ACK', 'Listen'];
			f = [{ title: ' ', text: '<small>(seconds)</small>' }];
			for (i = 1; i < 11; ++i) {
				f.push({ title: titles[i], name: ('f_tcp_' + (i - 1)),
					type: 'text', maxlen: 6, size: 8, value: v[i],
					hidden: (i == 1 || i == 10) ? 1 : 0,
					suffix: '<span id="count' + i + '"></span>' });
			}

			$('.content.tcptimeout').forms(f, { grid: ['col-sm-2', 'col-sm-10'] });
		</script>
	</div>

	<div class="box" data-box="ctnf-udp-time">
		<div class="heading">UDP Timeout</div>
		<div class="udptimeout content"></div>
		<script type="text/javascript">
			if ((v = nvram.ct_udp_timeout.match(/^(\d+)\s+(\d+)$/)) == null) {
				v = [0,30,180];
			}
			$('.content.udptimeout').forms([
				{ title: ' ', text: '<small>(seconds)</small>' },
				{ title: 'Unreplied', name: 'f_udp_0', type: 'text', maxlen: 6, size: 8, value: v[1], suffix: '<span id="count11"></span>' },
				{ title: 'Assured', name: 'f_udp_1', type: 'text', maxlen: 6, size: 8, value: v[2], suffix: '<span id="count12"></span>' }
			], { grid: ['col-sm-2', 'col-sm-10'] });
		</script>
	</div>

	<div class="box" data-box="ctnf-timeout">
		<div class="heading">Other Timeouts</div>
		<div class="otimeout content"></div>
		<script type="text/javascript">
			if ((v = nvram.ct_timeout.match(/^(\d+)\s+(\d+)$/)) == null) {
				v = [0,600,30];
			}

			$('.content.otimeout').forms([
				{ title: ' ', text: '<small>(seconds)</small>' },
				{ title: 'Generic', name: 'f_ct_0', type: 'text', maxlen: 6, size: 8, value: v[1] },
				{ title: 'ICMP', name: 'f_ct_1', type: 'text', maxlen: 6, size: 8, value: v[2] }
			], { grid: ['col-sm-2', 'col-sm-10'] });
		</script>
	</div>

	<div class="box" data-box="ctnf-nat">
		<div class="heading">Tracking / NAT Helpers</div>
		<div class="content helpers"></div>
		<script type="text/javascript">
			$('.content.helpers').forms([
				{ title: 'FTP', name: 'f_ftp', type: 'checkbox', value: nvram.nf_ftp != '0' },
				{ title: 'GRE / PPTP', name: 'f_pptp', type: 'checkbox', value: nvram.nf_pptp != '0' },
				{ title: 'H.323', name: 'f_h323', type: 'checkbox', value: nvram.nf_h323 != '0' },
				/* LINUX26-BEGIN */
				{ title: 'SIP', name: 'f_sip', type: 'checkbox', value: nvram.nf_sip != '0' },
				/* LINUX26-END */
				{ title: 'RTSP', name: 'f_rtsp', type: 'checkbox', value: nvram.nf_rtsp != '0' }
			], { grid: ['col-sm-2', 'col-sm-10'] });
		</script>
	</div>

	<div class="box" data-box="ctnf-misc"">
		<div class="heading">Miscellaneous</div>
		<div class="content misc"></div>
		<script type="text/javascript">
			v = [];
			for (i = -5; i <= 5; ++i) {
				v.push([i + '', i ? ((i > 0) ? '+' : '') + i : 'None']);
			}
			v.push(['', 'Custom']);

			$('.content.misc').forms([
				{ title: 'TTL Adjust', multi: [
					{ name: 'f_nf_ttl', type: 'select', options: v, value: nvram.nf_ttl.substr(0, 2) == 'c:' ? '' : nvram.nf_ttl },
					{ name: 'f_ttl_val', type: 'text', maxlen: 3, size: 6, value: nvram.nf_ttl.substr(0, 2) == 'c:' ?  nvram.nf_ttl.substr(2, 5) : '' }
				] },
				{ title: 'Inbound Layer 7', name: 'f_l7in', type: 'checkbox', value: nvram.nf_l7in != '0' }
			], { grid: ['col-sm-2', 'col-sm-10'] });
		</script>

	</div>

	<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
	<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
	<span id="footer-msg" class="alert info" style="visibility: hidden;"></span>

	<script type="text/javascript">verifyFields(null, 1);</script>
</content>