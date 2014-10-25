<!--
Tomato GUI
Copyright (C) 2006-2009 Jonathan Zarate
http://www.polarcloud.com/tomato/

Portions Copyright (C) 2010-2011 Jean-Yves Avenard, jean-yves@avenard.org

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>PPTP Client Configuration</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,pptp_client_enable,pptp_client_peerdns,pptp_client_mtuenable,pptp_client_mtu,pptp_client_mruenable,pptp_client_mru,pptp_client_nat,pptp_client_srvip,pptp_client_srvsub,pptp_client_srvsubmsk,pptp_client_username,pptp_client_passwd,pptp_client_mppeopt,pptp_client_crypt,pptp_client_custom,pptp_client_dfltroute,pptp_client_stateless"); %>

		pptpup = parseInt('<% psup("pptpclient"); %>');

		var changed = 0;

		function toggle(service, isup)
		{
			if (changed) {
				if (!confirm("Unsaved changes will be lost. Continue anyway?")) return;
			}
			$('#_' + service + '_button').html('<div class="spinner spinner-small"></div>');
			form.submitHidden('/service.cgi', {
				_redirect: '/#vpn-pptp.asp',
				_service: service + (isup ? '-stop' : '-start')
			});
		}

		function verifyFields(focused, quiet)
		{
			var ret = 1;

			elem.display(PR('_pptp_client_srvsub'), PR('_pptp_client_srvsubmsk'), !E('_f_pptp_client_dfltroute').checked);

			var f = E('_pptp_client_mtuenable').value == '0';
			if (f) {
				E('_pptp_client_mtu').value = '1450';
			}
			E('_pptp_client_mtu').disabled = f;
			f = E('_pptp_client_mruenable').value == '0';
			if (f) {
				E('_pptp_client_mru').value = '1450';
			}
			E('_pptp_client_mru').disabled = f;

			if (!v_range('_pptp_client_mtu', quiet, 576, 1500)) ret = 0;
			if (!v_range('_pptp_client_mru', quiet, 576, 1500)) ret = 0;
			if (!v_ip('_pptp_client_srvip', true) && !v_domain('_pptp_client_srvip', true)) { ferror.set(E('_pptp_client_srvip'), "Invalid server address.", quiet); ret = 0; }
			if (!E('_f_pptp_client_dfltroute').checked && !v_ip('_pptp_client_srvsub', true)) { ferror.set(E('_pptp_client_srvsub'), "Invalid subnet address.", quiet); ret = 0; }
			if (!E('_f_pptp_client_dfltroute').checked && !v_ip('_pptp_client_srvsubmsk', true)) { ferror.set(E('_pptp_client_srvsubmsk'), "Invalid netmask address.", quiet); ret = 0; }

			changed |= ret;
			return ret;
		}

		function save()
		{
			if (!verifyFields(null, false)) return;

			var fom = E('_fom');

			E('pptp_client_enable').value = E('_f_pptp_client_enable').checked ? 1 : 0;
			E('pptp_client_nat').value = E('_f_pptp_client_nat').checked ? 1 : 0;
			E('pptp_client_dfltroute').value = E('_f_pptp_client_dfltroute').checked ? 1 : 0;
			E('pptp_client_stateless').value = E('_f_pptp_client_stateless').checked ? 1 : 0;

			form.submit(fom, 1);

			changed = 0;
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#vpn-pptp.asp">
		<input type="hidden" name="_service" value="">
		<input type="hidden" name="_nextwait" value="5">

		<input type="hidden" id="pptp_client_enable" name="pptp_client_enable">
		<input type="hidden" id="pptp_client_peerdns" name="pptp_client_peerdns">
		<input type="hidden" id="pptp_client_nat" name="pptp_client_nat">
		<input type="hidden" id="pptp_client_dfltroute" name="pptp_client_dfltroute">
		<input type="hidden" id="pptp_client_stateless" name="pptp_client_stateless">

		<div class="box" id="pptp-client">
			<div class="heading">PPTP Client Configuration <span class="pptp-client-status"></span></div>
			<div class="content"></div>
			<script type="text/javascript">
				$('#pptp-client').forms([
					{ title: 'Start with WAN', name: 'f_pptp_client_enable', type: 'checkbox', value: nvram.pptp_client_enable != 0 },
					{ title: 'Server Address', name: 'pptp_client_srvip', type: 'text', size: 17, value: nvram.pptp_client_srvip },
					{ title: 'Username: ', name: 'pptp_client_username', type: 'text', maxlen: 50, size: 54, value: nvram.pptp_client_username },
					{ title: 'Password: ', name: 'pptp_client_passwd', type: 'password', maxlen: 50, size: 54, value: nvram.pptp_client_passwd },
					{ title: 'Encryption', name: 'pptp_client_crypt', type: 'select', value: nvram.pptp_client_crypt,
						options: [['0', 'Auto'],['1', 'None'],['2','Maximum (128 bit only)'],['3','Required (128 or 40 bit)']] },
					{ title: 'Stateless MPPE connection', name: 'f_pptp_client_stateless', type: 'checkbox', value: nvram.pptp_client_stateless != 0 },
					{ title: 'Accept DNS configuration', name: 'pptp_client_peerdns', type: 'select', options: [[0, 'Disabled'],[1, 'Yes'],[2, 'Exclusive']], value: nvram.pptp_client_peerdns },
					{ title: 'Redirect Internet traffic', name: 'f_pptp_client_dfltroute', type: 'checkbox', value: nvram.pptp_client_dfltroute != 0 },
					{ title: 'Remote subnet / netmask', multi: [
						{ name: 'pptp_client_srvsub', type: 'text', maxlen: 15, size: 17, value: nvram.pptp_client_srvsub },
						{ name: 'pptp_client_srvsubmsk', type: 'text', maxlen: 15, size: 17, prefix: ' /&nbsp', value: nvram.pptp_client_srvsubmsk } ] },
					{ title: 'Create NAT on tunnel', name: 'f_pptp_client_nat', type: 'checkbox', value: nvram.pptp_client_nat != 0 },
					{ title: 'MTU', multi: [
						{ name: 'pptp_client_mtuenable', type: 'select', options: [['0', 'Default'],['1','Manual']], value: nvram.pptp_client_mtuenable },
						{ name: 'pptp_client_mtu', type: 'text', maxlen: 4, size: 6, value: nvram.pptp_client_mtu } ] },
					{ title: 'MRU', multi: [
						{ name: 'pptp_client_mruenable', type: 'select', options: [['0', 'Default'],['1','Manual']], value: nvram.pptp_client_mruenable },
						{ name: 'pptp_client_mru', type: 'text', maxlen: 4, size: 6, value: nvram.pptp_client_mru } ] },
					{ title: 'Custom Configuration', name: 'pptp_client_custom', type: 'textarea', value: nvram.pptp_client_custom, style: "width: 100%; height: 80px;" }
				]);

				$('#pptp-client .pptp-client-status').html((!pptpup ? '<small style="color: red;">(Stopped)</small>' : '<small style="color: green;">(Running)</small>'));
				$('#pptp-client .pptp-client-status').after('<a href="#" data-toggle="tooltip" class="pull-right pptp-client-control" title="' +
					(pptpup ? 'Stop PPTP Client' : 'Start PPTP Client') + '" onclick="toggle(\'pptpclient\', pptpup); return false;" id="_pptpclient_button">' + (pptpup ? '<i class="icon-stop"></i>' : '<i class="icon-play"></i>') + '</a>');

			</script>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert success" style="visibility: hidden;"></span>
	</form>

	<script type="text/javascript">verifyFields(null, 1);</script>
</content>