<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>DHCP / DNS</title>
<content>
	<script type="text/javascript">

		//	<% nvram("dnsmasq_q,ipv6_radvd,dhcpd_dmdns,dns_addget,dhcpd_gwmode,dns_intcpt,dhcpd_slt,dhcpc_minpkt,dnsmasq_custom,dnsmasq_norw,dhcpd_lmax,dhcpc_custom,dns_norebind,dhcpd_static_only"); %>

		if ((isNaN(nvram.dhcpd_lmax)) || ((nvram.dhcpd_lmax *= 1) < 1)) nvram.dhcpd_lmax = 255;

		function verifyFields(focused, quiet)
		{
			var b = (E('_f_dhcpd_sltsel').value == 1);
			elem.display('_dhcpd_sltman', b);
			if ((b) && (!v_range('_f_dhcpd_slt', quiet, 1, 43200))) return 0;
			if (!v_length('_dnsmasq_custom', quiet, 0, 2048)) return 0;
			if (!v_range('_dhcpd_lmax', quiet, 1, 0xFFFF)) return 0;
			if (!v_length('_dhcpc_custom', quiet, 0, 80)) return 0;
			return 1;
		}

		function nval(a, b)
		{
			return (a == null || (a + '').trim() == '') ? b : a;
		}

		function save()
		{
			if (!verifyFields(null, false)) return;

			var a;
			var fom = E('_fom');

			fom.dhcpd_dmdns.value = E('_f_dhcpd_dmdns').checked ? 1 : 0;
			a = E('_f_dhcpd_sltsel').value;
			fom.dhcpd_slt.value = (a != 1) ? a : E('_f_dhcpd_slt').value;
			fom.dns_addget.value = E('_f_dns_addget').checked ? 1 : 0;
			fom.dns_norebind.value = E('_f_dns_norebind').checked ? 1 : 0;
			fom.dhcpd_gwmode.value = E('_f_dhcpd_gwmode').checked ? 1 : 0;
			fom.dns_intcpt.value = E('_f_dns_intcpt').checked ? 1 : 0;
			fom.dhcpc_minpkt.value = E('_f_dhcpc_minpkt').checked ? 1 : 0;
			fom.dhcpd_static_only.value = E('_f_dhcpd_static_only').checked ? '1' : '0';
			fom.ipv6_radvd.value = E('_f_ipv6_radvd').checked ? '1' : '0';

			fom.dnsmasq_q.value = 0;
			if (fom.f_dnsmasq_q4.checked) fom.dnsmasq_q.value |= 1;
			if (fom.f_dnsmasq_q6.checked) fom.dnsmasq_q.value |= 2;
			if (fom.f_dnsmasq_qr.checked) fom.dnsmasq_q.value |= 4;

			if (fom.dhcpc_minpkt.value != nvram.dhcpc_minpkt ||
				fom.dhcpc_custom.value != nvram.dhcpc_custom) {
				nvram.dhcpc_minpkt = fom.dhcpc_minpkt.value;
				nvram.dhcpc_custom = fom.dhcpc_custom.value;
				fom._service.value = '*';
			}
			else {
				fom._service.value = 'dnsmasq-restart';
			}


			if (fom.dns_intcpt.value != nvram.dns_intcpt) {
				nvram.dns_intcpt = fom.dns_intcpt.value;
				if (fom._service.value != '*') fom._service.value += ',firewall-restart';
			}

			/* IPV6-BEGIN */
			if (fom.dhcpd_dmdns.value != nvram.dhcpd_dmdns) {
				nvram.dhcpd_dmdns = fom.dhcpd_dmdns.value;
				if (fom._service.value != '*') fom._service.value += ',dnsmasq-restart';
			}
			/* IPV6-END */

			form.submit(fom, 1);
		}

		function toggleVisibility(whichone) {
			if(E('sesdiv' + whichone).style.display=='') {
				E('sesdiv' + whichone).style.display='none';
				E('sesdiv' + whichone + 'showhide').innerHTML='<i class="icon-chevron-up"></i>';
				cookie.set('adv_dhcpdns_' + whichone + '_vis', 0);
			} else {
				E('sesdiv' + whichone).style.display='';
				E('sesdiv' + whichone + 'showhide').innerHTML='<i class="icon-chevron-down"></i>';
				cookie.set('adv_dhcpdns_' + whichone + '_vis', 1);
			}
		}

		function init() {
			var c;
			if (((c = cookie.get('adv_dhcpdns_notes_vis')) != null) && (c == '1')) {
				toggleVisibility("notes");
			}
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#advanced-dhcpdns.asp">
		<input type="hidden" name="_service" value="">

		<input type="hidden" name="dhcpd_dmdns">
		<input type="hidden" name="dhcpd_slt">
		<input type="hidden" name="dns_addget">
		<input type="hidden" name="dns_norebind">
		<input type="hidden" name="dhcpd_gwmode">
		<input type="hidden" name="dns_intcpt">
		<input type="hidden" name="dhcpc_minpkt">
		<input type="hidden" name="dhcpd_static_only">
		<input type="hidden" name="ipv6_radvd">
		<input type="hidden" name="dnsmasq_q">

		<div class="box" data-box="dhcp-server">
			<div class="heading">DHCP / DNS Server (LAN)</div>
			<div class="section dnsdhcp content"></div>
			<script type="text/javascript">
				$('.section.dnsdhcp').forms([
					{ title: 'Use internal DNS', name: 'f_dhcpd_dmdns', type: 'checkbox', value: nvram.dhcpd_dmdns == '1' },
					{ title: 'Use received DNS with user-entered DNS', name: 'f_dns_addget', type: 'checkbox', value: nvram.dns_addget == '1' },
					{ title: 'Prevent DNS-rebind attacks', name: 'f_dns_norebind', type: 'checkbox', value: nvram.dns_norebind == '1' },
					{ title: 'Intercept DNS port<br>(UDP 53)', name: 'f_dns_intcpt', type: 'checkbox', value: nvram.dns_intcpt == '1' },
					{ title: 'Use user-entered gateway if WAN is disabled', name: 'f_dhcpd_gwmode', type: 'checkbox', value: nvram.dhcpd_gwmode == '1' },
					{ title: 'Ignore DHCP requests from unknown devices', name: 'f_dhcpd_static_only', type: 'checkbox', value: nvram.dhcpd_static_only == '1' },
					{ title: 'Maximum active DHCP leases', name: 'dhcpd_lmax', type: 'text', maxlen: 5, size: 8, value: nvram.dhcpd_lmax },
					{ title: 'Static lease time', multi: [
						{ name: 'f_dhcpd_sltsel', type: 'select', options: [[0,'Same as normal lease time'],[-1,'"Infinite"'],[1,'Custom']],
							value: (nvram.dhcpd_slt < 1) ? nvram.dhcpd_slt : 1 },
						{ name: 'f_dhcpd_slt', type: 'text', maxlen: 5, size: 8, prefix: '<span id="_dhcpd_sltman"> ', suffix: 'minutes</span>',
							value: (nvram.dhcpd_slt >= 1) ? nvram.dhcpd_slt : 3600 } ] },
					{ title: 'Announce IPv6 on LAN', name: 'f_ipv6_radvd', type: 'checkbox', value: nvram.ipv6_radvd == '1' },
					{ title: 'Mute dhcpv4 logging', name: 'f_dnsmasq_q4', type: 'checkbox', value: (nvram.dnsmasq_q & 1) },
					{ title: 'Mute dhcpv6 logging', name: 'f_dnsmasq_q6', type: 'checkbox', value: (nvram.dnsmasq_q & 2) },
					{ title: 'Mute RA logging', name: 'f_dnsmasq_qr', type: 'checkbox', value: (nvram.dnsmasq_q & 4) },
					{ title: '<a href="http://www.thekelleys.org.uk/" target="_new">Dnsmasq</a><br>Custom configuration', name: 'dnsmasq_custom', type: 'textarea', value: nvram.dnsmasq_custom,
						style: 'width: 100%; height:100px;' }
				]);
			</script>
		</div>

		<div class="box" data-box="dhcp-client-wan">
			<div class="heading">DHCP Client (WAN)</div>
			<div class="section dhcpwan content"></div>
			<script type="text/javascript">
				$('.dhcpwan').forms([
					{ title: 'DHCPC Options', name: 'dhcpc_custom', type: 'text', maxlen: 80, size: 34, value: nvram.dhcpc_custom },
					{ title: 'Reduce packet size', name: 'f_dhcpc_minpkt', type: 'checkbox', value: nvram.dhcpc_minpkt == '1' }
				]);
			</script>
		</div>

		<div class="box">
			<div class="heading">Notes <a class="pull-right" data-toggle="tooltip" title="Hide/Show Notes" href="javascript:toggleVisibility('notes');"><span id="sesdivnotesshowhide"><i class="icon-chevron-up"></i></span></a></div>
			<div class="section content" id="sesdivnotes" style="display:none">

				<i>DHCP / DNS Server (LAN):</i><br>
				<ul>
					<li><b>Use internal DNS</b> - Allow dnsmasq to be your DNS server on LAN.</li>
					<li><b>Use received DNS with user-entered DNS</b> - Add DNS servers received from your WAN connection to the static DNS server list (see <a href='basic-network.asp'>Network</a> configuration).</li>
					<li><b>Prevent DNS-rebind attacks</b> - Enable DNS rebinding protection on Dnsmasq.</li>
					<li><b>Intercept DNS port</b> - Any DNS requests/packets sent out to UDP port 53 are redirected to the internal DNS server.</li>
					<li><b>Use user-entered gateway if WAN is disabled</b> - DHCP will use the IP address of the router as the default gateway on each LAN.</li>
					<li><b>Ignore DHCP requests (...)</b> - Dnsmasq will ignore DHCP requests  to Only MAC addresses listed on the <a href='basic-static.asp'>Static DHCP/ARP</a> page won't be able to obtain an IP address through DHCP.</li>
					<li><b>Maximum active DHCP leases</b> - Self-explanatory.</li>
					<li><b>Static lease time</b> - Absolute maximum amount of time allowed for any DHCP lease to be valid.</li>
					<li><b>Custom configuration</b> - Extra options to be added to the Dnsmasq configuration file.</li>
				</ul>

				<i>DHCP Client (WAN):</i><br>
				<ul>
					<li><b>DHCPC Options</b> - Extra options for the DHCP client.</li>
					<li><b>Reduce packet size</b> - Self-explanatory.</li>
				</ul>

				<i>Other relevant notes/hints:</i><br>
				<ul>
					<li>The contents of file /etc/dnsmasq.custom are also added to the end of Dnsmasq's configuration file (if it exists).</li>
				</ul>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		&nbsp; <span id="footer-msg" class="alert warning" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">init(); verifyFields(null, true);</script>
</content>