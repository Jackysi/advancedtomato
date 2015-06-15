<title>Status Overview</title>
<content>
	<script type="text/javascript" src="js/wireless.jsx?_http_id=<% nv(http_id); %>"></script>
	<script type="text/javascript" src="js/interfaces.js?_http_id=<% nv(http_id); %>"></script>
	<script type="text/javascript" src="js/status-data.jsx?_http_id=<% nv(http_id); %>"></script>
	<script type="text/javascript">
		//    <% nvstat(); %>
		//    <% etherstates(); %>
		wmo = {'ap':'Access Point','sta':'Wireless Client','wet':'Wireless Ethernet Bridge','wds':'WDS'};
		auth = {'disabled':'-','wep':'WEP','wpa_personal':'WPA Personal (PSK)','wpa_enterprise':'WPA Enterprise','wpa2_personal':'WPA2 Personal (PSK)','wpa2_enterprise':'WPA2 Enterprise','wpaX_personal':'WPA / WPA2 Personal','wpaX_enterprise':'WPA / WPA2 Enterprise','radius':'Radius'};
		enc = {'tkip':'TKIP','aes':'AES','tkip+aes':'TKIP / AES'};
		bgmo = {'disabled':'-','mixed':'Auto','b-only':'B Only','g-only':'G Only','bg-mixed':'B/G Mixed','lrs':'LRS','n-only':'N Only'};
	</script>
	<script type="text/javascript">
		show_dhcpc = ((nvram.wan_proto == 'dhcp') || (((nvram.wan_proto == 'l2tp') || (nvram.wan_proto == 'pptp')) && (nvram.pptp_dhcp == '1')));
		show_codi = ((nvram.wan_proto == 'pppoe') || (nvram.wan_proto == 'l2tp') || (nvram.wan_proto == 'pptp') || (nvram.wan_proto == 'ppp3g'));

		show_radio = [];
		for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
			/* REMOVE-BEGIN
			//	show_radio.push((nvram['wl'+wl_unit(uidx)+'_radio'] == '1'));
			REMOVE-END */
			if (wl_sunit(uidx)<0)
				show_radio.push((nvram['wl'+wl_fface(uidx)+'_radio'] == '1'));
		}

		nphy = features('11n');

		function dhcpc(what)
		{
			form.submitHidden('dhcpc.cgi', { exec: what, _redirect: '/#status-home.asp' });
		}

		function serv(service, sleep)
		{
			form.submitHidden('service.cgi', { _service: service, _redirect: '/#status-home.asp', _sleep: sleep });
		}

		function wan_connect()
		{
			serv('wan-restart', 5);
		}

		function wan_disconnect()
		{
			serv('wan-stop', 2);
		}

		function wlenable(uidx, n)
		{
			form.submitHidden('wlradio.cgi', { enable: '' + n, _nextpage: 'status-overview.asp', _nextwait: n ? 6 : 3, _wl_unit: wl_unit(uidx) });
		}

		var ref = new TomatoRefresh('js/status-data.jsx', '', 0, 'status_overview_refresh');

		ref.refresh = function(text)
		{
			stats = {};
			try {
				eval(text);
			}
			catch (ex) {
				stats = {};
			}
			show();
		}


		function c(id, htm)
		{
			E(id).cells[1].innerHTML = htm;
		}

		function ethstates()
		{
			var status, speed, code = '';

			if (etherstates.port0 == "disable" || typeof (etherstates.port0) == 'undefined' || typeof (etherstates.port1) == 'undefined' || typeof (etherstates.port2) == 'undefined' || typeof (etherstates.port3) == 'undefined' || typeof (etherstates.port4) == 'undefined') {
				$('#ethernetPorts').remove();
				return false;
			}

			// Above code checks if ETH ports are Disabled/Enabled
			code += '<div id="ethPorts">';

			// WAN
			if (etherstates.port0 == "DOWN") { status = 'off'; speed = etherstates.port0.replace("DOWN","Unplugged");
			} else { status = 'on'; speed = etherstates.port0.replace('HD', 'M Half'); speed = speed.replace("FD","M Full"); }
			if (stats.lan_desc != '1') { speed = ' '; }

			code += '<div class="eth ' + status + ' wan"><div class="title">WAN</div><div class="speed">' + speed + '</div></div>';

			// LAN 1
			if (etherstates.port1 == "DOWN") { status = 'off'; speed = etherstates.port1.replace("DOWN","Unplugged");
			} else { status = 'on'; speed = etherstates.port1.replace('HD', 'M Half'); speed = speed.replace("FD","M Full"); }
			if (stats.lan_desc != '1') { speed = ' '; }

			code += '<div class="eth ' + status + '"><div class="title">LAN 1</div><div class="speed">' + speed + '</div></div>';

			// LAN 2
			if (etherstates.port2 == "DOWN") { status = 'off'; speed = etherstates.port2.replace("DOWN","Unplugged");
			} else { status = 'on'; speed = etherstates.port2.replace('HD', 'M Half'); speed = speed.replace("FD","M Full"); }
			if (stats.lan_desc != '1') { speed = ' '; }

			code += '<div class="eth ' + status + '"><div class="title">LAN 2</div><div class="speed">' + speed + '</div></div>';

			// LAN 3
			if (etherstates.port3 == "DOWN") { status = 'off'; speed = etherstates.port3.replace("DOWN","Unplugged");
			} else { status = 'on'; speed = etherstates.port3.replace('HD', 'M Half'); speed = speed.replace("FD","M Full"); }
			if (stats.lan_desc != '1') { speed = ' '; }

			code += '<div class="eth ' + status + '"><div class="title">LAN 3</div><div class="speed">' + speed + '</div></div>';

			// LAN 4
			if (etherstates.port4 == "DOWN") { status = 'off'; speed = etherstates.port4.replace("DOWN","Unplugged");
			} else { status = 'on'; speed = etherstates.port4.replace('HD', 'M Half'); speed = speed.replace("FD","M Full"); }
			if (stats.lan_desc != '1') { speed = ' '; }

			code += '<div class="eth ' + status + '"><div class="title">LAN 4</div><div class="speed">' + speed + '</div></div>';

			code += '</div>';
			$("#ethernetPorts .content").html(code);
		}

		function show() {

			c('cpu', stats.cpuload);
			c('uptime', stats.uptime);
			c('time', stats.time);
			c('wanip', stats.wanip);
			c('wanprebuf',stats.wanprebuf); //Victek
			c('wannetmask', stats.wannetmask);
			c('wangateway', stats.wangateway);
			c('dns', stats.dns);
			c('memory', stats.memory + '<div class="progress"><div class="bar" style="width: ' + stats.memoryperc + ';"></div></div>');
			c('swap', stats.swap + '<div class="progress"><div class="bar" style="width: ' + stats.swapperc + ';"></div></div>');
			elem.display('swap', stats.swap != '');

			/* IPV6-BEGIN */
			c('ip6_wan', stats.ip6_wan);
			elem.display('ip6_wan', stats.ip6_wan != '');
			c('ip6_lan', stats.ip6_lan);
			elem.display('ip6_lan', stats.ip6_lan != '');
			c('ip6_lan_ll', stats.ip6_lan_ll);
			elem.display('ip6_lan_ll', stats.ip6_lan_ll != '');
			/* IPV6-END */

			c('wanstatus', ((stats.wanstatus == 'Connected') ? '<span class="text-green">Connected</span> <i class="icon-globe"></i>' : '<span class="text-red">' + stats.wanstatus + '</span> <i class="icon-cancel"></i>'));
			c('wanuptime', stats.wanuptime);
			if (show_dhcpc) c('wanlease', stats.wanlease);
			if (show_codi) {

				if (stats.wanup) {

					$('#b_connect').hide();
					$('#b_disconnect').show();

				} else {

					$('#b_connect').show();
					$('#b_disconnect').hide();	

				}
			}

			for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
				if (wl_sunit(uidx)<0) {
					c('radio'+uidx, wlstats[uidx].radio ? 'Enabled <i class="icon-check"></i>' : 'Disabled <i class="icon-cancel"></i>');
					c('rate'+uidx, wlstats[uidx].rate);

					if (show_radio[uidx]) {

						if (wlstats[uidx].radio) {

							$('#b_wl'+uidx+'_enable').hide();
							$('#b_wl'+uidx+'_disable').show();

						} else {

							$('#b_wl'+uidx+'_enable').show();
							$('#b_wl'+uidx+'_disable').hide();

						}

					} else {

						// Interface disabled, hide enable/disable
						$('#b_wl'+uidx+'_enable').hide();
						$('#b_wl'+uidx+'_disable').hide();

					}

					c('channel'+uidx, stats.channel[uidx]);
					if (nphy) {
						c('nbw'+uidx, wlstats[uidx].nbw);
					}
					c('interference'+uidx, stats.interference[uidx]);
					elem.display('interference'+uidx, stats.interference[uidx] != '');

					if (wlstats[uidx].client) {
						c('rssi'+uidx, wlstats[uidx].rssi || '');
						c('noise'+uidx, wlstats[uidx].noise || '');
						c('qual'+uidx, stats.qual[uidx] || '');
					}
				}
				c('ifstatus'+uidx, wlstats[uidx].ifstatus || '');
			}
		}

		function earlyInit()
		{
			elem.display('b_dhcpc', show_dhcpc);
			elem.display('b_connect', 'b_disconnect', show_codi);
			if (nvram.wan_proto == 'disabled')
				elem.display('wan-title', 'sesdiv_wan', 0);
			for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {
				if (wl_sunit(uidx)<0)
					$('#b_wl'+wl_fface(uidx)+'_enable').closest('.btn-control-group').show();
			}

			ethstates();
			init();
		}

		function init() {

			$('.refresher').after(genStdRefresh(1,1,'ref.toggle()'));
			ref.initPage(3000, 3);
			show();

		}

	</script>

	<div class="fluid-grid">

		<div class="box" data-box="home_systembox">
			<div class="heading">System</div>
			<div class="content" id="sesdiv_system">
				<div class="section"></div>
				<script type="text/javascript">
					var a = nvstat.free / nvstat.size * 100.0;
					createFieldTable('', [
						{ title: 'Name', text: nvram.router_name },
						{ title: 'Model', text: nvram.t_model_name },
						{ title: 'Chipset', text: stats.systemtype },
						{ title: 'CPU Freq', text: stats.cpumhz },
						{ title: 'Flash Size', text: stats.flashsize },
						null,
						{ title: 'Time', rid: 'time', text: stats.time },
						{ title: 'Uptime', rid: 'uptime', text: stats.uptime },
						{ title: 'CPU Load <small>(1 / 5 / 15 mins)</small>', rid: 'cpu', text: stats.cpuload },
						{ title: 'Total / Free Memory', rid: 'memory', text: stats.memory + '<div class="progress"><div class="bar" style="width: ' + stats.memoryperc + ';"></div></div>' },
						{ title: 'Total / Free Swap', rid: 'swap', text: stats.swap + '<div class="progress"><div class="bar" style="width: ' + stats.swapperc + ';"></div></div>', hidden: (stats.swap == '') },
						{ title: 'Total / Free NVRAM', text: scaleSize(nvstat.size) + ' / ' + scaleSize(nvstat.free) + ' <small>(' + (a).toFixed(2) + '%)</small> <div class="progress"><div class="bar" style="width: ' + (a).toFixed(2) + '%;"></div></div>' },
						], '#sesdiv_system', 'data-table dataonly');
				</script>
			</div>
		</div>

		<div class="box" id="wan-title" data-box="home_wanbox">
			<div class="heading">WAN</div>
			<div class="content" id="sesdiv_wan">
				<div class="WANField"></div>
				<script type="text/javascript">
					createFieldTable('', [
						{ title: 'MAC Address', text: nvram.wan_hwaddr },
						{ title: 'Connection Type', text: { 'dhcp':'DHCP', 'static':'Static IP', 'pppoe':'PPPoE', 'pptp':'PPTP', 'l2tp':'L2TP', 'ppp3g':'3G Modem' }[nvram.wan_proto] || '-' },
						{ title: 'IP Address', rid: 'wanip', text: stats.wanip },
						{ title: 'Previous WAN IP', rid: 'wanprebuf', text: stats.wanprebuf, hidden: ((nvram.wan_proto != 'pppoe') && (nvram.wan_proto != 'pptp') && (nvram.wan_proto != 'l2tp') && (nvram.wan_proto != 'ppp3g')) }, //Victek
						{ title: 'Subnet Mask', rid: 'wannetmask', text: stats.wannetmask },
						{ title: 'Gateway', rid: 'wangateway', text: stats.wangateway },
						/* IPV6-BEGIN */
						{ title: 'IPv6 Address', rid: 'ip6_wan', text: stats.ip6_wan, hidden: (stats.ip6_wan == '') },
						/* IPV6-END */
						{ title: 'DNS', rid: 'dns', text: stats.dns },
						{ title: 'MTU', text: nvram.wan_run_mtu },
						null,
						{ title: 'Status', rid: 'wanstatus', text: ((stats.wanstatus == 'Connected') ? 'Connected <i class="icon-globe"></i>' : stats.wanstatus + ' <i class="icon-cancel icon-red"></i>') },
						{ title: 'Connection Uptime', rid: 'wanuptime', text: stats.wanuptime },
						{ title: 'Remaining Lease Time', rid: 'wanlease', text: stats.wanlease, ignore: !show_dhcpc }
						], '.WANField', 'data-table dataonly');
				</script>

				<button type="button" class="btn btn-primary pull-left" onclick="wan_connect()" value="Connect" id="b_connect" style="display:none">Connect <i class="icon-check"></i></button>
				<button type="button" class="btn btn-danger pull-left" onclick="wan_disconnect()" value="Disconnect" id="b_disconnect" style="display:none">Disconnect <i class="icon-cancel"></i></button>

				<div id="b_dhcpc" class="btn-group pull-left" style="margin-left: 5px; display:none;">
					<button type="button" class="btn" onclick="dhcpc('renew')" value="Renew">Renew</button>
					<button type="button" class="btn" onclick="dhcpc('release')" value="Release">Release</button>
				</div>

				<div class="clearfix"></div>

			</div>
		</div>

		<div class="box" id="ethernetPorts" data-box="home_ethports">
			<div class="heading">Ethernet Ports State
				<a class="ajaxload pull-right" data-toggle="tooltip" title="Configure Settings" href="#basic-network.asp"><i class="icon-system"></i></a>
			</div>
			<div class="content" id="sesdiv_lan-ports"></div>
		</div>

		<div class="box" id="LAN-settings" data-box="home_lanbox">
			<div class="heading">LAN </div>
			<div class="content" id="sesdiv_lan">
				<script type="text/javascript">

					/* VLAN-BEGIN */
					var s='';
					var t='';
					for (var i = 0 ; i <= MAX_BRIDGE_ID; i++) {
						var j = (i == 0) ? '' : i.toString();
						if (nvram['lan' + j + '_ifname'].length > 0) {
							if (nvram['lan' + j + '_proto'] == 'dhcp') {
								if ((!fixIP(nvram.dhcpd_startip)) || (!fixIP(nvram.dhcpd_endip))) {
									var x = nvram['lan' + j + '_ipaddr'].split('.').splice(0, 3).join('.') + '.';
									nvram['dhcpd' + j + '_startip'] = x + nvram['dhcp' + j + '_start'];
									nvram['dhcpd' + j + '_endip'] = x + ((nvram['dhcp' + j + '_start'] * 1) + (nvram['dhcp' + j + '_num'] * 1) - 1);
								}
								s += ((s.length>0)&&(s.charAt(s.length-1) != ' ')) ? ', ' : '';
								s += '<a class="ajaxload" href="#status-devices.asp">' + nvram['dhcpd' + j + '_startip'] + ' - ' + nvram['dhcpd' + j + '_endip'] + '</a> on LAN' + j + ' (br' + i + ')';
							} else {
								s += ((s.length>0)&&(s.charAt(s.length-1) != ' ')) ? ', ' : '';
								s += 'Disabled on LAN' + j + ' (br' + i + ')';
							}
							t += ((t.length>0)&&(t.charAt(t.length-1) != ' ')) ? ', ' : '';
							t += nvram['lan' + j + '_ipaddr'] + '/' + numberOfBitsOnNetMask(nvram['lan' + j + '_netmask']) + ' on LAN' + j + ' (br' + i + ')';
						}
					}

					createFieldTable('', [
						{ title: 'Gateway', text: nvram.lan_gateway, ignore: nvram.wan_proto != 'disabled' },
						/* IPV6-BEGIN */
						{ title: 'Router IPv6 Address', rid: 'ip6_lan', text: stats.ip6_lan, ignore: stats.ip6_lan == '' },
						{ title: 'IPv6 Link-local Address', rid: 'ip6_lan_ll', text: stats.ip6_lan_ll, ignore: stats.ip6_lan_ll == '' },
						/* IPV6-END */
						{ title: 'DNS', rid: 'dns', text: stats.dns, ignore: nvram.wan_proto != 'disabled' },
						{ title: 'DHCP', text: s }
						], '#sesdiv_lan', 'data-table dataonly');
					/* VLAN-END */

					/* NOVLAN-BEGIN */
					if (nvram.lan_proto == 'dhcp') {
						if ((!fixIP(nvram.dhcpd_startip)) || (!fixIP(nvram.dhcpd_endip))) {
							var x = nvram.lan_ipaddr.split('.').splice(0, 3).join('.') + '.';
							nvram.dhcpd_startip = x + nvram.dhcp_start;
							nvram.dhcpd_endip = x + ((nvram.dhcp_start * 1) + (nvram.dhcp_num * 1) - 1);
						}
						s = '<a class="ajaxload" href="#status-devices.asp">' + nvram.dhcpd_startip + ' - ' + nvram.dhcpd_endip + '</a>';
					}
					else {
						s = 'Disabled';
					}
					createFieldTable('', [
						{ title: 'Router MAC Address', text: nvram.et0macaddr },
						{ title: 'Router IP Address', text: nvram.lan_ipaddr },
						{ title: 'Subnet Mask', text: nvram.lan_netmask },
						{ title: 'Gateway', text: nvram.lan_gateway, ignore: nvram.wan_proto != 'disabled' },
						/* IPV6-BEGIN */
						{ title: 'Router IPv6 Address', rid: 'ip6_lan', text: stats.ip6_lan, hidden: (stats.ip6_lan == '') },
						{ title: 'IPv6 Link-local Address', rid: 'ip6_lan_ll', text: stats.ip6_lan_ll, hidden: (stats.ip6_lan_ll == '') },
						/* IPV6-END */
						{ title: 'DNS', rid: 'dns', text: stats.dns, ignore: nvram.wan_proto != 'disabled' },
						{ title: 'DHCP', text: s }
						], '#sesdiv_lan', 'data-table dataonly');
					/* NOVLAN-END */

				</script>
			</div>
		</div>

		<script type="text/javascript">

			for (var uidx = 0; uidx < wl_ifaces.length; ++uidx) {

				var data = "";

				/* REMOVE-BEGIN
				//	u = wl_unit(uidx);
				REMOVE-END */
				u = wl_fface(uidx);
				data += '<div class="box" data-box="home_wl' + u +'"><div class="heading" id="wl'+u+'-title">Wireless';
				if (wl_ifaces.length > 0)
					data += ' (' + wl_display_ifname(uidx) + ')';
				data += '</div>';
				data += '<div class="content" id="sesdiv_wl_'+u+'">';
				sec = auth[nvram['wl'+u+'_security_mode']] + '';
				if (sec.indexOf('WPA') != -1) sec += ' + ' + enc[nvram['wl'+u+'_crypto']];

				wmode = wmo[nvram['wl'+u+'_mode']] + '';
				if ((nvram['wl'+u+'_mode'] == 'ap') && (nvram['wl'+u+'_wds_enable'] * 1)) wmode += ' + WDS';

				data += createFieldTable('', [
					{ title: 'MAC Address', text: nvram['wl'+u+'_hwaddr'] },
					{ title: 'Wireless Mode', text: wmode },
					{ title: 'Wireless Network Mode', text: bgmo[nvram['wl'+u+'_net_mode']], ignore: (wl_sunit(uidx)>=0) },
					{ title: 'Interface Status', rid: 'ifstatus'+uidx, text: wlstats[uidx].ifstatus },
					{ title: 'Radio', rid: 'radio'+uidx, text: (wlstats[uidx].radio == 0) ? 'Disabled <i class="icon-cancel"></i>' : 'Enabled <i class="icon-check"></i>', ignore: (wl_sunit(uidx)>=0) },
					/* REMOVE-BEGIN */
					//	{ title: 'SSID', text: (nvram['wl'+u+'_ssid'] + ' <small><i>' + ((nvram['wl'+u+'_mode'] != 'ap') ? '' : ((nvram['wl'+u+'_closed'] == 0) ? '(Broadcast Enabled)' : '(Broadcast Disabled)')) + '</i></small>') },
					/* REMOVE-END */
					{ title: 'SSID', text: nvram['wl'+u+'_ssid'] },
					{ title: 'Broadcast', text: (nvram['wl'+u+'_closed'] == 0) ? '<span class="text-green">Enabled <i class="icon-check"></i></span>' : '<span class="text-red">Disabled <i class="icon-cancel"></i></span>', ignore: (nvram['wl'+u+'_mode'] != 'ap') },
					{ title: 'Security', text: sec },
					{ title: 'Channel', rid: 'channel'+uidx, text: stats.channel[uidx], ignore: (wl_sunit(uidx)>=0) },
					{ title: 'Channel Width', rid: 'nbw'+uidx, text: wlstats[uidx].nbw, ignore: ((!nphy) || (wl_sunit(uidx)>=0)) },
					{ title: 'Interference Level', rid: 'interference'+uidx, text: stats.interference[uidx], hidden: ((stats.interference[uidx] == '') || (wl_sunit(uidx)>=0)) },
					{ title: 'Rate', rid: 'rate'+uidx, text: wlstats[uidx].rate, ignore: (wl_sunit(uidx)>=0) },
					{ title: 'RSSI', rid: 'rssi'+uidx, text: wlstats[uidx].rssi || '', ignore: ((!wlstats[uidx].client) || (wl_sunit(uidx)>=0)) },
					{ title: 'Noise', rid: 'noise'+uidx, text: wlstats[uidx].noise || '', ignore: ((!wlstats[uidx].client) || (wl_sunit(uidx)>=0)) },
					{ title: 'Signal Quality', rid: 'qual'+uidx, text: stats.qual[uidx] || '', ignore: ((!wlstats[uidx].client) || (wl_sunit(uidx)>=0)) }
					], null, 'data-table dataonly');

				data += '<div class="btn-control-group" style="display: none;">';
				data += '<button type="button" class="btn btn-primary" onclick="wlenable('+uidx+', 1)" id="b_wl'+uidx+'_enable" value="Enable">Enable <i class="icon-check"></i></button>';
				data += '<button type="button" class="btn btn-danger" onclick="wlenable('+uidx+', 0)" id="b_wl'+uidx+'_disable" value="Disable">Disable <i class="icon-disable"></i></button>';
				data += '</div></div></div>';
				$('#LAN-settings').after(data);
			}
		</script>
	</div>

	<div class="clearfix refresher"></div>
	<script type="text/javascript">earlyInit();</script>
</content>