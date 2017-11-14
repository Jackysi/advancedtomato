<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Basic Network Settings</title>
<content>
	<style type="text/css">
		#lan-grid .co1,
		#lan-grid .co2,
		#lan-grid .co3,
		#lan-grid .co4,
		#lan-grid .co5,
		#lan-grid .co6,
		#lan-grid .co7 {
			text-align: center;
		}

		#lan-grid .centered {
			text-align: center;
		}

		#spin {
			visibility: hidden;
			vertical-align: middle;
		}

	</style>

	<script type="text/javascript" src="js/md5.js"></script>
	<script type="text/javascript" src="js/wireless.jsx?_http_id=<% nv(http_id); %>"></script>
	<script type="text/javascript" src="js/interfaces.js"></script>
	<script type='text/javascript'>
		//<% nvram("dhcp_lease,dhcp_num,dhcp_start,dhcpd_startip,dhcpd_endip,lan_gateway,lan_ipaddr,lan_netmask,lan_proto,lan_state,lan_desc,lan_invert,wl_security_mode,dnscrypt_proxy,dnscrypt_priority,dnscrypt_port,dnscrypt_resolver,dnscrypt_log,dnscrypt_manual,dnscrypt_provider_name,dnscrypt_provider_key,dnscrypt_resolver_address,wan_wins,wl_wds_enable,wl_channel,wl_closed,wl_crypto,wl_key,wl_key1,wl_key2,wl_key3,wl_key4,wl_lazywds,wl_mode,wl_net_mode,wl_passphrase,wl_radio,wl_radius_ipaddr,wl_radius_port,wl_ssid,wl_wds,wl_wep_bit,wl_wpa_gtk_rekey,wl_wpa_psk,wl_radius_key,wl_auth,wl_hwaddr,t_features,wl_nbw_cap,wl_nctrlsb,wl_nband,wl_phytype,lan_ifname,lan_stp,lan1_ifname,lan1_ipaddr,lan1_netmask,lan1_proto,lan1_stp,dhcp1_start,dhcp1_num,dhcp1_lease,dhcpd1_startip,dhcpd1_endip,lan2_ifname,lan2_ipaddr,lan2_netmask,lan2_proto,lan2_stp,dhcp2_start,dhcp2_num,dhcp2_lease,dhcpd2_startip,dhcpd2_endip,lan3_ifname,lan3_ipaddr,lan3_netmask,lan3_proto,lan3_stp,dhcp3_start,dhcp3_num,dhcp3_lease,dhcpd3_startip,dhcpd3_endip,cstats_enable,wan_proto,wan_weight,wan_modem_pin,wan_modem_dev,wan_modem_init,wan_modem_apn,wan_modem_speed,wan_ppp_username,wan_ppp_passwd,wan_ppp_service,wan_l2tp_server_ip,wan_pptp_dhcp,wan_ipaddr,wan_netmask,wan_gateway,wan_pptp_server_ip,wan_ppp_custom,wan_ppp_demand,wan_ppp_idletime,wan_ppp_redialperiod,wan_pppoe_lei,wan_pppoe_lef,wan_mtu_enable,wan_mtu,wan_ppp_mlppp,wan_modem_ipaddr,wan_islan,wan_sta,wan_dns,wan_dns_auto,wan_ifnameX,wan_ckmtd,wan2_proto,wan2_weight,wan2_modem_pin,wan2_modem_dev,wan2_modem_init,wan2_modem_apn,wan2_modem_speed,wan2_ppp_username,wan2_ppp_passwd,wan2_ppp_service,wan2_l2tp_server_ip,wan2_pptp_dhcp,wan2_ipaddr,wan2_netmask,wan2_gateway,wan2_pptp_server_ip,wan2_ppp_custom,wan2_ppp_demand,wan2_ppp_idletime,wan2_ppp_redialperiod,wan2_pppoe_lei,wan2_pppoe_lef,wan2_mtu_enable,wan2_mtu,wan2_ppp_mlppp,wan2_modem_ipaddr,wan2_islan,wan2_sta,wan2_dns,wan2_dns_auto,wan2_ifnameX,wan2_ckmtd,wan3_proto,wan3_weight,wan3_modem_pin,wan3_modem_dev,wan3_modem_init,wan3_modem_apn,wan3_modem_speed,wan3_ppp_username,wan3_ppp_passwd,wan3_ppp_service,wan3_l2tp_server_ip,wan3_pptp_dhcp,wan3_ipaddr,wan3_netmask,wan3_gateway,wan3_pptp_server_ip,wan3_ppp_custom,wan3_ppp_demand,wan3_ppp_idletime,wan3_ppp_redialperiod,wan3_pppoe_lei,wan3_pppoe_lef,wan3_mtu_enable,wan3_mtu,wan3_ppp_mlppp,wan3_modem_ipaddr,wan3_islan,wan3_sta,wan3_dns,wan3_dns_auto,wan3_ifnameX,wan3_ckmtd,wan4_proto,wan4_weight,wan4_modem_pin,wan4_modem_dev,wan4_modem_init,wan4_modem_apn,wan4_modem_speed,wan4_ppp_username,wan4_ppp_passwd,wan4_ppp_service,wan4_l2tp_server_ip,wan4_pptp_dhcp,wan4_ipaddr,wan4_netmask,wan4_gateway,wan4_pptp_server_ip,wan4_ppp_custom,wan4_ppp_demand,wan4_ppp_idletime,wan4_ppp_redialperiod,wan4_pppoe_lei,wan4_pppoe_lef,wan4_mtu_enable,wan4_mtu,wan4_ppp_mlppp,wan4_modem_ipaddr,wan4_islan,wan4_sta,wan4_dns,wan4_dns_auto,wan4_ifnameX,wan4_ckmtd,mwan_num,mwan_cktime,mwan_ckdst,dnssec_enable"); %>

		/* MULTIWAN-BEGIN */
		maxwan_num = 4;
		/* MULTIWAN-END */

		/* DUALWAN-BEGIN */
		if ( typeof( maxwan_num ) == 'undefined' ) maxwan_num = 2;
		/* DUALWAN-END */

		var sta_list = new Array();
		function refresh_sta_list()
		{
			var u;
			for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
				if (wl_sunit(uidx)<0) {
					u = wl_unit(uidx);
					sta_list[u] = new Array();
					sta_list[u][0] = 'wl'+uidx;
					sta_list[u][1] = wl_display_ifname(uidx);
				}
			}
			sta_list[u+1] = new Array();
			sta_list[u+1][0] = '';
			sta_list[u+1][1] = 'Disabled';
		}
		
		var lg = new TomatoGrid();
		lg.setup = function() {
			this.init('lan-grid', '', 4, [
				{ type: 'select', options: [[0, '0'],[1, '1'],[2, '2'],[3, '3']], prefix: '<div class="centered">', suffix: '</div>' },
				{ type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
				{ type: 'text', maxlen: 15, size: 17 },
				{ type: 'text', maxlen: 15, size: 17 },
				{ type: 'checkbox', prefix: '<div class="centered">', suffix: '</div>' },
				{ multi: [ { type: 'text', maxlen: 15, size: 17}, { type: 'text', maxlen: 15, size: 17 } ] },
				{ type: 'text', maxlen: 6, size: 8 }] );
			this.headerSet(['Bridge', 'STP', 'IP Address', 'Netmask', 'DHCP', 'IP&nbsp;Range&nbsp;<i>(first/last)</i>', 'Lease&nbsp;Time&nbsp;<i>(mins)</i>']);

			var numBridges = 0;
			for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
				var j = (i == 0) ? '' : i.toString();
				if (nvram['lan' + j + '_ifname'].length > 0) {
					if ((!fixIP(nvram['dhcpd' + j + '_startip'])) || (!fixIP(nvram['dhcpd' + j + '_endip']))) {
						if ((fixIP(nvram['lan' + j + '_ipaddr'])) && (fixIP(nvram['lan' + j + '_netmask'])) && (nvram['dhcp' + j + '_start'] != '')) {
							var n = getNetworkAddress(nvram['lan' + j + '_ipaddr'], nvram['lan' + j + '_netmask']);
							nvram['dhcpd' + j + '_startip'] = getAddress(('0.0.0.' + nvram['dhcp' + j + '_start'] * 1), n);
							nvram['dhcpd' + j + '_endip'] = getAddress(('0.0.0.' + ((nvram['dhcp' + j + '_start'] * 1) + (nvram['dhcp' + j + '_num'] *1) - 1)), n);
						}
					}
					lg.insertData(-1, [
						i.toString(),
						nvram['lan' + j + '_stp'],
						nvram['lan' + j + '_ipaddr'],
						nvram['lan' + j + '_netmask'],
						(nvram['lan' + j + '_proto'] == 'dhcp') ? '1' : '0',
						nvram['dhcpd' + j + '_startip'],
						nvram['dhcpd' + j + '_endip'],
						(nvram['lan' + j + '_proto'] == 'dhcp') ? (((nvram['dhcp' + j + '_lease'])*1 == 0) ? '1440' : (nvram['dhcp' + j + '_lease']).toString()) : ''
					] ) ;
					numBridges++;
				}
			}
			lg.canDelete = false;
			lg.sort(0);
			elem.removeClass(lg.header.cells[lg.sortColumn], 'sortasc', 'sortdes');
			lg.showNewEditor();
			lg.resetNewEditor();
		}

		lg.dataToView = function(data) {
			return ['br' + data[0],
					(data[1].toString() == '1') ? '<small><i>Enabled</i></small>' : '<small><i>Disabled</i></small>',
					data[2],
					data[3],
					(data[4].toString() == '1') ? '<small><i>Enabled</i></small>' : '<small><i>Disabled</i></small>',
					(data[5].toString() + ((numberOfBitsOnNetMask(data[3])>=24) ? (' - ' + data[6].split('.').splice(3, 1).toString()) : ('<br>' + data[6].toString()) )),
					(((data[7] != null) && (data[7] != '')) ? data[7] : '') ];
		}

		lg.dataToFieldValues = function (data) {
			return [data[0],
					(data[1] != 0) ? 'checked' : '',
					data[2].toString(),
					data[3].toString(),
					(data[4].toString() == '1') ? 'checked' : '',
					data[5].toString(),
					data[6].toString(),
					data[7].toString() ];
		}

		lg.fieldValuesToData = function(row) {
			var f = fields.getAll(row);
			return [f[0].value,
					f[1].checked ? 1 : 0,
					f[2].value,
					f[3].value,
					f[4].checked ? 1 : 0,
					f[5].value,
					f[6].value,
					f[7].value ];
		}

		lg.resetNewEditor = function() {
			var f = fields.getAll(this.newEditor);
			f[0].selectedIndex=0;
			var t = MAX_BRIDGE_ID;
			while((this.countBridge(f[0].selectedIndex) > 0) && (t > 0)) {
				f[0].selectedIndex = (f[0].selectedIndex%(MAX_BRIDGE_ID))+1;
				t--;
			}
			for(var j=0; j<= MAX_BRIDGE_ID ; j++) {
				f[0].options[j].disabled = (this.countBridge(j) > 0);
			}
			f[1].checked = 0;
			f[2].value = '';
			f[3].value = '';
			f[5].value = '';
			f[6].value = '';
			f[7].value = '';
			f[4].checked = 0;
			f[4].disabled = 1;
			f[5].disabled = 1;
			f[6].disabled = 1;
			f[7].disabled = 1;
			ferror.clearAll(fields.getAll(this.newEditor));
		}

		lg.onCancel = function() {
			this.removeEditor();
			this.showSource();
			this.disableNewEditor(false);

			this.resetNewEditor();
		}

		lg.onAdd = function() {
			var data;

			this.moving = null;
			this.rpHide();

			if (!this.verifyFields(this.newEditor, false)) return;

			data = this.fieldValuesToData(this.newEditor);
			this.insertData(-1, data);

			this.disableNewEditor(false);
			this.resetNewEditor();

			this.resort();
		}

		lg.onOK = function() {
			var i, data, view;

			if (!this.verifyFields(this.editor, false)) return;

			data = this.fieldValuesToData(this.editor);
			view = this.dataToView(data);

			this.source.setRowData(data);
			for (i = 0; i < this.source.cells.length; ++i) {
				this.source.cells[i].innerHTML = view[i];
			}

			this.removeEditor();
			this.showSource();
			this.disableNewEditor(false);

			this.resort();
			this.resetNewEditor();
		}

		lg.onDelete = function() {
			this.removeEditor();
			elem.remove(this.source);
			this.source = null;
			this.disableNewEditor(false);

			this.resetNewEditor();
		}

		lg.countElem = function(f, v) {
			var data = this.getAllData();
			var total = 0;
			for (var i = 0; i < data.length; ++i) {
				total += (data[i][f] == v) ? 1 : 0;
			}
			return total;
		}

		lg.countBridge = function (v) {
			return this.countElem(0,v);
		}

		lg.countOverlappingNetworks = function (ip) {
			var data = this.getAllData();
			var total = 0;
			for (var i = 0; i < data.length; ++i) {
				var net = getNetworkAddress(data[i][2], data[i][3]);
				var brd = getBroadcastAddress(net, data[i][3]);
				total += ((aton(ip) <= aton(brd)) && (aton(ip) >= aton(net))) ? 1 : 0;
			}
			return total;
		}

		lg.verifyFields = function(row, quiet) {
			var ok=1;
			var f;

			f = fields.getAll(row);

			for(var j=0; j<= MAX_BRIDGE_ID ; j++) {
				f[0].options[j].disabled = (this.countBridge(j) > 0);
			}

			if(this.countBridge(f[0].selectedIndex) > 0) {
				ferror.set(f[0], 'Cannot add another entry for bridge br' + f[0].selectedIndex, quiet);
				ok = 0;
			} else {
				ferror.clear(f[0]);
			}
// valid IP address?
			if(!v_ip(f[2], quiet || !ok))
				ok = 0;
// if we have a properly defined IP address - 0.0.0.0 is NOT a valid IP address for our intents/purposes!
			if ((f[2].value != '') && (f[2].value != '0.0.0.0')) {
// allow DHCP to be enabled
				f[4].disabled = 0;
// validate netmask
				if(!v_netmask(f[3], quiet || !ok)) {
					return 0;
				} else {
// should be 22 bits or smaller network
					if ((numberOfBitsOnNetMask(f[3].value) < 22) && (nvram.cstats_enable == '1' )) {
						if (!confirm("Netmask should have at least 22 bits (255.255.252.0). You may continue anyway but remember - you was warned")) return;
					} else {
						ferror.clear(f[3]);
					}
				}
				if(f[2].value == getNetworkAddress(f[2].value, f[3].value)) {
					var s = 'Invalid IP address or subnet mask (the address of the network cannot be used)';
					ferror.set(f[2], s, quiet);
					ferror.set(f[3], s, quiet);
					return 0;
				} else
				if(f[2].value == getBroadcastAddress(getNetworkAddress(f[2].value, f[3].value), f[3].value)) {
					var s = 'Invalid IP address or subnet mask (the broadcast address cannot be used)';
					ferror.set(f[2], s, quiet);
					ferror.set(f[3], s, quiet);
					return 0;
				} else
				if (this.countOverlappingNetworks(f[2].value) > 0) {
					var s = 'Invalid IP address or subnet mask (conflicts/overlaps with another LAN bridge)';
					ferror.set(f[2], s, quiet);
					ferror.set(f[3], s, quiet);
					return 0;
				} else {
					ferror.clear(f[2]);
					ferror.clear(f[3]);
				}
			} else {
				f[4].checked = 0;
				f[4].disabled = 1;
			}
// dhcp enabled?
			if( (f[4].checked) && (v_ip(f[2], 1)) && (v_netmask(f[3],1)) ) {
				f[5].disabled = 0;
				f[6].disabled = 0;
				f[7].disabled = 0;
// first/last IP still unset?
				if (f[5].value == '') {
					var l;
					var m = aton(f[2].value) & aton(f[3].value);
					var o = (m) ^ (~ aton(f[3].value))
					var n = o - m;
					do {
						if (--n < 0) {
							f[5].value = '';
							return;
						}
						m++;
					} while (((l = fixIP(ntoa(m), 1)) == null) || (l == f[2].value) );
					f[5].value = l;
				}
				if (f[6].value == '') {
					var l;
					var m = aton(f[2].value) & aton(f[3].value);
					var o = (m) ^ (~ aton(f[3].value));
					var n = o - m;
					do {
						if (--n < 0) {
							f[6].value = '';
							return;
						}
						o--;
					} while (((l = fixIP(ntoa(o), 1)) == null) || (l == f[2].value) );
					f[6].value = l;
				}
// first IP valid?
				if ((getNetworkAddress(f[5].value, f[3].value) != getNetworkAddress(f[2].value, f[3].value)) ||
					(f[5].value == getBroadcastAddress(getNetworkAddress(f[2].value, f[3].value), f[3].value)) ||
					(f[5].value == getNetworkAddress(f[2].value, f[3].value)) ||
					(f[2].value == f[5].value)) {
					ferror.set(f[5], 'Invalid first IP address or subnet mask', quiet || !ok);
					return 0;
				} else {
					ferror.clear(f[5]);
				}
// last IP valid?
				if ((getNetworkAddress(f[6].value, f[3].value) != getNetworkAddress(f[2].value, f[3].value)) ||
					(f[6].value == getBroadcastAddress(getNetworkAddress(f[2].value, f[3].value), f[3].value)) ||
					(f[6].value == getNetworkAddress(f[2].value, f[3].value)) ||
					(f[2].value == f[6].value)) {
					ferror.set(f[6], 'Invalid last IP address or subnet mask', quiet || !ok);
					return 0;
				} else {
					ferror.clear(f[6]);
				}
// validate range, swap first/last IP if needed
				if (aton(f[6].value) < aton(f[5].value)) {
					var t = f[5].value;
					f[5].value = f[6].value;
					f[6].value = t;
				}
// lease time
				if (parseInt(f[7].value*1) == 0)
					f[7].value = 1440; // from nvram/defaults.c
				if(!v_mins(f[7], quiet || !ok, 1, 10080))
					ok = 0;
			} else {
				f[5].disabled = 1;
				f[6].disabled = 1;
				f[7].disabled = 1;
				ferror.clear(f[5]);
				ferror.clear(f[6]);
				ferror.clear(f[7]);
			}
			return ok;
		}

		W('<style type=\'text/css\'>');
		for (var u = 0; u < wl_ifaces.length; ++u) {
			W('#spin'+wl_unit(u)+', ');
		}
		W('#spin {');
		W('	visibility: hidden;');
		W('	vertical-align: middle;');
		W('}');
		W('</style>');

		var xob = null;
		var refresher = [];
		var nphy = features('11n');
		var acphy = features('11ac');

		var ghz = [];
		var bands = [];
		var nm_loaded = [], ch_loaded = [], max_channel = [];


		for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//	if(wl_ifaces[uidx][0].indexOf('.') < 0) {
			if (wl_sunit(uidx)<0) {
				var b;
				b = [];
				for (var i = 0; i < wl_bands[uidx].length; ++i) {
					b.push([wl_bands[uidx][i] + '', (wl_bands[uidx][i] == '1') ? '5 GHz' : '2.4 GHz']);
				}
				bands.push(b);

				b = [];
				ghz.push(b);

				nm_loaded.push(0);
				ch_loaded.push(0);
				max_channel.push(0);
				refresher.push(null);
			}
		}

		function selectedBand(uidx)
		{
			if (bands[uidx].length > 1) {
				var e = E('_f_wl'+u+'_nband');
				return (e.value + '' == '' ? eval('nvram.wl'+u+'_nband') : e.value);
			} else if (bands[uidx].length > 0) {
				return bands[uidx][0][0] || '0';
			} else {
				return '0';
			}
		}

		function refreshNetModes(uidx)
		{
			var e, i, buf, val;

			if (uidx >= wl_ifaces.length) return;
			var u = wl_unit(uidx);

			var m = [['mixed','Auto']];
			if (selectedBand(uidx) == '1') {
				m.push(['a-only','A Only']);
				if (nphy) {
					m.push(['n-only','N Only']);
				}
			}
			else {
				m.push(['b-only','B Only']);
				m.push(['g-only','G Only']);
				if (nphy) {
					m.push(['bg-mixed','B/G Mixed']);
					m.push(['n-only','N Only']);
				}
			}

			e = E('_wl'+u+'_net_mode');
			buf = '';
			val = (!nm_loaded[uidx] || (e.value + '' == '')) ? eval('nvram.wl'+u+'_net_mode') : e.value;
			if (val == 'disabled') val = 'mixed';
			for (i = 0; i < m.length; ++i)
				buf += '<option value="' + m[i][0] + '"' + ((m[i][0] == val) ? ' selected' : '') + '>' + m[i][1] + '</option>';

			e = E('__wl'+u+'_net_mode');
			buf = '<select name="wl'+u+'_net_mode" onchange="verifyFields(this, 1)" id = "_wl'+u+'_net_mode">' + buf + '</select>';
			elem.setInnerHTML(e, buf);
			nm_loaded[uidx] = 1;
		}

		function refreshBandWidth(uidx)
		{
			var e, i, buf, val;

			if (uidx >= wl_ifaces.length) return;
			var u = wl_unit(uidx);

			var m = [['0','20 MHz']];
			if(nphy || acphy){
				m.push(['1','40 MHz']);
			}
			if(acphy && selectedBand(uidx) == '1') {
				m.push(['3','80 MHz']);
			}

			e = E('_wl'+u+'_nbw_cap');
			buf = '';
			val = (!nm_loaded[uidx] || (e.value + '' == '')) ? eval('nvram.wl'+u+'_nbw_cap') : e.value;
			for (i = 0; i < m.length; ++i)
				buf += '<option value="' + m[i][0] + '"' + ((m[i][0] == val) ? ' selected' : '') + '>' + m[i][1] + '</option>';

			e = E('__wl'+u+'_nbw_cap');
			buf = '<select name="wl'+u+'_nbw_cap" onchange="verifyFields(this, 1)" id = "_wl'+u+'_nbw_cap">' + buf + '</select>';
			elem.setInnerHTML(e, buf);
			nm_loaded[uidx] = 1;
		}

		function refreshChannels(uidx)
		{
			if (refresher[uidx] != null) return;
			if (u >= wl_ifaces.length) return;
			var u = wl_unit(uidx);

			refresher[uidx] = new XmlHttp();
			refresher[uidx].onCompleted = function(text, xml) {
				try {
					var e, i, buf, val;

					var wl_channels = [];
					eval(text);

					ghz[uidx] = [];
					max_channel[uidx] = 0;
					for (i = 0; i < wl_channels.length; ++i) {
						ghz[uidx].push([wl_channels[i][0] + '',
										(wl_channels[i][0]) ? ((wl_channels[i][1]) ? wl_channels[i][0] + ' - ' + (wl_channels[i][1] / 1000.0).toFixed(3) + ' GHz' : wl_channels[i][0] + '') : 'Auto']);
						max_channel[uidx] = wl_channels[i][0] * 1;
					}

					e = E('_wl'+u+'_channel');
					buf = '';
					val = (!ch_loaded[uidx] || (e.value + '' == '')) ? eval('nvram.wl'+u+'_channel') : e.value;
					for (i = 0; i < ghz[uidx].length; ++i)
						buf += '<option value="' + ghz[uidx][i][0] + '"' + ((ghz[uidx][i][0] == val) ? ' selected' : '') + '>' + ghz[uidx][i][1] + '</option>';

					e = E('__wl'+u+'_channel');
					buf = '<select name="wl'+u+'_channel" onchange="verifyFields(this, 1)" id = "_wl'+u+'_channel">' + buf + '</select>';
					elem.setInnerHTML(e, buf);
					ch_loaded[uidx] = 1;

					refresher[uidx] = null;
					verifyFields(null, 1);
				}
				catch (x) {
				}
				refresher[uidx] = null;
			}

			var bw, sb, e;

			e = E('_f_wl'+u+'_nctrlsb');
			sb = (e.value + '' == '' ? eval('nvram.wl'+u+'_nctrlsb') : e.value);
			e = E('_wl'+u+'_nbw_cap');
			switch(e.value + '' == '' ? eval('nvram.wl'+u+'_nbw_cap') : e.value) {
				case '0':
					bw = '20';
					break;
				case '1':
					bw = '40';
					break;
				case '3':
					bw = '80';
					break;
				default:
					alert("Wrong nbw_cap.");
			}

			refresher[uidx].onError = function(ex) { alert(ex); refresher[uidx] = null; reloadPage(); }
			refresher[uidx].post('update.cgi', 'exec=wlchannels&arg0=' + u + '&arg1=' + (nphy || acphy ? '1' : '0') +
											   '&arg2=' + bw + '&arg3=' + selectedBand(uidx) + '&arg4=' + sb);
		}

		function spin(x, unit)
		{
			for (var u = 0; u < wl_ifaces.length; ++u) {
				E('_f_wl'+wl_unit(u)+'_scan').disabled = x;
			}
			var e = E('_f_wl'+unit+'_scan');
			if (x) e.value = 'Scan ' + (wscan.tries + 1);
			else e.value = 'Scan';
			E('spin'+unit).style.visibility = x ? 'visible' : 'hidden';
		}

		function scan()
		{
			if (xob) return;

			var unit = wscan.unit;
			var uidx = wl_uidx(unit);

			xob = new XmlHttp();
			xob.onCompleted = function(text, xml) {
				try {
					var i;

					wlscandata = [];
					eval(text);

					for (i = 0; i < wlscandata.length; ++i) {
						var data = wlscandata[i];
						var ch = data[2];
						var mac = data[0];

						if (!wscan.inuse[ch]) {
							wscan.inuse[ch] = {
								count: 0,
								rssi: -999,
								ssid: ''
							};
						}

						if (!wscan.seen[mac]) {
							wscan.seen[mac] = 1;
							++wscan.inuse[ch].count;
						}

						if (data[4] > wscan.inuse[ch].rssi) {
							wscan.inuse[ch].rssi = data[4];
							wscan.inuse[ch].ssid = data[1];
						}
					}
					var e = E('_wl'+unit+'_channel');
					for (i = 1; i < ghz[uidx].length; ++i) {
						var s = ghz[uidx][i][1];
						var u = wscan.inuse[ghz[uidx][i][0]];
						if (u) s += ' (' + u.count + ' AP' + (u.count == 1 ? '' : 's') + ' / strongest: "' + escapeHTML(ellipsis(u.ssid, 15)) + '" ' + u.rssi + ' dBm)';
						e.options[i].innerHTML = s;
					}
					e.style.width = '400px';

					xob = null;

					if (wscan.tries < 4) {
						++wscan.tries;
						setTimeout(scan, 1000);
						return;
					}
				}
				catch (x) {
				}
				spin(0, unit);
			}
			xob.onError = function(x) {
				alert('error: ' + x);
				spin(0, unit);
				xob = null;
			}

			spin(1, unit);
			xob.post('update.cgi', 'exec=wlscan&arg0='+unit);
		}

		function scanButton(u)
		{
			if (xob) return;

			wscan = {
				unit: u,
				seen: [],
				inuse: [],
				tries: 0
			};

			scan();
		}

		function joinAddr(a) {
			var r, i, s;

			r = [];
			for (i = 0; i < a.length; ++i) {
				s = a[i];
				if ((s != '00:00:00:00:00:00') && (s != '0.0.0.0')) r.push(s);
			}
			return r.join(' ');
		}

		function random_x(max)
		{
			var c = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
			var s = '';
			while (max-- > 0) s += c.substr(Math.floor(c.length * Math.random()), 1);
			return s;
		}

		function random_psk(id)
		{
			var e = E(id);
			e.value = random_x(63);
			verifyFields(null, 1);
		}

		function random_wep(u)
		{
			E('_wl'+u+'_passphrase').value = random_x(16);
			generate_wep(u);
		}

		function v_wep(e, quiet)
		{
			var s = e.value;

			if (((s.length == 5) || (s.length == 13)) && (s.length == (e.maxLength >> 1))) {
				// no checking
			}
			else {
				s = s.toUpperCase().replace(/[^0-9A-F]/g, '');
				if (s.length != e.maxLength) {
					ferror.set(e, 'Invalid WEP key. Expecting ' + e.maxLength + ' hex or ' + (e.maxLength >> 1) + ' ASCII characters.', quiet);
					return 0;
				}
			}

			e.value = s;
			ferror.clear(e);
			return 1;
		}

		// compatible w/ Linksys' and Netgear's (key 1) method for 128-bits
		function generate_wep(u)
		{
			function _wepgen(pass, i)
			{
				while (pass.length < 64) pass += pass;
				return hex_md5(pass.substr(0, 64)).substr(i, (E('_wl'+u+'_wep_bit').value == 128) ? 26 : 10);
			}

			var e = E('_wl'+u+'_passphrase');
			var pass = e.value;
			if (!v_length(e, false, 3)) return;
			E('_wl'+u+'_key1').value = _wepgen(pass, 0);
			pass += '#$%';
			E('_wl'+u+'_key2').value = _wepgen(pass, 2);
			pass += '!@#';
			E('_wl'+u+'_key3').value = _wepgen(pass, 4);
			pass += '%&^';
			E('_wl'+u+'_key4').value = _wepgen(pass, 6);
			verifyFields(null, 1);
		}

		function verifyFields(focused, quiet)
		{
			var i;
			var ok = 1;
			var a, b, c, d, e;
			var u, uidx;
			var wmode, sm2;
			var curr_mwan_num = E('_mwan_num').value;

			var n = E('_f_lan_state').checked;
			E('_f_lan_desc').disabled = !n;
			E('_f_lan_invert').disabled = !n;

			var mwan = E('_mwan_num');
			if (mwan.options[(mwan.selectedIndex)].disabled) mwan.selectedIndex = 0;

			if (E('_mwan_cktime').value == 0)
			{
				elem.display(PR('_f_mwan_ckdst_1'), 0);
				elem.display(PR('_f_mwan_ckdst_2'), 0);
				E('_f_mwan_ckdst_1').disabled = 1;
				E('_f_mwan_ckdst_2').disabled = 1;
			}
			else
			{
				elem.display(PR('_f_mwan_ckdst_1'), 1);
				elem.display(PR('_f_mwan_ckdst_2'), 1);
				E('_f_mwan_ckdst_1').disabled = 0;
				E('_f_mwan_ckdst_2').disabled = 0;
			}

			if (!v_ip('_f_mwan_ckdst_1', true) && !v_domain('_f_mwan_ckdst_1', true))
			{
				ferror.set(E('_f_mwan_ckdst_1'), "Target 1 is not a valid IP address or domain name.", quiet);
				ok = 0;
			}
			if (!v_ip('_f_mwan_ckdst_2', true) && !v_domain('_f_mwan_ckdst_2', true))
			{
				ferror.set(E('_f_mwan_ckdst_2'), "Target 2 is not a valid IP address or domain name.", quiet);
				ok = 0;
			}

			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					u = wl_unit(uidx);
					if (focused == E('_f_wl'+u+'_nband')) {
						refreshNetModes(uidx);
						refreshChannels(uidx);
						refreshBandWidth(uidx);
					}
					else if (focused == E('_f_wl'+u+'_nctrlsb') || focused == E('_wl'+u+'_nbw_cap')) {
						refreshChannels(uidx);
					}
				}
			}

			// --- visibility ---

			var vis = {
				_f_dns_1: 1,
				_f_dns_2: 1,
				/* DNSSEC-BEGIN */
				_f_dnssec_enable: 1,
				/* DNSSEC-END */
				_lan_gateway: 1,
				_wan_wins: 1
			};

			var wanproto = new Array();
			for (uidx = 1; uidx <= maxwan_num; ++uidx){
				u = (uidx>1) ? uidx : '';
				if (uidx <= curr_mwan_num){
					vis['_wan'+u+'_proto'] = 1;
					vis['_wan'+u+'_weight'] = 1;
					vis['_wan'+u+'_ppp_username'] = 1;
					vis['_wan'+u+'_ppp_passwd'] = 1;
					vis['_wan'+u+'_ppp_service'] = 1;
					vis['_wan'+u+'_ppp_custom'] = 1;
					vis['_wan'+u+'_l2tp_server_ip'] = 1;
					vis['_wan'+u+'_ipaddr'] = 1;
					vis['_wan'+u+'_netmask'] = 1;
					vis['_wan'+u+'_gateway'] = 1;
					vis['_wan'+u+'_ckmtd'] = 1;
					vis['_wan'+u+'_pptp_server_ip'] = 1;
					vis['_f_wan'+u+'_pptp_dhcp'] = 1;
					vis['_wan'+u+'_ppp_demand'] = 1;
					vis['_wan'+u+'_ppp_idletime'] = 1;
					vis['_wan'+u+'_ppp_redialperiod'] = 1;
					vis['_wan'+u+'_pppoe_lei'] = 1;
					vis['_wan'+u+'_pppoe_lef'] = 1;
					vis['_wan'+u+'_mtu_enable'] = 1;
					vis['_f_wan'+u+'_mtu'] = 1;
					vis['_f_wan'+u+'_islan'] = 0;
					vis['_f_wan'+u+'_ppp_mlppp'] = 1;
					vis['_wan'+u+'_modem_ipaddr'] = 1;
					vis['_wan'+u+'_modem_pin'] = 1;
					vis['_wan'+u+'_modem_dev'] = 1;
					vis['_wan'+u+'_modem_init'] = 1;
					vis['_wan'+u+'_modem_apn'] = 1;
					vis['_wan'+u+'_modem_speed'] = 1;
					vis['_wan'+u+'_sta'] = 1;
					vis['_f_wan'+u+'_dns_1'] = 1;
					vis['_f_wan'+u+'_dns_2'] = 1;
					vis['_wan'+u+'_dns_auto'] = 1;
					E('_wan'+u+'_proto').disabled = 0;
					E('_wan'+u+'_weight').disabled = 0;
					E('_wan'+u+'_ppp_username').disabled = 0;
					E('_wan'+u+'_ppp_passwd').disabled = 0;
					E('_wan'+u+'_ppp_service').disabled = 0;
					E('_wan'+u+'_ppp_custom').disabled = 0;
					E('_wan'+u+'_l2tp_server_ip').disabled = 0;
					E('_wan'+u+'_ipaddr').disabled = 0;
					E('_wan'+u+'_netmask').disabled = 0;
					E('_wan'+u+'_gateway').disabled = 0;
					E('_wan'+u+'_ckmtd').disabled = 0;
					E('_wan'+u+'_pptp_server_ip').disabled = 0;
					E('_f_wan'+u+'_pptp_dhcp').disabled = 0;
					E('_wan'+u+'_ppp_demand').disabled = 0;
					E('_wan'+u+'_ppp_idletime').disabled = 0;
					E('_wan'+u+'_ppp_redialperiod').disabled = 0;
					E('_wan'+u+'_pppoe_lei').disabled = 0;
					E('_wan'+u+'_pppoe_lef').disabled = 0;
					E('_wan'+u+'_mtu_enable').disabled = 0;
					E('_f_wan'+u+'_mtu').disabled = 0;
					E('_f_wan'+u+'_islan').disabled = 0;
					E('_f_wan'+u+'_ppp_mlppp').disabled = 0;
					E('_wan'+u+'_modem_ipaddr').disabled = 0;
					E('_wan'+u+'_modem_pin').disabled = 0;
					E('_wan'+u+'_modem_dev').disabled = 0;
					E('_wan'+u+'_modem_init').disabled = 0;
					E('_wan'+u+'_modem_apn').disabled = 0;
					E('_wan'+u+'_modem_speed').disabled = 0;
					E('_wan'+u+'_sta').disabled = 0;
					E('_f_wan'+u+'_dns_1').disabled = 0;
					E('_f_wan'+u+'_dns_2').disabled = 0;
					E('_wan'+u+'_dns_auto').disabled = 0;
					wanproto[uidx-1] = E('_wan'+u+'_proto').value;
				}
				else
				{
					vis['_wan'+u+'_proto'] = 0;
					vis['_wan'+u+'_weight'] = 0;
					vis['_wan'+u+'_ppp_username'] = 0;
					vis['_wan'+u+'_ppp_passwd'] = 0;
					vis['_wan'+u+'_ppp_service'] = 0;
					vis['_wan'+u+'_ppp_custom'] = 0;
					vis['_wan'+u+'_l2tp_server_ip'] = 0;
					vis['_wan'+u+'_ipaddr'] = 0;
					vis['_wan'+u+'_netmask'] = 0;
					vis['_wan'+u+'_gateway'] = 0;
					vis['_wan'+u+'_ckmtd'] = 0;
					vis['_wan'+u+'_pptp_server_ip'] = 0;
					vis['_f_wan'+u+'_pptp_dhcp'] = 0;
					vis['_wan'+u+'_ppp_demand'] = 0;
					vis['_wan'+u+'_ppp_idletime'] = 0;
					vis['_wan'+u+'_ppp_redialperiod'] = 0;
					vis['_wan'+u+'_pppoe_lei'] = 0;
					vis['_wan'+u+'_pppoe_lef'] = 0;
					vis['_wan'+u+'_mtu_enable'] = 0;
					vis['_f_wan'+u+'_mtu'] = 0;
					vis['_f_wan'+u+'_islan'] = 0;
					vis['_f_wan'+u+'_ppp_mlppp'] = 0;
					vis['_wan'+u+'_modem_ipaddr'] = 0;
					vis['_wan'+u+'_modem_pin'] = 0;
					vis['_wan'+u+'_modem_dev'] = 0;
					vis['_wan'+u+'_modem_init'] = 0;
					vis['_wan'+u+'_modem_apn'] = 0;
					vis['_wan'+u+'_modem_speed'] = 0;
					vis['_wan'+u+'_sta'] = 0;
					vis['_f_wan'+u+'_dns_1'] = 0;
					vis['_f_wan'+u+'_dns_2'] = 0;
					vis['_wan'+u+'_dns_auto'] = 0;
					E('_wan'+u+'_proto').disabled = 0;
					E('_wan'+u+'_weight').disabled = 1;
					E('_wan'+u+'_ppp_username').disabled = 1;
					E('_wan'+u+'_ppp_passwd').disabled = 1;
					E('_wan'+u+'_ppp_service').disabled = 1;
					E('_wan'+u+'_ppp_custom').disabled = 1;
					E('_wan'+u+'_l2tp_server_ip').disabled = 1;
					E('_wan'+u+'_ipaddr').disabled = 1;
					E('_wan'+u+'_netmask').disabled = 1;
					E('_wan'+u+'_gateway').disabled = 1;
					E('_wan'+u+'_ckmtd').disabled = 1;
					E('_wan'+u+'_pptp_server_ip').disabled = 1;
					E('_f_wan'+u+'_pptp_dhcp').disabled = 1;
					E('_wan'+u+'_ppp_demand').disabled = 1;
					E('_wan'+u+'_ppp_idletime').disabled = 1;
					E('_wan'+u+'_ppp_redialperiod').disabled = 1;
					E('_wan'+u+'_pppoe_lei').disabled = 1;
					E('_wan'+u+'_pppoe_lef').disabled = 1;
					E('_wan'+u+'_mtu_enable').disabled = 1;
					E('_f_wan'+u+'_mtu').disabled = 1;
					E('_f_wan'+u+'_islan').disabled = 1;
					E('_f_wan'+u+'_ppp_mlppp').disabled = 1;
					E('_wan'+u+'_modem_ipaddr').disabled = 1;
					E('_wan'+u+'_modem_pin').disabled = 1;
					E('_wan'+u+'_modem_dev').disabled = 1;
					E('_wan'+u+'_modem_init').disabled = 1;
					E('_wan'+u+'_modem_apn').disabled = 1;
					E('_wan'+u+'_modem_speed').disabled = 1;
					E('_f_wan'+u+'_dns_1').disabled = 1;
					E('_f_wan'+u+'_dns_2').disabled = 1;
					E('_wan'+u+'_dns_auto').disabled = 1;
				}
			}

			var wl_vis = [];
			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					a = {
						_f_wl_radio: 1,
						_f_wl_mode: 1,
						_f_wl_nband: (bands[uidx].length > 1) ? 1 : 0,
						_wl_net_mode: 1,
						_wl_ssid: 1,
						_f_wl_bcast: 1,
						_wl_channel: 1,
						_wl_nbw_cap: nphy || acphy ? 1 : 0,
						_f_wl_nctrlsb: nphy || acphy ? 1 : 0,
						_f_wl_scan: 1,

						_wl_security_mode: 1,
						_wl_crypto: 1,
						_wl_wpa_psk: 1,
						_f_wl_psk_random1: 1,
						_f_wl_psk_random2: 1,
						_wl_wpa_gtk_rekey: 1,
						_wl_radius_key: 1,
						_wl_radius_ipaddr: 1,
						_wl_radius_port: 1,
						_wl_wep_bit: 1,
						_wl_passphrase: 1,
						_f_wl_wep_gen: 1,
						_f_wl_wep_random: 1,
						_wl_key1: 1,
						_wl_key2: 1,
						_wl_key3: 1,
						_wl_key4: 1,

						_f_wl_lazywds: 1,
						_f_wl_wds_0: 1
					};
					wl_vis.push(a);
				}
			}

			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					wmode = E('_f_wl'+wl_unit(uidx)+'_mode').value;

					if (wmode == 'wet') {
						E('_mwan_num').value = 1;
						E('_mwan_cktime').value = 0;
						elem.display('mwan-title', 'wan-settings', false);
						for (uidx = 1; uidx <= maxwan_num; ++uidx) {
							u = (uidx>1) ? uidx : '';
							vis['_wan'+u+'_proto'] = 0;
							E('_wan'+u+'_proto').value = 'disabled';
							wanproto[uidx-1] = 'disabled';
							elem.display('wan'+u+'-title', 'sesdiv_wan'+u, false);
						}
					} else {
						elem.display('mwan-title', 'wan-settings', true);
						for (uidx = 1; uidx <= curr_mwan_num; ++uidx) {
							u = (uidx>1) ? uidx : '';
							vis['_wan'+u+'_proto'] = 1;
							elem.display('wan'+u+'-title', 'sesdiv_wan'+u, true);
						}
					}

					if ((wanproto[0] == 'disabled') || (wmode == 'wet')) {
						vis._f_wan_islan = 1;
					}
				}
			}

			for (uidx = 1; uidx <= maxwan_num; ++uidx){
				u = (uidx>1) ? uidx : '';
				switch (wanproto[uidx-1]) {
					case 'disabled':
						vis['_wan'+u+'_weight'] = 0;
						vis['_wan'+u+'_ppp_username'] = 0;
						vis['_wan'+u+'_ppp_service'] = 0;
						vis['_wan'+u+'_ppp_custom'] = 0;
						vis['_wan'+u+'_l2tp_server_ip'] = 0;
						vis['_wan'+u+'_ipaddr'] = 0;
						vis['_wan'+u+'_netmask'] = 0;
						vis['_wan'+u+'_gateway'] = 0;
						vis['_wan'+u+'_ckmtd'] = 0;
						vis['_wan'+u+'_pptp_server_ip'] = 0;
						vis['_f_wan'+u+'_pptp_dhcp'] = 0;
						vis['_wan'+u+'_ppp_demand'] = 0;
						vis['_wan'+u+'_mtu_enable'] = 0;
						vis['_f_wan'+u+'_mtu'] = 0;
						vis['_f_wan'+u+'_ppp_mlppp'] = 0;
						vis['_wan'+u+'_modem_ipaddr'] = 0;
						vis['_wan'+u+'_modem_pin'] = 0;
						vis['_wan'+u+'_modem_dev'] = 0;
						vis['_wan'+u+'_modem_init'] = 0;
						vis['_wan'+u+'_modem_apn'] = 0;
						vis['_wan'+u+'_modem_speed'] = 0;
						vis['_wan'+u+'_pppoe_lei'] = 0;
						vis['_wan'+u+'_pppoe_lef'] = 0;
						vis['_f_wan'+u+'_dns_1'] = 0;
						vis['_f_wan'+u+'_dns_2'] = 0;
						vis['_wan'+u+'_dns_auto'] = 0;
						E('_wan'+u+'_sta').value = '';
						break;
					case 'dhcp':
						vis['_wan'+u+'_l2tp_server_ip'] = 0;
						vis['_wan'+u+'_pppoe_lei'] = 0;
						vis['_wan'+u+'_pppoe_lef'] = 0;
						vis['_wan'+u+'_ppp_demand'] = 0;
						vis['_wan'+u+'_ppp_service'] = 0;
						vis['_wan'+u+'_ppp_username'] = 0;
						vis['_wan'+u+'_ppp_custom'] = 0;
						vis['_wan'+u+'_pptp_server_ip'] = 0;
						vis['_f_wan'+u+'_pptp_dhcp'] = 0;
						vis['_wan'+u+'_gateway'] = 0;
						vis['_wan'+u+'_ipaddr'] = 0;
						vis['_wan'+u+'_netmask'] = 0;
						vis['_f_wan'+u+'_ppp_mlppp'] = 0;
						vis['_wan'+u+'_modem_ipaddr'] = 1;

						vis['_wan'+u+'_modem_pin'] = 0;
						vis['_wan'+u+'_modem_dev'] = 0;
						vis['_wan'+u+'_modem_init'] = 0;
						vis['_wan'+u+'_modem_apn'] = 0;
						vis['_wan'+u+'_modem_speed'] = 0;
						break;
					case 'pppoe':
						vis['_wan'+u+'_l2tp_server_ip'] = 0;
						vis['_wan'+u+'_pptp_server_ip'] = 0;
//            vis['_f_wan'+u+'_pptp_dhcp'] = 0;
						vis['_wan'+u+'_gateway'] = 0;
						vis['_wan'+u+'_ipaddr'] = 0;
						vis['_wan'+u+'_netmask'] = 0;
						vis['_wan'+u+'_modem_ipaddr'] = 1;

						vis['_wan'+u+'_modem_pin'] = 0;
						vis['_wan'+u+'_modem_dev'] = 0;
						vis['_wan'+u+'_modem_init'] = 0;
						vis['_wan'+u+'_modem_apn'] = 0;
						vis['_wan'+u+'_modem_speed'] = 0;
						break;
					case 'ppp3g':
						vis['_wan'+u+'_pppoe_lei'] = 0;
						vis['_wan'+u+'_pppoe_lef'] = 0;
						vis['_wan'+u+'_ppp_service'] = 0;
						vis['_wan'+u+'_l2tp_server_ip'] = 0;
						vis['_wan'+u+'_pptp_server_ip'] = 0;
						vis['_f_wan'+u+'_pptp_dhcp'] = 0;
						vis['_wan'+u+'_gateway'] = 0;
						vis['_wan'+u+'_ipaddr'] = 0;
						vis['_wan'+u+'_netmask'] = 0;
						vis['_wan'+u+'_modem_ipaddr'] = 0;
						vis['_wan'+u+'_modem_speed'] = 0;
						vis['_f_wan'+u+'_ppp_mlppp'] = 0;
						vis['_wan'+u+'_sta'] = 0;
						break;
					case 'lte':
						vis['_wan'+u+'_pppoe_lei'] = 0;
						vis['_wan'+u+'_pppoe_lef'] = 0;
						vis['_wan'+u+'_ppp_service'] = 0;
						vis['_wan'+u+'_ppp_demand'] = 0;
						vis['_wan'+u+'_ppp_username'] = 0;
						vis['_wan'+u+'_ppp_custom'] = 0;
						vis['_wan'+u+'_l2tp_server_ip'] = 0;
						vis['_wan'+u+'_pptp_server_ip'] = 0;
						vis['_f_wan'+u+'_pptp_dhcp'] = 0;
						vis['_wan'+u+'_gateway'] = 0;
						vis['_wan'+u+'_ipaddr'] = 0;
						vis['_wan'+u+'_netmask'] = 0;
						vis['_wan'+u+'_modem_ipaddr'] = 0;
						vis['_wan'+u+'_modem_dev'] = 0;
						vis['_wan'+u+'_modem_init'] = 0;
						vis['_f_wan'+u+'_ppp_mlppp'] = 0;
						vis['_wan'+u+'_sta'] = 0;
						break;
					case 'static':
						vis['_wan'+u+'_pppoe_lei'] = 0;
						vis['_wan'+u+'_pppoe_lef'] = 0;
						vis['_wan'+u+'_l2tp_server_ip'] = 0;
						vis['_wan'+u+'_ppp_demand'] = 0;
						vis['_wan'+u+'_ppp_service'] = 0;
						vis['_wan'+u+'_ppp_username'] = 0;
						vis['_wan'+u+'_ppp_custom'] = 0;
						vis['_wan'+u+'_pptp_server_ip'] = 0;
						vis['_f_wan'+u+'_pptp_dhcp'] = 0;
						vis['_f_wan'+u+'_ppp_mlppp'] = 0;
						vis['_wan'+u+'_modem_ipaddr'] = 1;

						vis['_wan'+u+'_modem_pin'] = 0;
						vis['_wan'+u+'_modem_dev'] = 0;
						vis['_wan'+u+'_modem_init'] = 0;
						vis['_wan'+u+'_modem_apn'] = 0;
						vis['_wan'+u+'_modem_speed'] = 0;
						break;
					case 'pptp':
						vis['_wan'+u+'_pppoe_lei'] = 0;
						vis['_wan'+u+'_pppoe_lef'] = 0;
						vis['_wan'+u+'_l2tp_server_ip'] = 0;
						vis['_wan'+u+'_ppp_service'] = 0;
						vis['_wan'+u+'_gateway'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
						vis['_wan'+u+'_ipaddr'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
						vis['_wan'+u+'_netmask'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
						vis['_wan'+u+'_modem_ipaddr'] = 0;

						vis['_wan'+u+'_modem_pin'] = 0;
						vis['_wan'+u+'_modem_dev'] = 0;
						vis['_wan'+u+'_modem_init'] = 0;
						vis['_wan'+u+'_modem_apn'] = 0;
						vis['_wan'+u+'_modem_speed'] = 0;
						break;
					case 'l2tp':
						vis['_wan'+u+'_pppoe_lei'] = 0;
						vis['_wan'+u+'_pppoe_lef'] = 0;
						vis['_wan'+u+'_pptp_server_ip'] = 0;
						vis['_wan'+u+'_ppp_service'] = 0;
						vis['_wan'+u+'_gateway'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
						vis['_wan'+u+'_ipaddr'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
						vis['_wan'+u+'_netmask'] = (!E('_f_wan'+u+'_pptp_dhcp').checked);
						vis['_wan'+u+'_modem_ipaddr'] = 0;

						vis['_wan'+u+'_modem_pin'] = 0;
						vis['_wan'+u+'_modem_dev'] = 0;
						vis['_wan'+u+'_modem_init'] = 0;
						vis['_wan'+u+'_modem_apn'] = 0;
						vis['_wan'+u+'_modem_speed'] = 0;
						break;
				}
				if (wanproto[uidx-1] == 'static') {
					E('_wan'+u+'_dns_auto').value = '0';
					E('_wan'+u+'_dns_auto').options[0].disabled = true;
				} else {
					E('_wan'+u+'_dns_auto').options[0].disabled = false;
				}
				if ((vis['_wan'+u+'_dns_auto'] == 0) || (E('_wan'+u+'_dns_auto').value == '1')){
					vis['_f_wan'+u+'_dns_1'] = 0;
					vis['_f_wan'+u+'_dns_2'] = 0;
				}

				if(wanproto[uidx-1] != 'disabled' && uidx <= curr_mwan_num) {
					vis._lan_gateway = 0;
					vis._f_dns_1 = 0;
					vis._f_dns_2 = 0;
				}

				vis['_wan'+u+'_ppp_idletime'] = (E('_wan'+u+'_ppp_demand').value == 1) && vis['_wan'+u+'_ppp_demand']
				vis['_wan'+u+'_ppp_redialperiod'] = !vis['_wan'+u+'_ppp_idletime'] && vis['_wan'+u+'_ppp_demand'];
				vis['_wan'+u+'_ppp_passwd'] = vis['_wan'+u+'_ppp_username'];

				if (vis['_wan'+u+'_mtu_enable']) {
					if (E('_wan'+u+'_mtu_enable').value == 0) {
						vis['_f_wan'+u+'_mtu'] = 2;
						a = E('_f_wan'+u+'_mtu');
						switch (E('_wan'+u+'_proto').value) {
							case 'pppoe':
								a.value = 1492;
								break;
							case 'pptp':
							case 'l2tp':
								a.value = 1460;
								break;
							default:
								a.value = 1500;
								break;
						}
					}
				}
			}

			/* DNSCRYPT-BEGIN */
			var p = E('_f_dnscrypt_proxy').checked;
			vis._dnscrypt_priority = p;
			vis._dnscrypt_port = p;
			vis._dnscrypt_log = p;
			vis._f_dnscrypt_manual = p;
			var q = E('_f_dnscrypt_proxy').checked && E('_f_dnscrypt_manual').checked;
			vis._dnscrypt_provider_name = q;
			vis._dnscrypt_provider_key = q;
			vis._dnscrypt_resolver_address = q;
			var r = E('_f_dnscrypt_proxy').checked && !E('_f_dnscrypt_manual').checked;
			vis._dnscrypt_resolver = r;
			/* DNSCRYPT-END */

			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					u = wl_unit(uidx);
					wmode = E('_f_wl'+u+'_mode').value;

					if (!E('_f_wl'+u+'_radio').checked) {
						for (a in wl_vis[uidx]) {
							wl_vis[uidx][a] = 2;
						}
						wl_vis[uidx]._f_wl_radio = 1;
						wl_vis[uidx]._wl_nbw_cap = nphy || acphy ? 2 : 0;
						wl_vis[uidx]._f_wl_nband = (bands[uidx].length > 1) ? 2 : 0;
					}

					switch (wmode) {
						case 'apwds':
						case 'wds':
							break;
						case 'wet':
						case 'sta':
							wl_vis[uidx]._f_wl_bcast = 0;
							//wl_vis[uidx]._wl_channel = 0;
							//wl_vis[uidx]._wl_nbw_cap = 0;
							vis['_wan_modem_ipaddr'] = 0;
						default:
							wl_vis[uidx]._f_wl_lazywds = 0;
							wl_vis[uidx]._f_wl_wds_0 = 0;
							break;
					}

					sm2 = E('_wl'+u+'_security_mode').value;
					switch (sm2) {
						case 'disabled':
							wl_vis[uidx]._wl_crypto = 0;
							wl_vis[uidx]._wl_wep_bit = 0;
							wl_vis[uidx]._wl_wpa_psk = 0;
							wl_vis[uidx]._wl_radius_key = 0;
							wl_vis[uidx]._wl_radius_ipaddr = 0;
							wl_vis[uidx]._wl_wpa_gtk_rekey = 0;
							break;
						case 'wep':
							wl_vis[uidx]._wl_crypto = 0;
							wl_vis[uidx]._wl_wpa_psk = 0;
							wl_vis[uidx]._wl_radius_key = 0;
							wl_vis[uidx]._wl_radius_ipaddr = 0;
							wl_vis[uidx]._wl_wpa_gtk_rekey = 0;
							break;
						case 'radius':
							wl_vis[uidx]._wl_crypto = 0;
							wl_vis[uidx]._wl_wpa_psk = 0;
							break;
						default:	// wpa*
							wl_vis[uidx]._wl_wep_bit = 0;
							if (sm2.indexOf('personal') != -1) {
								wl_vis[uidx]._wl_radius_key = 0;
								wl_vis[uidx]._wl_radius_ipaddr = 0;
							}
							else {
								wl_vis[uidx]._wl_wpa_psk = 0;
							}
							break;
					}

					if ((E('_f_wl'+u+'_lazywds').value == 1) && (wl_vis[uidx]._f_wl_wds_0 == 1)) {
						wl_vis[uidx]._f_wl_wds_0 = 2;
					}

					if (wl_vis[uidx]._wl_nbw_cap != 0) {
						switch (E('_wl'+u+'_net_mode').value) {
							case 'b-only':
							case 'g-only':
							case 'a-only':
							case 'bg-mixed':
								wl_vis[uidx]._wl_nbw_cap = 2;
								if (E('_wl'+u+'_nbw_cap').value != '0') {
									E('_wl'+u+'_nbw_cap').value = 0;
									refreshChannels(uidx);
								}
								break;
						}
						// avoid Enterprise-TKIP with 40MHz
						if ((sm2 == 'wpa_enterprise') && (E('_wl'+u+'_crypto').value == 'tkip')) {
							wl_vis[uidx]._wl_nbw_cap = 2;
							if (E('_wl'+u+'_nbw_cap').value != '0') {
								E('_wl'+u+'_nbw_cap').value = 0;
								refreshChannels(uidx);
							}
						}
					}

					wl_vis[uidx]._f_wl_nctrlsb = (E('_wl'+u+'_nbw_cap').value == 0) ? 0 : wl_vis[uidx]._wl_nbw_cap;

					/* REMOVE-BEGIN
					 This is ugly...
					 Special case - 2.4GHz band, currently running in B/G-only mode,
					 with N/Auto and 40MHz selected in the GUI.
					 Channel list is not filtered in this case by the wl driver,
					 and includes all channels available with 20MHz channel width.
					 REMOVE-END */
					b = selectedBand(uidx);
					if (wl_vis[uidx]._wl_channel == 1 && wl_vis[uidx]._f_wl_nctrlsb != 0 &&
					    ((b == '2') || (wl_vis[uidx]._f_wl_nband == 0 && b == '0'))) {
						switch (eval('nvram.wl'+u+'_net_mode')) {
							case 'b-only':
							case 'g-only':
							case 'bg-mixed':
								i = E('_wl'+u+'_channel').value * 1;
								if (i > 0 && i < 5) {
									E('_f_wl'+u+'_nctrlsb').value = 'lower';
									wl_vis[uidx]._f_wl_nctrlsb = 2;
								}
								else if (i > max_channel[uidx] - 4) {
									E('_f_wl'+u+'_nctrlsb').value = 'upper';
									wl_vis[uidx]._f_wl_nctrlsb = 2;
								}
								break;
						}
					}

					wl_vis[uidx]._f_wl_scan = wl_vis[uidx]._wl_channel;
					wl_vis[uidx]._f_wl_psk_random1 = wl_vis[uidx]._wl_wpa_psk;
					wl_vis[uidx]._f_wl_psk_random2 = wl_vis[uidx]._wl_radius_key;
					wl_vis[uidx]._wl_radius_port = wl_vis[uidx]._wl_radius_ipaddr;
					wl_vis[uidx]._wl_key1 = wl_vis[uidx]._wl_key2 = wl_vis[uidx]._wl_key3 = wl_vis[uidx]._wl_key4 = wl_vis[uidx]._f_wl_wep_gen = wl_vis[uidx]._f_wl_wep_random = wl_vis[uidx]._wl_passphrase = wl_vis[uidx]._wl_wep_bit;

					for (i = 1; i < 10; ++i) {
						wl_vis[uidx]['_f_wl_wds_' + i] = wl_vis[uidx]._f_wl_wds_0;
					}
				}
			} // for each wl_iface

			for (a in vis) {
				b = E(a);
				c = vis[a];
				/* REMOVE-BEGIN
				 //		if (b != null)
				 REMOVE-END */
				b.disabled = (c != 1);
				PR(b).style.display = c ? '' : 'none';
			}

			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
				if(wl_ifaces[uidx][0].indexOf('.') < 0) {
					for (a in wl_vis[uidx]) {
						i = 3;
						if (a.substr(0, 6) == '_f_wl_') i = 5;
						b = E(a.substr(0, i) + wl_unit(uidx) + a.substr(i, a.length));
						c = wl_vis[uidx][a];
						b.disabled = (c != 1);
						PR(b).style.display = c ? '' : 'none';
					}
				}
			}

			// --- verify ---

			for (uidx = 1; uidx <= maxwan_num; ++uidx) {
				u = (uidx>1) ? uidx : '';
				ferror.clear('_wan'+u+'_proto');
			}

			var wlclnt = 0;
			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					u = wl_unit(uidx);
					wmode = E('_f_wl'+u+'_mode').value;
					sm2 = E('_wl'+u+'_security_mode').value;

					/* REMOVE-BEGIN
					 if ((wl_vis[uidx]._f_wl_mode == 1) && (wmode != 'ap') && (sm2.substr(0, 4) == 'wpa2')) {
					 ferror.set('_wl'+u+'_security_mode', 'WPA2 is supported only in AP mode.', quiet || !ok);
					 ok = 0;
					 }
					 else ferror.clear('_wl'+u+'_security_mode');
					 REMOVE-END */

					// --- N standard does not support WPA+TKIP ---
					a = E('_wl'+u+'_crypto');
					switch (E('_wl'+u+'_net_mode').value) {
						case 'mixed':
						case 'n-only':
							if ((nphy || acphy) && (a.value == 'tkip') && (sm2.indexOf('wpa') != -1)) {
								ferror.set(a, 'TKIP encryption is not supported with WPA / WPA2 in N and AC mode.', quiet || !ok);
								ok = 0;
							}
							else ferror.clear(a);
							break;
					}

					a = E('_wl'+u+'_net_mode');
					ferror.clear(a);
					b = E('_f_wl'+u+'_mode');
					ferror.clear(b);
					if ((wmode == 'sta') || (wmode == 'wet')) {
						++wlclnt;
						if (wlclnt > 1) {
							ferror.set(b, 'Only one wireless interface can be configured in client mode.', quiet || !ok);
							ok = 0;
						}
						else if (a.value == 'n-only') {
							ferror.set(a, 'N-only is not supported in wireless client modes, use Auto.', quiet || !ok);
							ok = 0;
						}
					}

					a = E('_wl'+u+'_wpa_psk');
					ferror.clear(a);
					if (wl_vis[uidx]._wl_wpa_psk == 1) {
						if ((a.value.length < 8) || ((a.value.length == 64) && (a.value.search(/[^0-9A-Fa-f]/) != -1))) {
							ferror.set('_wl'+u+'_wpa_psk', 'Invalid pre-shared key. Please enter at least 8 characters or 64 hexadecimal digits.', quiet || !ok);
							ok = 0;
						}
					}

					// wl channel
					if (((wmode == 'wds') || (wmode == 'apwds')) && (wl_vis[uidx]._wl_channel == 1) && (E('_wl'+u+'_channel').value == '0')) {
						ferror.set('_wl'+u+'_channel', 'Fixed wireless channel required in WDS mode.', quiet || !ok);
						ok = 0;
					}
					else ferror.clear('_wl'+u+'_channel');

				}
			}

			for (uidx = 1; uidx <= curr_mwan_num; ++uidx) {
				p = (uidx >1) ? uidx : '';
				// domain name or IP address
				if ((vis['_wan'+u+'_l2tp_server_ip']) && ((!v_length('_wan'+u+'_l2tp_server_ip', 1, 1)) || ((!v_ip('_wan'+u+'_l2tp_server_ip', 1)) && (!v_domain('_wan'+u+'_l2tp_server_ip', 1))))) {
					ok = 0;
					if (!quiet) ferror.show('_wan'+u+'_l2tp_server_ip');
				}
				if ((vis['_wan'+u+'_pptp_server_ip']) && ((!v_length('_wan'+u+'_pptp_server_ip', 1, 1)) || ((!v_ip('_wan'+u+'_pptp_server_ip', 1)) && (!v_domain('_wan'+u+'_pptp_server_ip', 1))))) {
					ok = 0;
					if (!quiet) ferror.show('_wan'+u+'_pptp_server_ip');
				}
				// WANx IP address
				if ((vis['_wan'+u+'_wan_ipaddr']) && (!v_ip('_wan'+u+'_wan_ipaddr', quiet))) ok = 0;
				if ((vis['_wan'+u+'_wan_gateway']) && (!v_ip('_wan'+u+'_wan_gateway', quiet))) ok = 0;
				if ((vis['_wan'+u+'_modem_ipaddr']) && (!v_ip('_wan'+u+'_modem_ipaddr', quiet))) ok = 0;
				// WANx netmask
				if ((vis['_wan'+u+'_wan_netmask']) && (!v_netmask('_wan'+u+'_wan_netmask', quiet))) ok = 0;
				// range
				if ((vis['_wan'+u+'_ppp_idletime']) && (!v_range('_wan'+u+'_ppp_idletime', quiet, 3, 1440))) ok = 0;
				if ((vis['_wan'+u+'_ppp_redialperiod']) && (!v_range('_wan'+u+'_ppp_redialperiod', quiet, 1, 86400))) ok = 0;
				if ((vis['_f_wan'+u+'_mtu']) && (!v_range('_f_wan'+u+'_mtu', quiet, 576, 1500))) ok = 0;
				if ((vis['_wan'+u+'_pppoe_lei']) && (!v_range('_wan'+u+'_pppoe_lei', quiet, 1, 60))) ok = 0;
				if ((vis['_wan'+u+'_pppoe_lef']) && (!v_range('_wan'+u+'_pppoe_lef', quiet, 1, 10))) ok = 0;
			}

			// IP address, blank -> 0.0.0.0
			a = ['_f_dns_1', '_f_dns_2', '_f_dns_3','_wan_wins','_lan_gateway'];
			for (i = a.length - 1; i >= 0; --i)
				if ((vis[a[i]]) && (!v_dns(a[i], quiet || !ok))) ok = 0;

			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					u = wl_unit(uidx);

					// IP address
					a = ['_radius_ipaddr'];
					for (i = a.length - 1; i >= 0; --i) {
						if ((wl_vis[uidx]['_wl'+a[i]]) && (!v_ip('_wl'+u+a[i], quiet || !ok))) ok = 0;
					}

					// range
					a = [['_wpa_gtk_rekey', 60, 7200], ['_radius_port', 1, 65535]];
					for (i = a.length - 1; i >= 0; --i) {
						v = a[i];
						if ((wl_vis[uidx]['_wl'+v[0]]) && (!v_range('_wl'+u+v[0], quiet || !ok, v[1], v[2]))) ok = 0;
					}

					// length
					a = [['_ssid', 1], ['_radius_key', 1]];
					for (i = a.length - 1; i >= 0; --i) {
						v = a[i];
						if ((wl_vis[uidx]['_wl'+v[0]]) && (!v_length('_wl'+u+v[0], quiet || !ok, v[1], E('_wl'+u+v[0]).maxlength))) ok = 0;
					}

					if (wl_vis[uidx]._wl_key1) {
						a = (E('_wl'+u+'_wep_bit').value == 128) ? 26 : 10;
						for (i = 1; i <= 4; ++i) {
							b = E('_wl'+u+'_key' + i);
							b.maxLength = a;
							if ((b.value.length > 0) || (E('_f_wl'+u+'_wepidx_' + i).checked)) {
								if (!v_wep(b, quiet || !ok)) ok = 0;
							}
							else ferror.clear(b);
						}
					}

					ferror.clear('_f_wl'+u+'_wds_0');
					if (wl_vis[uidx]._f_wl_wds_0 == 1) {
						b = 0;
						for (i = 0; i < 10; ++i) {
							a = E('_f_wl'+u+'_wds_' + i);
							if (!v_macz(a, quiet || !ok)) ok = 0;
							else if (!isMAC0(a.value)) b = 1;
						}
						if (!b) {
							ferror.set('_f_wl'+u+'_wds_0', 'WDS MAC address required.', quiet || !ok);
							ok = 0;
						}
					}
				}
			}

			for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
				if (wl_sunit(uidx)<0) {
					var u = wl_unit(uidx);
					E('_f_wl'+u+'_mode').options[0].disabled = false;
					E('_f_wl'+u+'_mode').options[1].disabled = false;
					E('_f_wl'+u+'_mode').options[2].disabled = true;
					E('_f_wl'+u+'_mode').options[3].disabled = false;
					E('_f_wl'+u+'_mode').options[4].disabled = false;
					E('_wl'+u+'_security_mode').options[2].disabled = false;
					E('_wl'+u+'_security_mode').options[3].disabled = false;
					E('_wl'+u+'_security_mode').options[4].disabled = false;
					E('_wl'+u+'_security_mode').options[5].disabled = false;
				}
			}

			for (var uidx = 1; uidx <= curr_mwan_num; ++uidx){
				var u = (uidx >1) ? uidx : '';
				var sta_wl = E('_wan'+u+'_sta').value;
				if(sta_wl != ''){
					E('_f_'+sta_wl+'_mode').value = 'sta';
					E('_f_'+sta_wl+'_mode').options[0].disabled = true;
					E('_f_'+sta_wl+'_mode').options[1].disabled = true;
					E('_f_'+sta_wl+'_mode').options[2].disabled = false;
					E('_f_'+sta_wl+'_mode').options[3].disabled = true;
					E('_f_'+sta_wl+'_mode').options[4].disabled = true;
					E('_'+sta_wl+'_security_mode').options[2].disabled = true;
					E('_'+sta_wl+'_security_mode').options[3].disabled = true;
					E('_'+sta_wl+'_security_mode').options[4].disabled = true;
					E('_'+sta_wl+'_security_mode').options[5].disabled = true;
					for (var i = uidx+1; i <= curr_mwan_num; ++i){
						if(E('_wan'+u+'_sta').value == E('_wan'+i+'_sta').value) {
							ferror.set('_wan'+i+'_sta', 'Wireless Client mode can be set only to one WAN port', quiet || !ok);
							ok = 0;
						}
					}
				}
			}

			if (curr_mwan_num == 1)
			{
				elem.display(PR('_wan_weight'), 0);
				E('_wan_weight').disabled = 1;
			}

			if (E('_mwan_cktime').value == 0)
			{
				elem.display(PR('_wan_ckmtd'), 0);
				E('_wan_ckmtd').disabled = 1;
				elem.display(PR('_wan2_ckmtd'), 0);
				E('_wan2_ckmtd').disabled = 1;
				/* MULTIWAN-BEGIN */
				elem.display(PR('_wan3_ckmtd'), 0);
				E('_wan3_ckmtd').disabled = 1;
				elem.display(PR('_wan4_ckmtd'), 0);
				E('_wan4_ckmtd').disabled = 1;
				/* MULTIWAN-END */
			}

			var count = 0;
			if((E('_wan_proto').value == 'lte') || (E('_wan_proto').value == 'ppp3g')) count++;
			if((E('_wan2_proto').value == 'lte') || (E('_wan2_proto').value == 'ppp3g')) count++;
			/* MULTIWAN-BEGIN */
			if((E('_wan3_proto').value == 'lte') || (E('_wan3_proto').value == 'ppp3g')) count++;
			if((E('_wan4_proto').value == 'lte') || (E('_wan4_proto').value == 'ppp3g')) count++;
			/* MULTIWAN-END */

			if(count > 1) {
				for (var g = 0; g <= curr_mwan_num; g++) {
					var h = (g==0) ? '' : g.toString();
					ferror.set('_wan'+h+'_proto', '3G or LTE mode can be set only to one WAN port', quiet || !ok);
					ok = 0;
				}
			}

			return ok;
		}

		function earlyInit()
		{
			var mwan = E('_mwan_num');
			if(nvram.wan_ifnameX.length < 1)
				mwan.options[0].disabled=true;
			if(nvram.wan2_ifnameX.length < 1)
				mwan.options[1].disabled=true;
			/* MULTIWAN-BEGIN */
			if(nvram.wan3_ifnameX.length < 1)
				mwan.options[2].disabled=true;
			if(nvram.wan4_ifnameX.length < 1)
				mwan.options[3].disabled=true;
			/* MULTIWAN-END */

			verifyFields(null, 1);
		}

		function save()
		{
			if (lg.isEditing()) return;
			lg.resetNewEditor();

			var a, b, c;
			var i;
			var u, uidx, wmode, sm2, wradio;
			var curr_mwan_num = E('_mwan_num').value;

			if (!verifyFields(null, false)) return;

			var fom = E('_fom');

			for (uidx = 0; uidx < wl_ifaces.length; ++uidx) {
//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					u = wl_unit(uidx);
					wmode = E('_f_wl'+u+'_mode').value;
					sm2 = E('_wl'+u+'_security_mode').value;
					wradio = E('_f_wl'+u+'_radio').checked;

					E('_wl'+u+'_nband').value = selectedBand(uidx);

					if (wmode == 'apwds') E('_wl'+u+'_mode').value = 'ap';
					else E('_wl'+u+'_mode').value = wmode;

					if (wmode == 'wet') {
						for (wan_uidx = 1; wan_uidx <= maxwan_num; ++wan_uidx) {
							var v = (wan_uidx>1) ? wan_uidx : '';
							E('_wan'+v+'_proto').value = 'disabled';
						}
						/* REMOVE-BEGIN */
// TODO - what's required ? integrate with tomatogrid?
						/* REMOVE-END */
					}

					a = [];
					for (i = 0; i < 10; ++i) a.push(E('_f_wl'+u+'_wds_' + i).value);
					E('_wl'+u+'_wds').value = joinAddr(a);

					if (wmode.indexOf('wds') != -1) {
						E('_wl'+u+'_wds_enable').value = 1;
						E('_wl'+u+'_lazywds').value = E('_f_wl'+u+'_lazywds').value;
						if (E('_wl'+u+'_lazywds').value == 1) E('_wl'+u+'_wds').value = '';
					}
					else {
						E('_wl'+u+'_wds_enable').value = 0;
						E('_wl'+u+'_wds').value = '';
						E('_wl'+u+'_lazywds').value = 0;
					}

					E('_wl'+u+'_radio').value = wradio ? 1 : 0;
					E('_wl'+u+'_auth').value = eval('nvram.wl'+u+'_auth');

					e = E('_wl'+u+'_akm');
					switch (sm2) {
						case 'disabled':
						case 'radius':
						case 'wep':
							e.value = '';
							break;
						default:
							c = [];

							if (sm2.indexOf('personal') != -1) {
								if (sm2.indexOf('wpa2_') == -1) c.push('psk');
								if (sm2.indexOf('wpa_') == -1) c.push('psk2');
							}
							else {
								if (sm2.indexOf('wpa2_') == -1) c.push('wpa');
								if (sm2.indexOf('wpa_') == -1) c.push('wpa2');
							}
							c = c.join(' ');
							e.value = c;
							break;
					}
					E('_wl'+u+'_auth_mode').value = (sm2 == 'radius') ? 'radius' : 'none';
					E('_wl'+u+'_wep').value = ((sm2 == 'radius') || (sm2 == 'wep')) ? 'enabled': 'disabled';

					if (sm2.indexOf('wpa') != -1) E('_wl'+u+'_auth').value = 0;

					E('_wl'+u+'_nreqd').value = 0;
					E('_wl'+u+'_gmode').value = 1;
					E('_wl'+u+'_nmode').value = 0;
					E('_wl'+u+'_nmcsidx').value = -2; // Legacy Rate
					E('_wl'+u+'_nbw').value = 0;
					switch (E('_wl'+u+'_net_mode').value) {
						case 'b-only':
							E('_wl'+u+'_gmode').value = 0;
							break;
						case 'g-only':
							E('_wl'+u+'_gmode').value = 4;
							break;
						case 'bg-mixed':
							break;
						case 'a-only':
							E('_wl'+u+'_nmcsidx').value = -1; // Auto
							break;
						case 'n-only':
							if (selectedBand(uidx) == '1') { // 5 GHz
								E('_wl'+u+'_nmode').value = -1;
								E('_wl'+u+'_nmcsidx').value = -1;
							} else {
								E('_wl'+u+'_nmode').value = 1;
								E('_wl'+u+'_nmcsidx').value = 32;
							}
							E('_wl'+u+'_nreqd').value = 1;
							break;
						default: // Auto
							E('_wl'+u+'_nmode').value = -1;
							E('_wl'+u+'_nmcsidx').value = -1;
							break;
					}

					E('_wl'+u+'_nctrlsb').value = eval('nvram.wl'+u+'_nctrlsb');
					if (E('_wl'+u+'_nmode').value != 0) {
						E('_wl'+u+'_nctrlsb').value = E('_f_wl'+u+'_nctrlsb').value;
						E('_wl'+u+'_nbw').value = (E('_wl'+u+'_nbw_cap').value == 0) ? 20 : ((E('_wl'+u+'_nbw_cap').value== 3) ? 80:40);
					}

					E('_wl'+u+'_closed').value = E('_f_wl'+u+'_bcast').checked ? 0 : 1;

					a = fields.radio.selected(eval('fom.f_wl'+u+'_wepidx'));
					if (a) E('_wl'+u+'_key').value = a.value;
				}
			}

			/* DNSSEC-BEGIN */
			fom.dnssec_enable.value = fom.f_dnssec_enable.checked ? 1 : 0;
			/* DNSSEC-END */

			/* DNSCRYPT-BEGIN */
			fom.dnscrypt_proxy.value = fom.f_dnscrypt_proxy.checked ? 1 : 0;
			fom.dnscrypt_manual.value = fom.f_dnscrypt_manual.checked ? 1 : 0;
			/* DNSCRYPT-END */

			fom.lan_state.value = fom.f_lan_state.checked ? 1 : 0;
			fom.lan_desc.value = fom.f_lan_desc.checked ? 1 : 0;
			fom.lan_invert.value = fom.f_lan_invert.checked ? 1 : 0;

			// initialize/wipe out relevant fields
			for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
				var j = (i == 0) ? '' : i.toString();
				fom['lan' + j + '_ifname'].value = '';
				fom['lan' + j + '_ipaddr'].value = '';
				fom['lan' + j + '_netmask'].value = '';
				fom['lan' + j + '_proto'].value = '';
				fom['lan' + j + '_stp'].value = '';
				fom['dhcp' + j + '_start'].value = '';
				fom['dhcp' + j + '_num'].value = '';
				fom['dhcp' + j + '_lease'].value = '';
				fom['dhcpd' + j + '_startip'].value = '';
				fom['dhcpd' + j + '_endip'].value = '';
			}

			var d = lg.getAllData();
			for (var i = 0; i < d.length; ++i) {

				if (lg.countOverlappingNetworks(d[i][2]) > 1) {
					var s = 'Cannot proceed: two or more LAN bridges have conflicting IP addresses or overlapping subnets';
					alert(s);
					var e = E('footer-msg');
					e.innerHTML = s;
					e.style.visibility = 'visible';
					setTimeout(
						function() {
							e.innerHTML = '';
							e.style.visibility = 'hidden';
						}, 5000);
					return;
				}

				var j = (parseInt(d[i][0]) == 0) ? '' : d[i][0].toString();
				fom['lan' + j + '_ifname'].value = 'br' + d[i][0];
				fom['lan' + j + '_stp'].value = d[i][1];
				fom['lan' + j + '_ipaddr'].value = d[i][2];
				fom['lan' + j + '_netmask'].value = d[i][3];
				fom['lan' + j + '_proto'].value = (d[i][4] != '0') ? 'dhcp' : 'static';
				fom['dhcp' + j + '_start'].value = (d[i][4] != '0') ? (d[i][5]).split('.').splice(3, 1) : '';
				fom['dhcp' + j + '_num'].value = (d[i][4] != '0') ? d[i][6].split('.').splice(3, 1) - (d[i][5]).split('.').splice(3, 1) + 1 : ''; // presuming /24 subnet (legacy)
				fom['dhcp' + j + '_lease'].value = (d[i][4] != '0') ? d[i][7] : '';
				fom['dhcpd' + j + '_startip'].value = (d[i][4] != '0') ? d[i][5] : '';
				fom['dhcpd' + j + '_endip'].value = (d[i][4] != '0') ? d[i][6] : '';

				/* REMOVE-BEGIN */
				alert('lan' + j + '_ifname=' + fom['lan' + j + '_ifname'].value + '\n' +
					'lan' + j + '_stp=' + fom['lan' + j + '_stp'].value + '\n' +
					'lan' + j + '_ipaddr=' + fom['lan' + j + '_ipaddr'].value + '\n' +
					'lan' + j + '_netmask=' + fom['lan' + j + '_netmask'].value + '\n' +
					'lan' + j + '_proto=' + fom['lan' + j + '_proto'].value + '\n' +
					'dhcp' + j + '_start=' + fom['dhcp' + j + '_start'].value + '\n' +
					'dhcp' + j + '_num=' + fom['dhcp' + j + '_num'].value + '\n' +
					'dhcp' + j + '_lease=' + fom['dhcp' + j + '_lease'].value + '\n' +
					'dhcpd' + j + '_startip=' + fom['dhcpd' + j + '_startip'].value + '\n' +
					'dhcpd' + j + '_endip=' + fom['dhcpd' + j + '_endip'].value);
				/* REMOVE-END */
			}

			var e = E('footer-msg');
			var t = fixIP(fom['lan_ipaddr'].value);
			if ((fom['lan_ifname'].value != 'br0') || (fom['lan_ipaddr'].value == '0.0.0.0') || (!t)) {
				e.innerHTML = 'Bridge br0 must be always defined and have a valid IP address set.';
				e.style.visibility = 'visible';
				setTimeout(
					function() {
						e.innerHTML = '';
						e.style.visibility = 'hidden';
					}, 5000);
				return;
			}

			/* REMOVE-BEGIN
			 //	if ((nvram.lan_ipaddr != fom.lan_ipaddr.value) || (nvram.lan1_ipaddr != fom.lan1_ipaddr.value) ||
			 //		(nvram.lan2_ipaddr != fom.lan2_ipaddr.value) || (nvram.lan3_ipaddr != fom.lan3_ipaddr.value)){
			 REMOVE-END */
			for ( uidx = 1; uidx <= maxwan_num; ++uidx ) {
				u                                     = (uidx > 1) ? uidx : '';
				fom[ 'wan' + u + '_mtu' ].value       = fom[ 'f_wan' + u + '_mtu' ].value;
				fom[ 'wan' + u + '_mtu' ].disabled    = fom[ 'f_wan' + u + '_mtu' ].disabled;
				fom[ 'wan' + u + '_islan' ].value     = fom[ 'f_wan' + u + '_islan' ].checked ? 1 : 0;
				fom[ 'wan' + u + '_pptp_dhcp' ].value = fom[ 'f_wan' + u + '_pptp_dhcp' ].checked ? 1 : 0;
				fom[ 'wan' + u + '_ppp_mlppp' ].value = fom[ 'f_wan' + u + '_ppp_mlppp' ].checked ? 1 : 0;
				if ( fom[ 'wan' + u + '_dns_auto' ].value == '1' ) {
					fom[ 'wan' + u + '_dns' ].value = '';
				} else {
					fom[ 'wan' + u + '_dns' ].value = joinAddr( [ fom[ 'f_wan' + u + '_dns_1' ].value, fom[ 'f_wan' + u + '_dns_2' ].value ] );
				}
			}

			fom.wan_dns.value = joinAddr( [ fom.f_dns_1.value, fom.f_dns_2.value ] );
			for ( var uidx = 1; uidx <= curr_mwan_num; ++uidx ) {
				var u = (uidx > 1) ? uidx : '';
				if ( fom[ 'wan' + u + '_proto' ].value != 'disabled' ) {
					fom.wan_dns.value = joinAddr( [ fom.f_wan_dns_1.value, fom.f_wan_dns_2.value ] );
					break;
				}
			}
			if ( E( '_mwan_cktime' ).value ) {
				fom.mwan_ckdst.value = fom.f_mwan_ckdst_1.value + ',' + fom.f_mwan_ckdst_2.value;
			} else {
				fom.mwan_ckdst.value = '';
			}
			/* REMOVE-BEGIN */
			for ( var uidx = 1; uidx <= curr_mwan_num; ++uidx ) {
				var u = (uidx > 1) ? uidx : '';
				if ( nvram[ 'wan' + u + '_sta' ] != fom[ 'wan' + u + '_sta' ].value ) {
					fom.mwan_init.value = 1;
					fom._reboot.value   = 1;
					form.submit( fom );
				}
			}
			if ( nvram.mwan_num != fom.mwan_num.value ) {
				fom.mwan_init.value = 1;
				fom._reboot.value   = 1;
				form.submit( fom );
			}
			/* REMOVE-END */
			if ( nvram.lan_ipaddr != fom.lan_ipaddr.value ) {
				fom._moveip.value = 1;
				form.submit( fom );
			}
			else {
				form.submit( fom, 1 );
			}
		}

		function init() {

			for ( var uidx = 0; uidx < wl_ifaces.length; ++uidx ) {
				//		if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if ( wl_sunit( uidx ) < 0 ) {
					refreshNetModes( uidx );
					refreshChannels( uidx );
					refreshBandWidth( uidx );
				}
			}

			refreshWanSection();
		}

	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#basic-network.asp">
		<input type="hidden" name="_nextwait" value="10">
		<input type='hidden' name='_reboot' value='0'>
		<input type="hidden" name="_service" value="*">
		<input type="hidden" name="_moveip" value="0">

		<!-- DNSSEC-BEGIN -->
		<input type='hidden' name='dnssec_enable'>
		<!-- DNSSEC-END -->
		/* DNSCRYPT-BEGIN */
		<input type='hidden' name='dnscrypt_proxy'>
		<input type='hidden' name='dnscrypt_manual'>
		/* DNSCRYPT-END */
		<input type='hidden' name='lan_state'>
		<input type='hidden' name='lan_desc'>
		<input type='hidden' name='lan_invert'>
		<div id="extraInputs"></div>

		<!-- VLAN-BEGIN -->
		<script type="text/javascript">
			var thisForm = $('#extraInputs');
			for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
				var j = (i == 0) ? '' : i.toString();
				$(thisForm).append('<input type=\'hidden\' id=\'lan' + j + '_ifname\' name=\'lan' + j + '_ifname\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'lan' + j + '_ipaddr\' name=\'lan' + j + '_ipaddr\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'lan' + j + '_netmask\' name=\'lan' + j + '_netmask\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'lan' + j + '_proto\' name=\'lan' + j + '_proto\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'lan' + j + '_stp\' name=\'lan' + j + '_stp\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'dhcp' + j + '_start\' name=\'dhcp' + j + '_start\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'dhcp' + j + '_num\' name=\'dhcp' + j + '_num\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'dhcp' + j + '_lease\' name=\'dhcp' + j + '_lease\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'dhcpd' + j + '_startip\' name=\'dhcpd' + j + '_startip\'>');
				$(thisForm).append('<input type=\'hidden\' id=\'dhcpd' + j + '_endip\' name=\'dhcpd' + j + '_endip\'>');
			}
		</script>
		<!-- VLAN-END -->

		<div class="box" data-box="network-wan">
			<div class="heading" id="mwan-title">MultiWAN</div>
			<div class="content" id="wan-settings"></div>

				/* REMOVE-BEGIN */
				<input type="hidden" name="mwan_init" value="0">
				/* REMOVE-END */

				<input type="hidden" name="mwan_ckdst">
				<script type="text/javascript">
					
					function refreshWanSection() {
						var curr_mwan_num = E( '_mwan_num' ).value;
						for ( uidx = maxwan_num; uidx > 1; --uidx ) {
							var u = (uidx > 1) ? uidx : '';
							elem.display( 'wan' + u + '-title', 'sesdiv_wan' + u, curr_mwan_num >= uidx );
						}
					}
					
					ckdst = nvram.mwan_ckdst.split( ',' );
					$( '#wan-settings' ).forms([
						{
							title: 'Number of WAN Ports', name: 'mwan_num', type: 'select', options: [ [ '1', '1 WAN' ], [ '2', '2 WAN' ]
							/* MULTIWAN-BEGIN */
							, [ '3', '3 WAN' ], [ '4', '4 WAN' ]
							/* MULTIWAN-END */
						], value : nvram.mwan_num, suffix: ' <small>Please configure <a href="/#advanced-vlan.asp">VLAN</a> first</small>'
						},
						{
							title : 'Check connections every', name: 'mwan_cktime', type: 'select', options: [
							[ '0', 'Disabled' ], [ '60', '1 minute' ], [ '120', '2 minutes *' ], [ '180', '3 minutes' ], [ '300', '5 minutes' ],
							[ '600', '10 minutes' ], [ '900', '15 minutes' ], [ '1800', '30 minutes' ], [ '3600', '1 hour' ] ],
							suffix: ' <small>(Default: 2 minutes, when the network conditions are poor, try using longer detection period)</small>',
							value : nvram.mwan_cktime
						},
						{ title: 'Target 1', indent: 2, name: 'f_mwan_ckdst_1', type: 'text', maxlen: 30, size: 30, value: ckdst[ 0 ] || '' },
						{ title: 'Target 2', indent: 2, name: 'f_mwan_ckdst_2', type: 'text', maxlen: 30, size: 30, value: ckdst[ 1 ] || '' }
					]);

					$( '_mwan_num' ).on('change', function() {
						refreshWanSection();
						verifyFields( null, 1 );
					});

				</script>

			<script type="text/javascript">

				refresh_sta_list();

				for ( var uidx = 1; uidx <= maxwan_num; ++uidx ) {
					var u = (uidx > 1) ? uidx : '';
					dns   = nvram[ 'wan' + u + '_dns' ].split( /\s+/ );
					$(thisForm).append( '<input type=\'hidden\' name=\'wan' + u + '_mtu\'>' );
					$(thisForm).append( '<input type=\'hidden\' name=\'wan' + u + '_islan\'>' );
					$(thisForm).append( '<input type=\'hidden\' name=\'wan' + u + '_pptp_dhcp\'>' );
					$(thisForm).append( '<input type=\'hidden\' name=\'wan' + u + '_ppp_mlppp\'>' );
					$(thisForm).append( '<input type=\'hidden\' name=\'wan' + u + '_dns\'>' );

					$( '#wan-settings' ).append( '<div id=\'sesdiv_wan' + u + '\'><br/><h5 id=\'wan' + u + '-title\'>WAN' + u + ' Settings</h5>' );
					$( '#wan-settings' ).forms([
						{
							title: 'Type', name: 'wan' + u + '_proto', type: 'select', options: [ [ 'dhcp', 'DHCP' ], [ 'pppoe', 'PPPoE' ], [ 'static', 'Static' ], [ 'pptp', 'PPTP' ], [ 'l2tp', 'L2TP' ],
							/* LINUX26-BEGIN */
							/* USB-BEGIN */
							[ 'ppp3g', '3G Modem' ],
							[ 'lte', '4G/LTE' ],
							/* USB-END */
							/* LINUX26-END */
						  [ 'disabled', 'Disabled' ] ],
							value: nvram[ 'wan' + u + '_proto' ]
						},
						{ title: 'Wireless Client Mode', name: 'wan' + u + '_sta', type: 'select', options: sta_list, value: nvram[ 'wan' + u + '_sta' ] },
						{ title    : 'Modem device',
							name   : 'wan' + u + '_modem_dev',
							type   : 'select',
							options: [ [ 'ttyUSB0', '/dev/ttyUSB0' ], [ 'ttyUSB1', '/dev/ttyUSB1' ], [ 'ttyUSB2', '/dev/ttyUSB2' ], [ 'ttyUSB3', '/dev/ttyUSB3' ], [ 'ttyUSB4', '/dev/ttyUSB4' ], [ 'ttyUSB5', '/dev/ttyUSB5' ], [ 'ttyUSB6', '/dev/ttyUSB6' ], [ 'ttyACM0', '/dev/ttyACM0' ] ],
							value  : nvram[ 'wan' + u + '_modem_dev' ]
						},
						{ title: 'Load Balance Weight', name: 'wan' + u + '_weight', type: 'text', maxlen: 3, size: 8, value: nvram[ 'wan' + u + '_weight' ], suffix: ' <i>(Failover: 0; Load balancing: 1 - 256)</i>' },
						{ title: 'PIN Code', name: 'wan' + u + '_modem_pin', type: 'text', maxlen: 6, size: 8, value: nvram[ 'wan' + u + '_modem_pin' ], suffix: ' <i>Advised to turn off PIN Code</i>' },
						{ title: 'Modem init string', name: 'wan' + u + '_modem_init', type: 'text', maxlen: 25, size: 32, value: nvram[ 'wan' + u + '_modem_init' ] },
						{ title: 'APN', name: 'wan'+u+'_modem_apn', type: 'text', maxlen: 25, size: 32, suffix: ' <i>(if empty, AT+CGDCONT will not be sent)</i>', value: nvram['wan'+u+'_modem_apn'] },						{ title    : 'Network Type',
							name   : 'wan' + u + '_modem_speed',
							type   : 'select',
							options: [ [ '00', 'Auto' ], [ '030201', '4G/3G/2G' ], [ '0302', '4G/3G only' ], [ '03', '4G only' ], [ '02', '3G only' ] ],
							value  : nvram[ 'wan' + u + '_modem_speed' ],
							suffix : ' <i>works only with non-Hilink modems</i>'
						},
						{ title: 'Username', name: 'wan' + u + '_ppp_username', type: 'text', maxlen: 60, size: 64, value: nvram[ 'wan' + u + '_ppp_username' ] },
						{ title: 'Password', name: 'wan' + u + '_ppp_passwd', type: 'password', maxlen: 60, size: 64, peekaboo: 1, value: nvram[ 'wan' + u + '_ppp_passwd' ] },
						{ title: 'Service Name', name: 'wan' + u + '_ppp_service', type: 'text', maxlen: 50, size: 64, value: nvram[ 'wan' + u + '_ppp_service' ] },
						{ title: 'L2TP Server', name: 'wan' + u + '_l2tp_server_ip', type: 'text', maxlen: 128, size: 64, value: nvram[ 'wan' + u + '_l2tp_server_ip' ] },
						{ title: 'Use DHCP', name: 'f_wan'+u+'_pptp_dhcp', type: 'checkbox', value: (nvram['wan'+u+'_pptp_dhcp'] == 1) },
						{ title: 'IP Address', name: 'wan' + u + '_ipaddr', type: 'text', maxlen: 15, size: 17, value: nvram[ 'wan' + u + '_ipaddr' ] },
						{ title: 'Subnet Mask', name: 'wan' + u + '_netmask', type: 'text', maxlen: 15, size: 17, value: nvram[ 'wan' + u + '_netmask' ] },
						{ title: 'Gateway', name: 'wan' + u + '_gateway', type: 'text', maxlen: 15, size: 17, value: nvram[ 'wan' + u + '_gateway' ] },
						{ title: 'PPTP Gateway', name: 'wan' + u + '_pptp_server_ip', type: 'text', maxlen: 128, size: 64, value: nvram[ 'wan' + u + '_pptp_server_ip' ] },
						{ title: 'Options', name: 'wan' + u + '_ppp_custom', type: 'text', maxlen: 256, size: 64, value: nvram[ 'wan' + u + '_ppp_custom' ] },
						{ title: 'DNS Server', name: 'wan' + u + '_dns_auto', type: 'select', options: [ [ '1', 'Auto' ], [ '0', 'Manual' ] ], value: nvram[ 'wan' + u + '_dns_auto' ] },
						{ title: 'DNS 1', indent: 2, name: 'f_wan' + u + '_dns_1', type: 'text', maxlen: 15, size: 17, value: dns[ 0 ] || '0.0.0.0' },
						{ title: 'DNS 2', indent: 2, name: 'f_wan' + u + '_dns_2', type: 'text', maxlen: 15, size: 17, value: dns[ 1 ] || '0.0.0.0' },
						{
							title: 'Connect Mode', name: 'wan' + u + '_ppp_demand', type: 'select', options: [ [ '1', 'Connect On Demand' ], [ '0', 'Keep Alive' ] ],
							value: nvram[ 'wan' + u + '_ppp_demand' ]
						},
						{
							title: 'Max Idle Time', indent: 2, name: 'wan' + u + '_ppp_idletime', type: 'text', maxlen: 5, size: 7, suffix: ' <i>(minutes)</i>',
							value: nvram[ 'wan' + u + '_ppp_idletime' ]
						},
						{
							title: 'Redial Interval', indent: 2, name: 'wan' + u + '_ppp_redialperiod', type: 'text', maxlen: 5, size: 7, suffix: ' <i>(seconds)</i>',
							value: nvram[ 'wan' + u + '_ppp_redialperiod' ]
						},
						{
							title: 'LCP Echo Interval', indent: 2, name: 'wan' + u + '_pppoe_lei', type: 'text', maxlen: 5, size: 7, suffix: ' <i>seconds (range: 1 - 60; default: 10)</i>',
							value: nvram[ 'wan' + u + '_pppoe_lei' ]
						},
						{
							title: 'LCP Echo Link fail limit', indent: 2, name: 'wan' + u + '_pppoe_lef', type: 'text', maxlen: 5, size: 7, suffix: ' <i>(range: 1 - 10; default: 5)</i>',
							value: nvram[ 'wan' + u + '_pppoe_lef' ]
						},
						{
							title: 'MTU', multi: [
							{ name: 'wan' + u + '_mtu_enable', type: 'select', options: [ [ '0', 'Default' ], [ '1', 'Manual' ] ], value: nvram[ 'wan' + u + '_mtu_enable' ] },
							{ name: 'f_wan' + u + '_mtu', type: 'text', maxlen: 4, size: 6, value: nvram[ 'wan' + u + '_mtu' ] } ]
						},
						{ title: 'Single Line MLPPP', name: 'f_wan' + u + '_ppp_mlppp', type: 'checkbox', value: (nvram[ 'wan' + u + '_ppp_mlppp' ] == 1) },

						{ title: 'Route Modem IP', name: 'wan' + u + '_modem_ipaddr',type: 'text',	maxlen: 15,	size  : 17,	suffix: ' <i>(must be in different subnet to router, 0.0.0.0 to disable)</i>', value : nvram[ 'wan' + u + '_modem_ipaddr' ]	},
						{ title: 'Watchdog Mode', name: 'wan'+u+'_ckmtd', type: 'select', options: [['1','Ping'],['2','Traceroute*']], value: nvram['wan'+u+'_ckmtd'], suffix: ' <i>(default: Traceroute; Use Ping only when Traceroute is not working correctly</i>' },

						{ title: 'Bridge WAN port to primary LAN (br0)', name: 'f_wan' + u + '_islan', type: 'checkbox', value: (nvram[ 'wan' + u + '_islan' ] == 1) }
					]);

					$( '#wan-settings' ).append( '</div>');

				}

			</script>

		</div>

		<div class="box" data-box="network-lan">
			<div class="heading">LAN</div>
			<div class="content lan-settings">
				<!-- VLAN-BEGIN -->
				<table class="line-table" id="lan-grid"></table><br />
				<script type="text/javascript">lg.setup();</script>
				<!-- VLAN-END -->

			</div>
			<script type="text/javascript">
				dns = nvram.wan_dns.split(/\s+/);
				$('.content.lan-settings').forms([
	                { title: 'Default Gateway', name: 'lan_gateway', type: 'text', maxlen: 15, size: 17, value: nvram.lan_gateway },
	                { title: 'Static DNS', suffix: '&nbsp; <i>(IP:port)</i>', name: 'f_dns_1', type: 'text', maxlen: 21, size: 25, value: dns[0] || '0.0.0.0' },
	                { title: '', name: 'f_dns_2', type: 'text', maxlen: 21, size: 25, value: dns[1] || '0.0.0.0' },
	                /* DNSSEC-BEGIN */
	                { title: 'Enable DNSSEC', name: 'f_dnssec_enable', type: 'checkbox', suffix: ' <i>(must be supported by the upstream nameservers)</i>', value: (nvram.dnssec_enable == 1) },
	                /* DNSSEC-END */
	                /* DNSCRYPT-BEGIN */
	                { title: 'Use dnscrypt-proxy', name: 'f_dnscrypt_proxy', type: 'checkbox', value: (nvram.dnscrypt_proxy == 1) },
	                { title: 'Manual Entry', indent: 2, name: 'f_dnscrypt_manual', type: 'checkbox', value: (nvram.dnscrypt_manual == 1) },
					{ title: 'Resolver', indent: 2, name: 'dnscrypt_resolver', type: 'select', options: [/*dnscrypt_resolvers*/],  value: nvram.dnscrypt_resolver, suffix: ' <a href=\'https://github.com/jedisct1/dnscrypt-proxy/blob/master/dnscrypt-resolvers.csv\' target=\'_new\'>Resolver Details</a>' },
					{ title: 'Resolver Address', indent: 2, name: 'dnscrypt_resolver_address', type: 'text', maxlen: 50, size: 25, value: nvram.dnscrypt_resolver_address, suffix: ' <a href=\'https://github.com/jedisct1/dnscrypt-proxy/blob/master/dnscrypt-resolvers.csv\' target=\'_new\'>Resolver Details</a>' },
					{ title: 'Provider Name', indent: 2, name: 'dnscrypt_provider_name', type: 'text', maxlen: 60, size: 25, value: nvram.dnscrypt_provider_name },
					{ title: 'Provider Public Key', indent: 2, name: 'dnscrypt_provider_key', type: 'text', maxlen: 80, size: 25, value: nvram.dnscrypt_provider_key },
					{ title: 'Priority', indent: 2, name: 'dnscrypt_priority', type: 'select', options: [['1','Strict-Order'],['2','No-Resolv'],['0','None']], value: nvram.dnscrypt_priority },
					{ title: 'Local Port', indent: 2, name: 'dnscrypt_port', type: 'text', maxlen: 5, size: 7, value: nvram.dnscrypt_port },
					{ title: 'Log Level', indent: 2, name: 'dnscrypt_log', type: 'text', maxlen: 2, size: 5, value: nvram.dnscrypt_log },
					/* DNSCRYPT-END */
					{ title: 'WINS <i>(for DHCP)</i>', name: 'wan_wins', type: 'text', maxlen: 15, size: 17, value: nvram.wan_wins }
				]);
			</script>
		</div>

		<div class="box" data-box="network-ethports">
			<div class="heading">Ethernet Ports State - Configuration</div>
			<div class="content eth-ports"></div>
			<script type="text/javascript">
				$('.content.eth-ports').forms([
					{ title: 'Enable Ports State', name: 'f_lan_state', type: 'checkbox', value: (nvram.lan_state == 1) },
					{ title: 'Show Speed Info', indent: 2, name: 'f_lan_desc', type: 'checkbox', value: (nvram.lan_desc == 1) },
					{ title: 'Invert Ports Order', indent: 2, name: 'f_lan_invert', type: 'checkbox', value: (nvram.lan_invert == 1) }
				]);
			</script>
		</div>

		<div id="wifi-interfaces"></div>
		<script type="text/javascript">

			var htmlOut = '';

			for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
				//if(wl_ifaces[uidx][0].indexOf('.') < 0) {
				if (wl_sunit(uidx)<0) {
					var u = wl_unit(uidx);

					htmlOut += ('<input type="hidden" id="_wl'+u+'_mode" name="wl'+u+'_mode">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_nband" name="wl'+u+'_nband">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_wds_enable" name="wl'+u+'_wds_enable">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_wds" name="wl'+u+'_wds">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_radio" name="wl'+u+'_radio">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_closed" name="wl'+u+'_closed">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_key" name="wl'+u+'_key">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_gmode" name="wl'+u+'_gmode">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_akm" name="wl'+u+'_akm">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_auth" name="wl'+u+'_auth">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_auth_mode" name="wl'+u+'_auth_mode">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_wep" name="wl'+u+'_wep">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_lazywds" name="wl'+u+'_lazywds">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_nmode" name="wl'+u+'_nmode">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_nmcsidx" name="wl'+u+'_nmcsidx">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_nreqd" name="wl'+u+'_nreqd">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_nctrlsb" name="wl'+u+'_nctrlsb">');
					htmlOut += ('<input type="hidden" id="_wl'+u+'_nbw" name="wl'+u+'_nbw">');

					htmlOut += '<div class="box" data-box="network_wl' + u +'"><div class="heading">Wireless';
					//	if (wl_ifaces.length > 1)
					htmlOut += ' (' + wl_display_ifname(uidx) + ')';
					htmlOut += '</div>';

					htmlOut += '<div class="content wifi-' + uidx + '">';

					f = [
						{ title: 'Enable Wireless', name: 'f_wl'+u+'_radio', type: 'checkbox',
							value: (eval('nvram.wl'+u+'_radio') == '1') && (eval('nvram.wl'+u+'_net_mode') != 'disabled') },
						{ title: 'MAC Address', text: '<a href="/#advanced-mac.asp">' + eval('nvram.wl'+u+'_hwaddr') + '</a>' },
						{ title: 'Wireless Mode', name: 'f_wl'+u+'_mode', type: 'select',
							options: [['ap', 'Access Point'],['apwds', 'Access Point + WDS'],['sta', 'Wireless Client'],['wet', 'Wireless Ethernet Bridge'],['wds', 'WDS']],
							value: ((eval('nvram.wl'+u+'_mode') == 'ap') && (eval('nvram.wl'+u+'_wds_enable') == '1')) ? 'apwds' : eval('nvram.wl'+u+'_mode') },
						{ title: 'Radio Band', name: 'f_wl'+u+'_nband', type: 'select', options: bands[uidx],
							value: eval('nvram.wl'+u+'_nband') || '0' == '0' ? bands[uidx][0][0] : eval('nvram.wl'+u+'_nband') },
						{ title: 'Wireless Network Mode', name: 'wl'+u+'_net_mode', type: 'select',
							value: (eval('nvram.wl'+u+'_net_mode') == 'disabled') ? 'mixed' : eval('nvram.wl'+u+'_net_mode'),
							options: [], prefix: '<span id="__wl'+u+'_net_mode">', suffix: '</span>' },
						{ title: 'SSID', name: 'wl'+u+'_ssid', type: 'text', maxlen: 32, size: 34, value: eval('nvram.wl'+u+'_ssid') },
						{ title: 'Broadcast', indent: 2, name: 'f_wl'+u+'_bcast', type: 'checkbox', value: (eval('nvram.wl'+u+'_closed') == '0') },
						{ title: 'Channel', name: 'wl'+u+'_channel', type: 'select', options: ghz[uidx], prefix: '<div class="pull-left" id="__wl'+u+'_channel">', suffix: '</div> &nbsp; <button class="btn btn-primary" type="button" id="_f_wl'+u+'_scan" value="Scan" onclick="scanButton('+u+')">Scan <i class="icon-search"></i></button> &nbsp; <div class="spinner" style="visibility: hidden;" id="spin'+u+'"></div>',
							value: eval('nvram.wl'+u+'_channel') },
						{ title: 'Channel Width', name: 'wl'+u+'_nbw_cap', type: 'select', options: [],
							value: eval('nvram.wl'+u+'_nbw_cap'), prefix: '<span id="__wl'+u+'_nbw_cap">', suffix: '</span>' },
						{ title: 'Control Sideband', name: 'f_wl'+u+'_nctrlsb', type: 'select', options: [['lower','Lower'],['upper','Upper']],
							value: eval('nvram.wl'+u+'_nctrlsb') == 'none' ? 'lower' : eval('nvram.wl'+u+'_nctrlsb') },
						{ title: 'Security', name: 'wl'+u+'_security_mode', type: 'select',
							options: [['disabled','Disabled'],['wep','WEP'],['wpa_personal','WPA Personal'],['wpa_enterprise','WPA Enterprise'],['wpa2_personal','WPA2 Personal'],['wpa2_enterprise','WPA2 Enterprise'],['wpaX_personal','WPA / WPA2 Personal'],['wpaX_enterprise','WPA / WPA2 Enterprise'],['radius','Radius']],
							value: eval('nvram.wl'+u+'_security_mode') },
						{ title: 'Encryption', indent: 2, name: 'wl'+u+'_crypto', type: 'select',
							options: [['tkip','TKIP'],['aes','AES'],['tkip+aes','TKIP / AES']], value: eval('nvram.wl'+u+'_crypto') },
						{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_wpa_psk', type: 'password', maxlen: 64, size: 64, peekaboo: 1,
							suffix: ' <button class="btn" type="button" id="_f_wl'+u+'_psk_random1" value="Random" onclick="random_psk(\'_wl'+u+'_wpa_psk\')">Random</button>',
							value: eval('nvram.wl'+u+'_wpa_psk') },
						{ title: 'Shared Key', indent: 2, name: 'wl'+u+'_radius_key', type: 'password', maxlen: 64, size: 32, peekaboo: 1,
							suffix: ' <button class="btn" type="button" id="_f_wl'+u+'_psk_random2" value="Random" onclick="random_psk(\'_wl'+u+'_radius_key\')">Random</button>',
							value: eval('nvram.wl'+u+'_radius_key') },
						{ title: 'Group Key Renewal', indent: 2, name: 'wl'+u+'_wpa_gtk_rekey', type: 'text', maxlen: 4, size: 6, suffix: ' <i>(seconds)</i>',
							value: eval('nvram.wl'+u+'_wpa_gtk_rekey') },
						{ title: 'Radius Server', indent: 2, multi: [
							{ name: 'wl'+u+'_radius_ipaddr', type: 'text', maxlen: 15, size: 17, value: eval('nvram.wl'+u+'_radius_ipaddr') },
							{ name: 'wl'+u+'_radius_port', type: 'text', maxlen: 5, size: 7, prefix: ' : ', value: eval('nvram.wl'+u+'_radius_port') } ] },
						{ title: 'Encryption', indent: 2, name: 'wl'+u+'_wep_bit', type: 'select', options: [['128','128-bits'],['64','64-bits']],
							value: eval('nvram.wl'+u+'_wep_bit') },
						{ title: 'Passphrase', indent: 2, name: 'wl'+u+'_passphrase', type: 'text', maxlen: 16, size: 20,
							suffix: ' <button class="btn" type="button" id="_f_wl'+u+'_wep_gen" value="Generate" onclick="generate_wep('+u+')">Generate</button> <button class="btn" type="button" id="_f_wl'+u+'_wep_random" value="Random" onclick="random_wep('+u+')">Random</button>',
							value: eval('nvram.wl'+u+'_passphrase') }
					];

					for (i = 1; i <= 4; ++i)	{
						f.push(
							{ title: ('Key ' + i), indent: 2, name: ('wl'+u+'_key' + i), type: 'text', maxlen: 26, size: 34,
								suffix: '<input type="radio" onchange="verifyFields(this,1)" onclick="verifyFields(this,1)" name="f_wl'+u+'_wepidx" id="_f_wl'+u+'_wepidx_' + i + '" value="' + i + '"' + ((eval('nvram.wl'+u+'_key') == i) ? ' checked>' : '>'),
								value: nvram['wl'+u+'_key' + i] });
					}

					f.push(null,
						{ title: 'WDS', name: 'f_wl'+u+'_lazywds', type: 'select',
							options: [['0','Link With...'],['1','Automatic']], value: nvram['wl'+u+'_lazywds'] } );
					wds = eval('nvram.wl'+u+'_wds').split(/\s+/);
					for (i = 0; i < 10; i += 2)	{
						f.push({ title: (i ? '' : 'MAC Address'), indent: 2, multi: [
							{ name: 'f_wl'+u+'_wds_' + i, type: 'text', maxlen: 17, size: 20, value: wds[i] || '00:00:00:00:00:00' },
							{ name: 'f_wl'+u+'_wds_' + (i + 1), type: 'text', maxlen: 17, size: 20, value: wds[i + 1] || '00:00:00:00:00:00' } ] } );
					}


					htmlOut += createFormFields(f);
					htmlOut += '</div></div>';
				}
			}
			// for each wlif
			// Write HTML
			$('#wifi-interfaces').append(htmlOut);
		</script>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript"> earlyInit();init();</script>
</content>