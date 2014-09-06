<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>DMZ Forwarding</title>
<content>
	<script type="text/javascript" src="js/interfaces.js"></script>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,dmz_enable,dmz_ipaddr,dmz_sip,dmz_ifname,lan_ifname,lan1_ifname,lan2_ifname,lan3_ifname"); %>

		var lipp = '<% lipp(); %>.';

		function verifyFields(focused, quiet)
		{
			var sip, dip, off;

			off = !E('_f_dmz_enable').checked;

			dip = E('_f_dmz_ipaddr')
			dip.disabled = off;

			sip = E('_f_dmz_sip');
			sip.disabled = off;

			/* VLAN-BEGIN */
			var dif = E('_dmz_ifname');
			dif.disabled = off;
			if (dif.options[(dif.selectedIndex)].disabled) dif.selectedIndex = 0;
			/* VLAN-END */

			if (off) {
				ferror.clearAll(dip, sip);
				return 1;
			}

			if (dip.value.indexOf('.') == -1) dip.value = lipp + dip.value;
			if (!v_ip(dip)) return 0;

			if ((sip.value.length) && (!v_iptaddr(sip, quiet, 15))) return 0;
			ferror.clear(sip);

			return 1;
		}

		function save()
		{
			var fom;
			var en;
			var s;

			if (!verifyFields(null, false)) return;

			fom = E('_fom');
			en = fom.f_dmz_enable.checked;
			fom.dmz_enable.value = en ? 1 : 0;
			if (en) {
				// shorten it if possible to be more compatible with original
				s = fom.f_dmz_ipaddr.value;
				fom.dmz_ipaddr.value = (s.indexOf(lipp) == 0) ? s.replace(lipp, '') : s;
			}
			fom.dmz_sip.value = fom.f_dmz_sip.value.split(/\s*,\s*/).join(',');
			form.submit(fom, 0);
		}

		function init() {
			/* VLAN-BEGIN */
			var dif = E('_dmz_ifname');
			if(nvram.lan_ifname.length < 1)
				dif.options[0].disabled=true;
			if(nvram.lan1_ifname.length < 1)
				dif.options[1].disabled=true;
			if(nvram.lan2_ifname.length < 1)
				dif.options[2].disabled=true;
			if(nvram.lan3_ifname.length < 1)
				dif.options[3].disabled=true;
			if(nvram.dmz_enable == '1')
				verifyFields(null,true);
			/* VLAN-END */
		}

	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#forward-dmz.asp">
		<input type="hidden" name="_service" value="firewall-restart">

		<input type="hidden" name="dmz_enable">
		<input type="hidden" name="dmz_ipaddr">
		<input type="hidden" name="dmz_sip">

		<div class="box">
			<div class="heading">DMZ Settings</div>
			<div class="content dmz-settings"></div>
			<script type="text/javascript">
				$('.dmz-settings').forms([
					{ title: 'Enable DMZ', name: 'f_dmz_enable', type: 'checkbox', value: (nvram.dmz_enable == '1') },
					{ title: 'Destination Address', indent: 2, name: 'f_dmz_ipaddr', type: 'text', maxlen: 15, size: 17,
						value: (nvram.dmz_ipaddr.indexOf('.') != -1) ? nvram.dmz_ipaddr : (lipp + nvram.dmz_ipaddr) },
					/* VLAN-BEGIN */
					{ title: 'Destination Interface', indent: 2, name: 'dmz_ifname', type: 'select',
						options: [['br0','LAN (br0)'],['br1','LAN1  (br1)'],['br2','LAN2 (br2)'],['br3','LAN3 (br3)']], value: nvram.dmz_ifname },
					/* VLAN-END */
					{ title: 'Source Address Restriction', indent: 2, name: 'f_dmz_sip', type: 'text', maxlen: 512, size: 64,
						value: nvram.dmz_sip, suffix: '<br><small>(optional; ex: "1.1.1.1", "1.1.1.0/24", "1.1.1.1 - 2.2.2.2" or "me.example.com")</small>' }
					]);
			</script>
		</div>
		<script type="text/javascript">if (nvram.dmz_enable == '1') show_notice1('<% notice("iptables"); %>');</script>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert info" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">init(); verifyFields(null, 1);</script>
</content>