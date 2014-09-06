<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>IPv6 Configuration</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,ipv6_prefix,ipv6_prefix_length,ipv6_radvd,ipv6_accept_ra,ipv6_rtr_addr,ipv6_service,ipv6_dns,ipv6_tun_addr,ipv6_tun_addrlen,ipv6_ifname,ipv6_tun_v4end,ipv6_relay,ipv6_tun_mtu,ipv6_tun_ttl"); %>

		nvram.ipv6_accept_ra = fixInt(nvram.ipv6_accept_ra, 0, 3, 0);

		function verifyFields(focused, quiet)
		{
			var i;
			var ok = 1;
			var a, b, c;

			// --- visibility ---

			var vis = {
				_ipv6_service: 1,
				_f_ipv6_prefix: 1,
				_f_ipv6_prefix_length: 1,
				_f_ipv6_rtr_addr_auto: 1,
				_f_ipv6_rtr_addr: 1,
				_f_ipv6_dns_1: 1,
				_f_ipv6_dns_2: 1,
				_f_ipv6_dns_3: 1,
				_f_ipv6_radvd: 1,
				_f_ipv6_accept_ra_wan: 1,
				_f_ipv6_accept_ra_lan: 1,
				_ipv6_tun_v4end: 1,
				_ipv6_relay: 1,
				_ipv6_ifname: 1,
				_ipv6_tun_addr: 1,
				_ipv6_tun_addrlen: 1,
				_ipv6_tun_ttl: 1,
				_ipv6_tun_mtu: 1
			};

			c = E('_ipv6_service').value;
			switch(c) {
				case '':
					vis._ipv6_ifname = 0;
					vis._f_ipv6_rtr_addr_auto = 0;
					vis._f_ipv6_rtr_addr = 0;
					vis._f_ipv6_dns_1 = 0;
					vis._f_ipv6_dns_2 = 0;
					vis._f_ipv6_dns_3 = 0;
					vis._f_ipv6_radvd = 0;
					vis._f_ipv6_accept_ra_wan = 0;
					vis._f_ipv6_accept_ra_lan = 0;
				// fall through
				case 'other':
					vis._f_ipv6_prefix = 0;
					vis._f_ipv6_prefix_length = 0;
					vis._ipv6_tun_v4end = 0;
					vis._ipv6_relay = 0;
					vis._ipv6_tun_addr = 0;
					vis._ipv6_tun_addrlen = 0;
					vis._ipv6_tun_ttl = 0;
					vis._ipv6_tun_mtu = 0;
					if (c == 'other') {
						E('_f_ipv6_rtr_addr_auto').value = 1;
						vis._f_ipv6_rtr_addr_auto = 2;
					}
					break;
				case 'native-pd':
					vis._f_ipv6_prefix = 0;
					vis._f_ipv6_rtr_addr_auto = 0;
					vis._f_ipv6_rtr_addr = 0;
				// fall through
				case 'native':
					vis._ipv6_ifname = 0;
					vis._ipv6_tun_v4end = 0;
					vis._ipv6_relay = 0;
					vis._ipv6_tun_addr = 0;
					vis._ipv6_tun_addrlen = 0;
					vis._ipv6_tun_ttl = 0;
					vis._ipv6_tun_mtu = 0;
					break;
				case '6to4':
					vis._ipv6_ifname = 0;
					vis._f_ipv6_prefix = 0;
					vis._f_ipv6_rtr_addr_auto = 0;
					vis._f_ipv6_rtr_addr = 0;
					vis._ipv6_tun_v4end = 0;
					vis._ipv6_tun_addr = 0;
					vis._ipv6_tun_addrlen = 0;
					vis._f_ipv6_accept_ra_wan = 0;
					vis._f_ipv6_accept_ra_lan = 0;
					break;
				case 'sit':
					vis._ipv6_ifname = 0;
					vis._ipv6_relay = 0;
					vis._f_ipv6_accept_ra_wan = 0;
					vis._f_ipv6_accept_ra_lan = 0;
					break;
			}

			if (vis._f_ipv6_rtr_addr_auto && E('_f_ipv6_rtr_addr_auto').value == 0) {
				vis._f_ipv6_rtr_addr = 2;
			}

			if (E('_f_ipv6_radvd').checked) {
				if (vis._f_ipv6_accept_ra_lan) vis._f_ipv6_accept_ra_lan = 2;
				E('_f_ipv6_accept_ra_lan').checked = false;
			}

			for (a in vis) {
				b = E(a);
				c = vis[a];
				b.disabled = (c != 1);
				PR(b).style.display = c ? '' : 'none';
			}

			// --- verify ---

			if (vis._ipv6_ifname == 1) {
				if (E('_ipv6_service').value != 'other') {
					if (!v_length('_ipv6_ifname', quiet || !ok, 2)) ok = 0;
				}
				else ferror.clear('_ipv6_ifname');
			}

			/* REMOVE-BEGIN
			// Length
			a = [['_ipv6_ifname', 2]];
			for (i = a.length - 1; i >= 0; --i) {
			v = a[i];
			if ((vis[v[0]]) && (!v_length(v[0], quiet || !ok, v[1]))) ok = 0;
			}
			REMOVE-END */

			// IP address
			a = ['_ipv6_tun_v4end'];
			for (i = a.length - 1; i >= 0; --i)
			if ((vis[a[i]]) && (!v_ip(a[i], quiet || !ok))) ok = 0;

			// range
			a = [['_f_ipv6_prefix_length', 3, 64], ['_ipv6_tun_addrlen', 3, 127], ['_ipv6_tun_ttl', 0, 255], ['_ipv6_relay', 1, 254]];
			for (i = a.length - 1; i >= 0; --i) {
				b = a[i];
				if ((vis[b[0]]) && (!v_range(b[0], quiet || !ok, b[1], b[2]))) ok = 0;
			}

			// mtu
			b = '_ipv6_tun_mtu';
			if (vis[b]) {
				if ((!v_range(b, 1, 0, 0)) && (!v_range(b, quiet || !ok, 1280, 1480))) ok = 0;
				else ferror.clear(E(b));
			}

			// IPv6 prefix
			b = '_f_ipv6_prefix';
			c = vis._f_ipv6_accept_ra_wan && (E('_f_ipv6_accept_ra_wan').checked || E('_f_ipv6_accept_ra_lan').checked);
			if (vis[b] && (E(b).value.length > 0 || (!c))) {
				if (!v_ipv6_addr(b, quiet || !ok)) ok = 0;
			}
			else ferror.clear(b);

			// IPv6 address
			a = ['_ipv6_tun_addr'];
			for (i = a.length - 1; i >= 0; --i)
			if ((vis[a[i]]) && (!v_ipv6_addr(a[i], quiet || !ok))) ok = 0;

			if (vis._f_ipv6_rtr_addr == 2) {
				b = E('_f_ipv6_prefix');
				ip = (b.value.length > 0) ? ZeroIPv6PrefixBits(b.value, E('_f_ipv6_prefix_length').value) : '';
				b.value = CompressIPv6Address(ip);
				E('_f_ipv6_rtr_addr').value = (ip.length > 0) ? CompressIPv6Address(ip + '1') : '';
			}

			// optional IPv6 address
			a = ['_f_ipv6_rtr_addr', '_f_ipv6_dns_1', '_f_ipv6_dns_2', '_f_ipv6_dns_3'];
			for (i = a.length - 1; i >= 0; --i)
			if ((vis[a[i]]==1) && (E(a[i]).value.length > 0) && (!v_ipv6_addr(a[i], quiet || !ok))) ok = 0;

			return ok;
		}

		function earlyInit()
		{
			verifyFields(null, 1);
		}

		function joinIPv6Addr(a) {
			var r, i, s;

			r = [];
			for (i = 0; i < a.length; ++i) {
				s = CompressIPv6Address(a[i]);
				if ((s) && (s != '')) r.push(s);
			}
			return r.join(' ');
		}

		function save()
		{
			var a, b, c;
			var i;

			if (!verifyFields(null, false)) return;

			var fom = E('_fom');

			fom.ipv6_dns.value = joinIPv6Addr([fom.f_ipv6_dns_1.value, fom.f_ipv6_dns_2.value, fom.f_ipv6_dns_3.value]);
			fom.ipv6_radvd.value = fom.f_ipv6_radvd.checked ? 1 : 0;

			fom.ipv6_accept_ra.value = 0;
			if (fom.f_ipv6_accept_ra_wan.checked && !fom.f_ipv6_accept_ra_wan.disabled)
				fom.ipv6_accept_ra.value |= 1;
			if (fom.f_ipv6_accept_ra_lan.checked && !fom.f_ipv6_accept_ra_lan.disabled)
			fom.ipv6_accept_ra.value |= 2;

			fom.ipv6_prefix_length.value = fom.f_ipv6_prefix_length.value;
			fom.ipv6_prefix.value = fom.f_ipv6_prefix.value;

			switch(E('_ipv6_service').value) {
				case 'other':
					fom.ipv6_prefix_length.value = 64;
					fom.ipv6_prefix.value = '';
					fom.ipv6_rtr_addr.value = fom.f_ipv6_rtr_addr.value;
					break;
				case '6to4':
				case 'native-pd':
					fom.ipv6_prefix.value = '';
					fom.ipv6_rtr_addr.value = '';
					break;
				default:
					fom.ipv6_rtr_addr.disabled = fom.f_ipv6_rtr_addr_auto.disabled;
					if (fom.f_ipv6_rtr_addr_auto.value == 1)
						fom.ipv6_rtr_addr.value = fom.f_ipv6_rtr_addr.value;
					else
						fom.ipv6_rtr_addr.value = '';
					break;
			}

			form.submit(fom, 1);
		}

	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#basic-ipv6.asp">
		<input type="hidden" name="_nextwait" value="10">
		<input type="hidden" name="_service" value="*">

		<input type="hidden" name="ipv6_radvd">
		<input type="hidden" name="ipv6_dns">
		<input type="hidden" name="ipv6_prefix">
		<input type="hidden" name="ipv6_prefix_length">
		<input type="hidden" name="ipv6_rtr_addr">
		<input type="hidden" name="ipv6_accept_ra">

		<div class="box" id="section-ipv6">
			<div class="heading">Basic IPv6 Configuration</div>

			<div class="content">
				<script type="text/javascript">show_notice1('<% notice("ip6tables"); %>');</script>

				<div class="formfields"></div>
				<script type="text/javascript">
					dns = nvram.ipv6_dns.split(/\s+/);

					$('#section-ipv6 .formfields').forms([
						{ title: 'IPv6 Service Type', name: 'ipv6_service', type: 'select',
							options: [['', 'Disabled'],['native','Native IPv6 from ISP'],['native-pd','DHCPv6 with Prefix Delegation'],['6to4','6to4 Anycast Relay'],['sit','6in4 Static Tunnel'],['other','Other (Manual Configuration)']],
							value: nvram.ipv6_service },
						{ title: 'IPv6 WAN Interface', name: 'ipv6_ifname', type: 'text', maxlen: 8, size: 10, value: nvram.ipv6_ifname },
						{ title: 'Assigned / Routed Prefix', name: 'f_ipv6_prefix', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_prefix },
						{ title: 'Prefix Length', name: 'f_ipv6_prefix_length', type: 'text', maxlen: 3, size: 5, value: nvram.ipv6_prefix_length },
						{ title: 'Router IPv6 Address', multi: [
							{ name: 'f_ipv6_rtr_addr_auto', type: 'select', options: [['0', 'Default'],['1','Manual']], value: (nvram.ipv6_rtr_addr == '' ? '0' : '1') },
							{ name: 'f_ipv6_rtr_addr', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_rtr_addr }
						] },
						{ title: 'Static DNS', name: 'f_ipv6_dns_1', type: 'text', maxlen: 46, size: 48, value: dns[0] || '' },
						{ title: '',           name: 'f_ipv6_dns_2', type: 'text', maxlen: 46, size: 48, value: dns[1] || '' },
						{ title: '',           name: 'f_ipv6_dns_3', type: 'text', maxlen: 46, size: 48, value: dns[2] || '' },
						{ title: 'Enable Router Advertisements', name: 'f_ipv6_radvd', type: 'checkbox', value: nvram.ipv6_radvd == '1' },
						{ title: 'Accept RA from', multi: [
							{ suffix: '&nbsp; WAN &nbsp;&nbsp;&nbsp;', name: 'f_ipv6_accept_ra_wan', type: 'checkbox', value: (nvram.ipv6_accept_ra & 1) },
							{ suffix: '&nbsp; LAN &nbsp;',	name: 'f_ipv6_accept_ra_lan', type: 'checkbox', value: (nvram.ipv6_accept_ra & 2) }
						] },
						{ title: 'Tunnel Remote Endpoint (IPv4 Address)', name: 'ipv6_tun_v4end', type: 'text', maxlen: 15, size: 17, value: nvram.ipv6_tun_v4end },
						{ title: 'Relay Anycast Address', name: 'ipv6_relay', type: 'text', maxlen: 3, size: 5, prefix: '192.88.99.&nbsp&nbsp', value: nvram.ipv6_relay },
						{ title: 'Tunnel Client IPv6 Address', multi: [
							{ name: 'ipv6_tun_addr', type: 'text', maxlen: 46, size: 48, value: nvram.ipv6_tun_addr, suffix: ' / ' },
							{ name: 'ipv6_tun_addrlen', type: 'text', maxlen: 3, size: 5, value: nvram.ipv6_tun_addrlen }
						] },
						{ title: 'Tunnel MTU', name: 'ipv6_tun_mtu', type: 'text', maxlen: 4, size: 8, value: nvram.ipv6_tun_mtu, suffix: ' <small>(0 for default)</small>' },
						{ title: 'Tunnel TTL', name: 'ipv6_tun_ttl', type: 'text', maxlen: 3, size: 8, value: nvram.ipv6_tun_ttl }
					]);
				</script>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert info" style="visibility: hidden;"></span>
	</form>

	<script type="text/javascript">earlyInit()</script>
</content>