<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Basic Settings</title>
<content>
	<script type="text/javascript">

		/* REMOVE-BEGIN
		!!TB - added qos_pfifo
		REMOVE-END */
		//	<% nvram("at_update,tomatoanon_answer,qos_classnames,qos_enable,qos_ack,qos_syn,qos_fin,qos_rst,qos_icmp,qos_udp,qos_default,qos_pfifo,qos_obw,qos_ibw,qos_orates,qos_irates,qos_reset,ne_vegas,ne_valpha,ne_vbeta,ne_vgamma,atm_overhead"); %>

		var classNames = nvram.qos_classnames.split(' ');		// Toastman - configurable class names

		pctListin = [[0, 'No Limit']];
		for (i = 1; i <= 100; ++i) pctListin.push([i, i + '%']);

		pctListout = [[0, 'No Limit']];
		for (i = 1; i <= 100; ++i) pctListout.push([i, i + '%']);

		function scale(bandwidth, rate, ceil)
		{
			if (bandwidth <= 0) return '';
			if (rate <= 0) return '';

			var s = comma(MAX(Math.floor((bandwidth * rate) / 100), 1));
			if (ceil > 0) s += ' - ' + MAX(Math.round((bandwidth * ceil) / 100), 1);
			return s + ' <small>kbit/s</small>';
		}

		function toggleFiltersVisibility(){
			if(E('qosclassnames').style.display=='')
				E('qosclassnames').style.display='none';
			else
				E('qosclassnames').style.display='';
		}

		function verifyClassCeilingAndRate(bandwidthString, rateString, ceilingString, resultsFieldName)
		{
			if (parseInt(ceilingString) >= parseInt(rateString))
			{
				elem.setInnerHTML(
					resultsFieldName,
					scale(
						bandwidthString,
						rateString,
						ceilingString));
			}
			else
			{
				elem.setInnerHTML(
					resultsFieldName,
					'Ceiling must be greater than or equal to rate.');

				return 0;
			}

			return 1;
		}

		function verifyFields(focused, quiet)
		{
			var i, e, b, f;

			if (!v_range('_qos_obw', quiet, 10, 999999)) return 0;
			for (i = 0; i < 10; ++i)
			{
				if (!verifyClassCeilingAndRate(
					E('_qos_obw').value,
					E('_f_orate_' + i).value,
					E('_f_oceil_' + i).value,
					'_okbps_' + i))
				{
					return 0;
				}
			}

			if (!v_range('_qos_ibw', quiet, 10, 999999)) return 0;
			for (i = 0; i < 10; ++i)
			{
				if (!verifyClassCeilingAndRate(
					E('_qos_ibw').value,
					E('_f_irate_' + i).value,
					E('_f_iceil_' + i).value,
					'_ikbps_' + i))
				{
					return 0;
				}
			}

			f = E('_fom').elements;
			b = !E('_f_qos_enable').checked;
			for (i = 0; i < f.length; ++i) {
				if ((f[i].name.substr(0, 1) != '_') && (f[i].type != 'button' && f[i].type != 'fieldset') && (f[i].name.indexOf('enable') == -1) &&
					(f[i].name.indexOf('ne_v') == -1)) f[i].disabled = b;
			}

			var abg = ['alpha', 'beta', 'gamma'];
			b = E('_f_ne_vegas').checked;
			for (i = 0; i < 3; ++i) {
				f = E('_ne_v' + abg[i]);
				f.disabled = !b;
				if (b) {
					if (!v_range(f, quiet, 0, 65535)) return 0;
				}
			}

			return 1;
		}

		function save()
		{
			var fom = E('_fom');
			var i, a, qos, c;


			fom.qos_enable.value = E('_f_qos_enable').checked ? 1 : 0;
			fom.qos_ack.value = E('_f_qos_ack').checked ? 1 : 0;
			fom.qos_syn.value = E('_f_qos_syn').checked ? 1 : 0;
			fom.qos_fin.value = E('_f_qos_fin').checked ? 1 : 0;
			fom.qos_rst.value = E('_f_qos_rst').checked ? 1 : 0;
			fom.qos_icmp.value = E('_f_qos_icmp').checked ? 1 : 0;
			fom.qos_udp.value = E('_f_qos_udp').checked ? 1 : 0;
			fom.qos_reset.value = E('_f_qos_reset').checked ? 1 : 0;

			qos = [];
			for (i = 1; i < 11; ++i) {
				qos.push(E('_f_qos_' + (i - 1)).value);
			}

			fom = E('_fom');
			fom.qos_classnames.value = qos.join(' ');

			a = [];
			for (i = 0; i < 10; ++i) {
				a.push(E('_f_orate_' + i).value + '-' + E('_f_oceil_' + i).value);
			}
			fom.qos_orates.value = a.join(',');

			a = [];

			for (i = 0; i < 10; ++i)
			{
				//a.push(E('_f_iceil_' + i).value);
				a.push(E('_f_irate_' + i).value + '-' + E('_f_iceil_' + i).value);
			}

			fom.qos_irates.value = a.join(',');

			fom.ne_vegas.value = E('_f_ne_vegas').checked ? 1 : 0;

			form.submit(fom, 1);
		}

	</script>


	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#qos-settings.asp">
		<input type="hidden" name="_service" value="qos-restart">

		<input type="hidden" name="qos_classnames">
		<input type="hidden" name="qos_enable">
		<input type="hidden" name="qos_ack">
		<input type="hidden" name="qos_syn">
		<input type="hidden" name="qos_fin">
		<input type="hidden" name="qos_rst">
		<input type="hidden" name="qos_icmp">
		<input type="hidden" name="qos_udp">
		<input type="hidden" name="qos_orates">
		<input type="hidden" name="qos_irates">
		<input type="hidden" name="qos_reset">
		<input type="hidden" name="ne_vegas">

		<div class="box" data-box="qos-basic-set">
			<div class="heading">Basic QOS Settings</div>
			<div class="content qos-settings"></div>
			<script type="text/javascript">

				classList = [];
				for (i = 0; i < 10; ++i) {
					classList.push([i, classNames[i]]);
				}

				$('.qos-settings').forms([
					{ title: 'Enable QoS', name: 'f_qos_enable', type: 'checkbox', value: nvram.qos_enable == '1' },
					{ title: 'Prioritize small packets with these control flags', multi: [
						{ suffix: ' ACK &nbsp;', name: 'f_qos_ack', type: 'checkbox', value: nvram.qos_ack == '1' },
						{ suffix: ' SYN &nbsp;', name: 'f_qos_syn', type: 'checkbox', value: nvram.qos_syn == '1' },
						{ suffix: ' FIN &nbsp;', name: 'f_qos_fin', type: 'checkbox', value: nvram.qos_fin == '1' },
						{ suffix: ' RST &nbsp;', name: 'f_qos_rst', type: 'checkbox', value: nvram.qos_rst == '1' }
					] },
					{ title: 'Prioritize ICMP', name: 'f_qos_icmp', type: 'checkbox', value: nvram.qos_icmp == '1' },
					{ title: 'No Ingress QOS for UDP', name: 'f_qos_udp', type: 'checkbox', value: nvram.qos_udp == '1' },
					{ title: 'Reset class when changing settings', name: 'f_qos_reset', type: 'checkbox', value: nvram.qos_reset == '1' },
					{ title: 'Default class', name: 'qos_default', type: 'select', options: classList, value: nvram.qos_default },
					/* REMOVE-BEGIN
					!!TB - added qos_pfifo
					REMOVE-END */
					{ title: 'Qdisc Scheduler', name: 'qos_pfifo', type: 'select', options: [['0','sfq'],['1','pfifo']], value: nvram.qos_pfifo }
				]);
			</script>
		</div>

		<div class="box" data-box="qos-dsl-set">
			<div class="heading">Settings for DSL only</div>
			<div class="content qos-dsl"></div>
			<script type="text/javascript">
				$('.qos-dsl').forms([
					{ title: 'DSL Overhead Value - ATM Encapsulation Type', multi:[
						{name: 'atm_overhead', type: 'select', options: [['0','None'],['32','32-PPPoE VC-Mux'],['40','40-PPPoE LLC/Snap'],
							['10','10-PPPoA VC-Mux'],['14','14-PPPoA LLC/Snap'],
							['8','8-RFC2684/RFC1483 Routed VC-Mux'],['16','16-RFC2684/RFC1483 Routed LLC/Snap'],
							['24','24-RFC2684/RFC1483 Bridged VC-Mux'],
							['32','32-RFC2684/RFC1483 Bridged LLC/Snap']], value:nvram.atm_overhead }
					] }
				]);
			</script>
		</div>

		<div class="box" data-box="qos-out-limits">
			<div class="heading">Outbound Rates / Limits</div>
			<div class="content out-limit"></div>
			<script type="text/javascript">
				cc = nvram.qos_orates.split(/[,-]/);
				f = [];
				f.push({ title: 'Max Bandwidth Limit', name: 'qos_obw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s   (Set to measured bandwidth less 15-30%)</small>', value: nvram.qos_obw });
				j = 0;
				for (i = 0; i < 10; ++i) {
					x = cc[j++] || 1;
					y = cc[j++] || 1;
					f.push({ title: classNames[i], multi: [
						{ name: 'f_orate_' + i, type: 'select', options: pctListout, value: x, suffix: ' ' },
						{ name:	'f_oceil_' + i, type: 'select', options: pctListout, value: y },
						{ type: 'custom', custom: ' &nbsp; <span id="_okbps_' + i + '"></span>' } ]
					});
				}
				$('.out-limit').forms(f);
			</script>
		</div>

		<div class="box" data-box="qos-in-limits">
			<div class="heading">Inbound Rates / Limits</div>
			<div class="content in-limit"></div>
			<script type="text/javascript">
				allRates = nvram.qos_irates.split(',');
				f = [];
				f.push({ title: 'Max Bandwidth Limit', name: 'qos_ibw', type: 'text', maxlen: 6, size: 8, suffix: ' <small>kbit/s   (Set to measured bandwidth less 15-30%)</small>', value: nvram.qos_ibw });

				for (i = 0; i < 10; ++i)
				{
					splitRate = allRates[i].split('-');
					incoming_rate = splitRate[0] || 1;
					incoming_ceil = splitRate[1] || 100;

					f.push(
						{
							title: classNames[i], multi: [
								{ name:	'f_irate_' + i, type: 'select', options: pctListin, value: incoming_rate, suffix: ' ' },
								{ name:	'f_iceil_' + i, type: 'select', options: pctListin, value: incoming_ceil },
								{ custom: ' &nbsp; <span id="_ikbps_' + i + '"></span>' } ]
					});
				}
				$('.in-limit').forms(f);
			</script>
		</div>

		<div class="box" data-box="qos-class-n">
			<div class="heading">QOS Class Names</div>
			<div class="content classnames" style="display:none"></div>
			<script type="text/javascript">

				if ((v = nvram.qos_classnames.match(/^(.+)\s+(.+)\s+(.+)\s+(.+)\s+(.+)\s+(.+)\s+(.+)\s+(.+)\s+(.+)\s+(.+)$/)) == null) {
					v = ["-","Highest","High","Medium","Low","Lowest","A","B","C","D","E"];
				}
				titles = ['-','Priority Class 1', 'Priority Class 2', 'Priority Class 3', 'Priority Class 4', 'Priority Class 5', 'Priority Class 6', 'Priority Class 7', 'Priority Class 8', 'Priority Class 9', 'Priority Class 10'];
				f = [{ title: ' ', text: '<small>(Maximum 10 characters, no spaces)</small>' }];
				for (i = 1; i < 11; ++i) {
					f.push({ title: titles[i], name: ('f_qos_' + (i - 1)),
						type: 'text', maxlen: 10, size: 15, value: v[i],
						suffix: '<span id="count' + i + '"></span>' });
				}
				$('.classnames').forms(f);
			</script>
		</div>

		<div class="box" data-box="qos-tcp-vegas">
			<div class="heading">TCP Vegas <small>(Network Congestion Control)</small></div>
			<div class="content tcp-vegas"></div>
			<script type="text/javascript">
				/* move me? */
				$('.tcp-vegas').forms([
					{ title: 'Enable TCP Vegas', name: 'f_ne_vegas', type: 'checkbox', value: nvram.ne_vegas == '1' },
					{ title: 'Alpha', name: 'ne_valpha', type: 'text', maxlen: 6, size: 8, value: nvram.ne_valpha },
					{ title: 'Beta', name: 'ne_vbeta', type: 'text', maxlen: 6, size: 8, value: nvram.ne_vbeta },
					{ title: 'Gamma', name: 'ne_vgamma', type: 'text', maxlen: 6, size: 8, value: nvram.ne_vgamma }
				]);
			</script>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		&nbsp; <span id="footer-msg" class="alert warning" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">verifyFields(null, 1);</script>
</content>