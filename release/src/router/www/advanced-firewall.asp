<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

Tomato VLAN GUI
Copyright (C) 2011 Augusto Bott
http://code.google.com/p/tomato-sdhc-vlan/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Firewall</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,block_wan,block_wan_limit,block_wan_limit_icmp,block_wan_limit_tr,nf_loopback,ne_syncookies,DSCP_fix_enable,multicast_pass,multicast_lan,multicast_lan1,multicast_lan2,multicast_lan3,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname,udpxy_enable,udpxy_stats,udpxy_clients,udpxy_port,ne_snat"); %>

		function verifyFields(focused, quiet)
		{
			/* ICMP */
			E('_f_icmp_limit').disabled = !E('_f_icmp').checked;
			E('_f_icmp_limit_icmp').disabled = (!E('_f_icmp').checked || !E('_f_icmp_limit').checked);
			E('_f_icmp_limit_traceroute').disabled = (!E('_f_icmp').checked || !E('_f_icmp_limit').checked);

			/* VLAN-BEGIN */
			var enable_mcast = E('_f_multicast').checked;
			E('_f_multicast_lan').disabled = ((!enable_mcast) || (nvram.lan_ifname.length < 1));
			E('_f_multicast_lan1').disabled = ((!enable_mcast) || (nvram.lan1_ifname.length < 1));
			E('_f_multicast_lan2').disabled = ((!enable_mcast) || (nvram.lan2_ifname.length < 1));
			E('_f_multicast_lan3').disabled = ((!enable_mcast) || (nvram.lan3_ifname.length < 1));
			if(nvram.lan_ifname.length < 1)
				E('_f_multicast_lan').checked = false;
			if(nvram.lan1_ifname.length < 1)
				E('_f_multicast_lan1').checked = false;
			if(nvram.lan2_ifname.length < 1)
				E('_f_multicast_lan2').checked = false;
			if(nvram.lan3_ifname.length < 1)
				E('_f_multicast_lan3').checked = false;
			if ((enable_mcast) && (!E('_f_multicast_lan').checked) && (!E('_f_multicast_lan1').checked) && (!E('_f_multicast_lan2').checked) && (!E('_f_multicast_lan3').checked)) {
				ferror.set('_f_multicast', 'IGMPproxy must be enabled in least one LAN bridge', quiet);
				return 0;
			} else {
				ferror.clear('_f_multicast');
			}
			/* VLAN-END */
			E('_f_udpxy_stats').disabled = !E('_f_udpxy_enable').checked;
			E('_f_udpxy_clients').disabled = !E('_f_udpxy_enable').checked;
			E('_f_udpxy_port').disabled = !E('_f_udpxy_enable').checked;
			return 1;
		}

		function save()
		{
			var fom;

			if (!verifyFields(null, 0)) return;

			fom = E('_fom');
			fom.block_wan.value = E('_f_icmp').checked ? 0 : 1;
			fom.block_wan_limit.value = E('_f_icmp_limit').checked? 1 : 0;
			fom.block_wan_limit_icmp.value = E('_f_icmp_limit_icmp').value;
			fom.block_wan_limit_tr.value = E('_f_icmp_limit_traceroute').value;

			fom.ne_syncookies.value = E('_f_syncookies').checked ? 1 : 0;
			fom.DSCP_fix_enable.value = E('_f_DSCP_fix_enable').checked ? 1 : 0;
			fom.multicast_pass.value = E('_f_multicast').checked ? 1 : 0;
			/* VLAN-BEGIN */
			fom.multicast_lan.value = E('_f_multicast_lan').checked ? 1 : 0;
			fom.multicast_lan1.value = E('_f_multicast_lan1').checked ? 1 : 0;
			fom.multicast_lan2.value = E('_f_multicast_lan2').checked ? 1 : 0;
			fom.multicast_lan3.value = E('_f_multicast_lan3').checked ? 1 : 0;
			/* VLAN-END */
			fom.udpxy_enable.value = E('_f_udpxy_enable').checked ? 1 : 0;
			fom.udpxy_stats.value = E('_f_udpxy_stats').checked ? 1 : 0;
			fom.udpxy_clients.value = E('_f_udpxy_clients').value;
			fom.udpxy_port.value = E('_f_udpxy_port').value;
			form.submit(fom, 1);
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#advanced-firewall.asp">
		<input type="hidden" name="_service" value="firewall-restart">

		<input type="hidden" name="block_wan">
		<input type="hidden" name="block_wan_limit">
		<input type="hidden" name="block_wan_limit_icmp">
		<input type="hidden" name="block_wan_limit_tr">
		<input type="hidden" name="ne_syncookies">
		<input type="hidden" name="DSCP_fix_enable">
		<input type="hidden" name="multicast_pass">

		<input type="hidden" name="multicast_lan">
		<input type="hidden" name="multicast_lan1">
		<input type="hidden" name="multicast_lan2">
		<input type="hidden" name="multicast_lan3">

		<input type="hidden" name="udpxy_enable">
		<input type="hidden" name="udpxy_stats">
		<input type="hidden" name="udpxy_clients">
		<input type="hidden" name="udpxy_port">

		<div class="box" data-box="firewal-set">
			<div class="heading">Firewall Settings</div>
			<div class="section firewall content"></div>
			<script type="text/javascript">
				$('.section.firewall').forms([
					{ title: 'Respond to ICMP ping', name: 'f_icmp', type: 'checkbox', value: nvram.block_wan == '0' },
					{ title: 'Limit PPS', indent: 2, name: 'f_icmp_limit', type: 'checkbox', value: nvram.block_wan_limit != '0' },
					{ title: 'ICMP', indent: 3, name: 'f_icmp_limit_icmp', type: 'text', maxlen: 3, size: 3, suffix: ' <small> request per second</small>', value: fixInt(nvram.block_wan_limit_icmp || 1, 1, 300, 5) },
					{ title: 'Traceroute', indent: 3, name: 'f_icmp_limit_traceroute', type: 'text', maxlen: 3, size: 3, suffix: ' <small> request per second</small>', value: fixInt(nvram.block_wan_limit_tr || 5, 1, 300, 5) },
					{ title: 'Enable SYN cookies', name: 'f_syncookies', type: 'checkbox', value: nvram.ne_syncookies != '0' },
					{ title: 'Enable DSCP Fix', name: 'f_DSCP_fix_enable', type: 'checkbox', value: nvram.DSCP_fix_enable != '0', suffix: ' <small>Fixes Comcast incorrect DSCP</small>' }
				]);
			</script>
		</div>

		<div class="box" data-box="firewall-nat">
			<div class="heading">NAT</div>
			<div class="section natfirewall content"></div>
			<script type="text/javascript">
				$('.section.natfirewall').forms([
					{ title: 'NAT loopback', name: 'nf_loopback', type: 'select', options: [[0,'All'],[1,'Forwarded Only'],[2,'Disabled']], value: fixInt(nvram.nf_loopback, 0, 2, 1) },
					{ title: 'NAT target', name: 'ne_snat', type: 'select', options: [[0,'MASQUERADE'],[1,'SNAT']], value: nvram.ne_snat }
				]);
			</script>
		</div>

		<div class="box" data-box="firewall-multicast">
			<div class="heading">Multicast</div>
			<div class="section multicast content"></div>
			<script type="text/javascript">
				$('.section.multicast').forms([
					{ title: 'Enable IGMPproxy', name: 'f_multicast', type: 'checkbox', value: nvram.multicast_pass == '1' },
					/* VLAN-BEGIN */
					{ title: 'LAN', indent: 2, name: 'f_multicast_lan', type: 'checkbox', value: (nvram.multicast_lan == '1') },
					{ title: 'LAN1', indent: 2, name: 'f_multicast_lan1', type: 'checkbox', value: (nvram.multicast_lan1 == '1') },
					{ title: 'LAN2', indent: 2, name: 'f_multicast_lan2', type: 'checkbox', value: (nvram.multicast_lan2 == '1') },
					{ title: 'LAN3', indent: 2, name: 'f_multicast_lan3', type: 'checkbox', value: (nvram.multicast_lan3 == '1') },
					/* VLAN-END */
					{ title: 'Enable Udpxy', name: 'f_udpxy_enable', type: 'checkbox', value: (nvram.udpxy_enable == '1') },
					{ title: 'Enable client statistics', indent: 2, name: 'f_udpxy_stats', type: 'checkbox', value: (nvram.udpxy_stats == '1') },
					{ title: 'Max clients', indent: 2, name: 'f_udpxy_clients', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.udpxy_clients || 3, 1, 5000, 3) },
					{ title: 'Udpxy port', indent: 2, name: 'f_udpxy_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.udpxy_port, 4022) }
				]);
			</script>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert info" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">verifyFields(null, 1);</script>
</content>