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
		show_dhcpc = [];
		show_codi = [];
		for ( var uidx = 1; uidx <= nvram.mwan_num; ++uidx ) {
			var u;
			u     = (uidx > 1) ? uidx : '';
			proto = nvram[ 'wan' + u + '_proto' ];
			if ( proto != 'disabled' ) show_langateway = 0;
			show_dhcpc[ uidx - 1 ] = ((proto == 'dhcp') || (proto == 'lte') || (((proto == 'l2tp') || (proto == 'pptp')) && (nvram.pptp_dhcp == '1')));
			show_codi[ uidx - 1 ]  = ((proto == 'pppoe') || (proto == 'l2tp') || (proto == 'pptp') || (proto == 'ppp3g'));
		}

		show_radio = [];
		for ( var uidx = 0; uidx < wl_ifaces.length; ++uidx ) {
			/* REMOVE-BEGIN
			 //	show_radio.push((nvram['wl'+wl_unit(uidx)+'_radio'] == '1'));
			 REMOVE-END */
			if ( wl_sunit( uidx ) < 0 )
				show_radio.push( (nvram[ 'wl' + wl_fface( uidx ) + '_radio' ] == '1') );
		}

		nphy = features( '11n' );

		function dhcpc( what, wan_prefix ) {
			form.submitHidden( 'dhcpc.cgi', { exec: what, prefix: wan_prefix, _redirect: '/#status-home.asp' } );
		}

		function serv( service, sleep ) {
			form.submitHidden( 'service.cgi', { _service: service, _redirect: '/#status-home.asp', _sleep: sleep } );
		}

		function wan_connect( uidx ) {
			serv( 'wan' + uidx + '-restart', 5 );
		}

		function wan_disconnect( uidx ) {
			serv( 'wan' + uidx + '-stop', 2 );
		}

		function wlenable( uidx, n ) {
			form.submitHidden( 'wlradio.cgi', { enable: '' + n, _nextpage: 'status-overview.asp', _nextwait: n ? 6 : 3, _wl_unit: wl_unit( uidx ) } );
		}

		var ref = new TomatoRefresh( 'js/status-data.jsx', '', 0, 'status_overview_refresh' );

		ref.refresh = function( text ) {
			stats = {};
			try {
				eval( text );
			}
			catch ( ex ) {
				stats = {};
			}
			show();
		}


		function c( id, htm ) {
			E( id ).cells[ 1 ].innerHTML = htm;
		}

		function ethstates() {

			var ports = [], names = {}, v = 0;

			// Fail safe (check if minimum 5 ports exist (bit lame since not all routers have 5 ports???)
			if ( etherstates.port0 == "disable" || typeof (etherstates.port0) == 'undefined' || typeof (etherstates.port1) == 'undefined' || typeof (etherstates.port2) == 'undefined' || typeof (etherstates.port3) == 'undefined' || typeof (etherstates.port4) == 'undefined' ) {

				$( '#ethernetPorts' ).remove();
				return false;

			}

			// Name ports (MultiWAN thing)
			for ( uidx = 1; uidx <= nvram.mwan_num; ++uidx ) {
				u = (uidx > 1) ? uidx : '';
				if ( (nvram[ 'wan' + u + '_sta' ] == '') && (nvram[ 'wan' + u + '_proto' ] != 'lte') && (nvram[ 'wan' + u + '_proto' ] != 'ppp3g') ) {
					names[ 'port' + v ] = 'WAN' + u;
					++v;
				}
			}

			for ( uidx = v; uidx <= 4; ++uidx ) { names[ 'port' + uidx ] = 'LAN' + uidx; }

			// F*** standard approach writing down port by port, lets just loop through array....
			$.each( etherstates, function( $k, $v ) {

				var speed, status;

				// Replace status/speed based on port status
				if ( $v == 'DOWN' ) {

					status = 'off';
					speed  = etherstates[ $k ].replace( "DOWN", "Unplugged" );

				} else {

					status = 'on';
					speed  = etherstates[ $k ].replace( 'HD', 'M Half' );
					speed  = speed.replace( "FD", "M Full" );

				}

				ports.push( '<div class="eth ' + status + ' ' + ( ( $k == 'port0' ) ? 'wan' : '' ) + '"><div class="title">' + names[$k] + '</div><div class="speed">' + speed + '</div></div>' );

			});

			$( "#ethernetPorts .content" ).html( '<div id="ethPorts">' + ports.join( '' ) + '</div>' );

		}

		function show() {

			c( 'cpu', stats.cpuload );
			c( 'uptime', stats.uptime );
			c( 'time', stats.time );
			c( 'memory', stats.memory + '<div class="progress"><div class="bar" style="width: ' + stats.memoryperc + ';"></div></div>' );
			c( 'swap', stats.swap + '<div class="progress"><div class="bar" style="width: ' + stats.swapperc + ';"></div></div>' );
			elem.display( 'swap', stats.swap != '' );

/* IPV6-BEGIN */
			c( 'ip6_wan', stats.ip6_wan );
			elem.display( 'ip6_wan', stats.ip6_wan != '' );
			c( 'ip6_lan', stats.ip6_lan );
			elem.display( 'ip6_lan', stats.ip6_lan != '' );
			c( 'ip6_lan_ll', stats.ip6_lan_ll );
			elem.display( 'ip6_lan_ll', stats.ip6_lan_ll != '' );
/* IPV6-END */

			for ( uidx = 1; uidx <= nvram.mwan_num; ++uidx ) {

				var u = (uidx > 1) ? uidx : '';

				c( 'wan' + u + 'ip', stats.wanip[ uidx - 1 ] );
				c( 'wan' + u + 'netmask', stats.wannetmask[ uidx - 1 ] );
				c( 'wan' + u + 'gateway', stats.wangateway[ uidx - 1 ] );
				c( 'wan' + u + 'dns', stats.dns[ uidx - 1 ] );
				c( 'wan' + u + 'uptime', stats.wanuptime[ uidx - 1 ] );
				c( 'wan' + u + 'status', ( ( stats.wanstatus[ uidx - 1 ] == 'Connected') ? '<span class="text-green">Connected</span>' : '<span class="text-red">' + stats.wanstatus[ uidx - 1 ] + '</span>') );
				if ( show_dhcpc[ uidx - 1 ] ) c( 'wan' + u + 'lease', stats.wanlease[ uidx - 1 ] );
				if ( show_dhcpc[ uidx - 1 ] ) c( 'wan' + u + 'lease', stats.wanlease[ uidx - 1 ] );
				if ( show_codi ) {

					if ( stats.wanup[ uidx - 1 ] ) {

						$( '#b' + u + '_connect_').hide();
						$( '#b' + u + '_disconnect' ).show();

					} else {

						$( '#b' + u + '_connect' + u ).show();
						$( '#b' + u + '_disconnect' + u ).hide();

					}
				}
			}

			for ( uidx = 0; uidx < wl_ifaces.length; ++uidx ) {
				if ( wl_sunit( uidx ) < 0 ) {
					c( 'radio' + uidx, wlstats[ uidx ].radio ? 'Enabled <i class="icon-check"></i>' : 'Disabled <i class="icon-cancel"></i>' );
					c( 'rate' + uidx, wlstats[ uidx ].rate );

					if ( show_radio[ uidx ] ) {

						if ( wlstats[ uidx ].radio ) {

							$( '#b_wl' + uidx + '_enable' ).hide();
							$( '#b_wl' + uidx + '_disable' ).show();

						} else {

							$( '#b_wl' + uidx + '_enable' ).show();
							$( '#b_wl' + uidx + '_disable' ).hide();

						}

					} else {

						// Interface disabled, hide enable/disable
						$( '#b_wl' + uidx + '_enable' ).hide();
						$( '#b_wl' + uidx + '_disable' ).hide();

					}

					c( 'channel' + uidx, stats.channel[ uidx ] );
					if ( nphy ) {
						c( 'nbw' + uidx, wlstats[ uidx ].nbw );
					}
					c( 'interference' + uidx, stats.interference[ uidx ] );
					elem.display( 'interference' + uidx, stats.interference[ uidx ] != '' );

					if ( wlstats[ uidx ].client ) {
						c( 'rssi' + uidx, wlstats[ uidx ].rssi || '' );
						c( 'noise' + uidx, wlstats[ uidx ].noise || '' );
						c( 'qual' + uidx, stats.qual[ uidx ] || '' );
					}
				}
				c( 'ifstatus' + uidx, wlstats[ uidx ].ifstatus || '' );
			}
		}

		function earlyInit() {

			var uidx;
			for ( uidx = 1; uidx <= nvram.mwan_num; ++uidx ) {
				var u = (uidx > 1) ? uidx : '';
				elem.display( 'b' + u + '_dhcpc', show_dhcpc[ uidx - 1 ] );
				elem.display( 'b' + u + '_connect', 'b' + u + '_disconnect', show_codi[ uidx - 1 ] );
				// elem.display( 'wan' + u + '-title', 'sesdiv_wan' + u, (nvram[ 'wan' + u + '_proto' ] != 'disabled') );
			}
			for ( uidx = 0; uidx < wl_ifaces.length; ++uidx ) {
				if ( wl_sunit( uidx ) < 0 )
					elem.display( 'b_wl' + uidx + '_enable', 'b_wl' + uidx + '_disable', show_radio[ uidx ] );
			}

			ethstates();
			show();
			init();
		}

		function init() {

			$( '.refresher' ).after( genStdRefresh( 1, 1, 'ref.toggle()' ) );
			ref.initPage( 3000, 3 );

		}

	</script>

	<div class="fluid-grid">

		<div class="box" data-box="home_systembox">
			<div class="heading">System</div>
			<div class="content" id="sesdiv_system">
				<div class="section"></div>
				<script type="text/javascript">
					var a = (nvstat.size - nvstat.free) / nvstat.size * 100.0;
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
						{ title: 'Memory Usage', rid: 'memory', text: stats.memory + '<div class="progress"><div class="bar" style="width: ' + stats.memoryperc + ';"></div></div>' },
						{ title: 'Swap Usage', rid: 'swap', text: stats.swap + '<div class="progress"><div class="bar" style="width: ' + stats.swapperc + ';"></div></div>', hidden: (stats.swap == '') },
						{ title: 'NVRAM Usage', text: scaleSize(nvstat.size - nvstat.free) + ' <small>/</small> ' + scaleSize(nvstat.size) + ' (' + (a).toFixed(2) + '%) <div class="progress"><div class="bar" style="width: ' + (a).toFixed(2) + '%;"></div></div>' },
						], '#sesdiv_system', 'data-table dataonly');
				</script>
			</div>
		</div>

		<div class="MultiWAN"></div>
		<script type="text/javascript">

			for ( uidx = 1; uidx <= nvram.mwan_num; ++uidx ) {

				var u = (uidx > 1) ? uidx : '';
				$( '.MultiWAN' ).append( '<div class="box" id="wan-title' + u + '" data-box="home_wanbox' + u + '"><div class="heading">WAN ' + u + '</div>' +
				                         '<div class="content" id="sesdiv_wan' + u + '"></div></div>' );

				createFieldTable( '', [
					{ title: 'MAC Address', text: nvram[ 'wan' + u + '_hwaddr' ] },
					{ title: 'Connection Type', text: { 'dhcp': 'DHCP', 'static': 'Static IP', 'pppoe': 'PPPoE', 'pptp': 'PPTP', 'l2tp': 'L2TP', 'ppp3g': '3G Modem', 'lte': '4G/LTE' }[ nvram[ 'wan' + u + '_proto' ] ] || '-' },
					{ title: 'IP Address', rid: 'wan' + u + 'ip', text: stats.wanip[ uidx - 1 ] },
					{ title: 'Subnet Mask', rid: 'wan' + u + 'netmask', text: stats.wannetmask[ uidx - 1 ] },
					{ title: 'Gateway', rid: 'wan' + u + 'gateway', text: stats.wangateway[ uidx - 1 ] },
					/* IPV6-BEGIN */
					{ title: 'IPv6 Address', rid: 'ip6_wan', text: stats.ip6_wan, hidden: (stats.ip6_wan == '') },
					/* IPV6-END */
					{ title: 'DNS', rid: 'wan' + u + 'dns', text: stats.dns[ uidx - 1 ] },
					{ title: 'MTU', text: nvram[ 'wan' + u + '_run_mtu' ] },
					null,
					{ title: 'Status', rid: 'wan' + u + 'status', text: stats.wanstatus[ uidx - 1 ] },
					{ title: 'Connection Uptime', rid: 'wan' + u + 'uptime', text: stats.wanuptime[ uidx - 1 ] },
					{ title: 'Remaining Lease Time', rid: 'wan' + u + 'lease', text: stats.wanlease[ uidx - 1 ], ignore: !show_dhcpc[ uidx - 1 ] }
				], '#sesdiv_wan' + u, 'data-table dataonly' );

				$( '#sesdiv_wan' + u ).append(
						'<br><button type="button" class="btn btn-primary pull-left" onclick="wan_connect(' + uidx + ')" value="Connect" id="b' + u + '_connect" style="display:none;margin-right: 5px;">Connect <i class="icon-reboot"></i></button>' +
						'<button type="button" class="btn btn-danger pull-left" onclick="wan_disconnect(' + uidx + ')" value="Disconnect" id="b' + u + '_disconnect" style="display:none;margin-right: 5px;">Disconnect <i class="icon-cancel"></i></button>' +
						'<div id="b' + u + '_dhcpc" class="btn-group pull-left" style="display:none;"><button type="button" class="btn" onclick="dhcpc(\'renew\', \'wan' + u + '\')" value="Renew">Renew</button>' +
						'<button type="button" class="btn" onclick="dhcpc(\'release\', \'wan' + u + '\')" value="Release">Release</button><div class="clearfix"></div></div>'
				);

			}

		</script>

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

					function h_countbitsfromleft(num) {
						if (num == 255 ){
							return(8);
						}
						var i = 0;
						var bitpat=0xff00;
						while (i < 8){
							if (num == (bitpat & 0xff)){
								return(i);
							}
							bitpat=bitpat >> 1;
							i++;
						}
						return(Number.NaN);
					}

					function numberOfBitsOnNetMask(netmask) {
						var total = 0;
						var t = netmask.split('.');
						for (var i = 0; i<= 3 ; i++) {
							total += h_countbitsfromleft(t[i]);
						}
						return total;
					}

					var s='';
					var t='';
					for (var i = 0 ; i <= MAX_BRIDGE_ID ; i++) {
						var j = (i == 0) ? '' : i.toString();
						if (nvram['lan' + j + '_ifname'].length > 0) {
							if (nvram['lan' + j + '_proto'] == 'dhcp') {
								if ((!fixIP(nvram.dhcpd_startip)) || (!fixIP(nvram.dhcpd_endip))) {
									var x = nvram['lan' + j + '_ipaddr'].split('.').splice(0, 3).join('.') + '.';
									nvram['dhcpd' + j + '_startip'] = x + nvram['dhcp' + j + '_start'];
									nvram['dhcpd' + j + '_endip'] = x + ((nvram['dhcp' + j + '_start'] * 1) + (nvram['dhcp' + j + '_num'] * 1) - 1);
								}
								s += ((s.length>0)&&(s.charAt(s.length-1) != ' ')) ? '<br>' : '';
								s += '<b>br' + i + '</b> (LAN' + j + ') - ' + nvram['dhcpd' + j + '_startip'] + ' - ' + nvram['dhcpd' + j + '_endip'];
							} else {
								s += ((s.length>0)&&(s.charAt(s.length-1) != ' ')) ? '<br>' : '';
								s += '<b>br' + i + '</b> (LAN' + j + ') - Disabled';
							}
							t += ((t.length>0)&&(t.charAt(t.length-1) != ' ')) ? '<br>' : '';
							t += '<b>br' + i + '</b> (LAN' + j + ') - ' + nvram['lan' + j + '_ipaddr'] + '/' + numberOfBitsOnNetMask(nvram['lan' + j + '_netmask']);

						}
					}

					createFieldTable('', [
						{ title: 'Router MAC Address', text: nvram.et0macaddr },
						{ title: 'Router IP Addresses', text: t },
						{ title: 'Gateway', text: nvram.lan_gateway, ignore: nvram.wan_proto != 'disabled' },
						/* IPV6-BEGIN */
						{ title: 'Router IPv6 Address', rid: 'ip6_lan', text: stats.ip6_lan, hidden: (stats.ip6_lan == '') },
						{ title: 'IPv6 Link-local Address', rid: 'ip6_lan_ll', text: stats.ip6_lan_ll, hidden: (stats.ip6_lan_ll == '') },
						/* IPV6-END */
						{ title: 'DNS', rid: 'dns', text: stats.dns, ignore: nvram.wan_proto != 'disabled' },
						{ title: 'DHCP', text: s }
						], '#sesdiv_lan', 'data-table dataonly' );

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

				data += '<div class="btn-control-group"><br>';
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