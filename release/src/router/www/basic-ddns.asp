<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Dynamic DNS</title>
<content>
	<script type="text/javascript">
		//<% nvram("ddnsx0,ddnsx1,ddnsx_ip,wan_dns,wan_get_dns,dns_addget,ddnsx_refresh,ddnsx_save"); %>
		//<% ddnsx(); %>

		/* REMOVE-BEGIN
		 t = hostname (top)
		 u = username/password
		 p = password
		 h = hostname
		 j = hostname (optional)
		 c = custom url
		 w = wildcard
		 m = MX
		 b = backup MX
		 o = use OpenDNS
		 a = freedns.afraid
		 z = can't use client-given IP address
		 s = save state checkbox
		 REMOVE-END */

		var services = [
			[ '', 'None', '', '' ],
			[ '3322', '3322', 'http://www.3322.org/', 'uhwmb' ],
			[ '3322-static', '3322 - Static', 'http://www.3322.org/', 'uhwmb' ],
			[ 'dnsexit', 'DNS Exit', 'http://www.dnsexit.com/', 'uh' ],
			[ 'dnsomatic', 'DNS-O-Matic', 'http://www.dnsomatic.com/', 'uj' ],
			[ 'dyndns', 'DynDNS - Dynamic', 'http://www.dyndns.com/', 'uhwmbs' ],
			[ 'dyndns-static', 'DynDNS - Static', 'http://www.dyndns.com/', 'uhwmbs' ],
			[ 'dyndns-custom', 'DynDNS - Custom', 'http://www.dyndns.com/', 'uhwmbs' ],
			[ 'sdyndns', 'DynDNS (https) - Dynamic', 'http://www.dyndns.com/', 'uhwmbs' ],
			[ 'sdyndns-static', 'DynDNS (https) - Static', 'http://www.dyndns.com/', 'uhwmbs' ],
			[ 'sdyndns-custom', 'DynDNS (https) - Custom', 'http://www.dyndns.com/', 'uhwmbs' ],
			[ 'dyns', 'DyNS', 'http://www.dyns.cx/', 'uh' ],
			[ 'easydns', 'easyDNS', 'http://www.easydns.com/', 'uhwm' ],
			[ 'seasydns', 'easyDNS (https)', 'http://www.easydns.com/', 'uhwm' ],
			[ 'editdns', 'EditDNS', 'http://www.editdns.net/', 'tpz' ],
			[ 'everydns', 'EveryDNS', 'http://www.everydns.net/', 'uj', null, null, 'Domain <small>(optional)</small>' ],
			[ 'minidns', 'miniDNS', 'http://www.minidns.net/', 'uh' ],
			[ 'enom', 'eNom', 'http://www.enom.com/', 'ut', 'Domain' ],
			[ 'afraid', 'FreeDNS (afraid.org)', 'http://freedns.afraid.org/', 'az' ],
			[ 'heipv6tb', 'HE.net IPv6 Tunnel Broker', 'http://www.tunnelbroker.net/', 'uh', 'User ID <small>(not your username)</small>', null, 'Global Tunnel ID' ],
			[ 'ieserver', 'ieServer.net', 'http://www.ieserver.net/', 'uhz', 'Username / Hostname', null, 'Domain' ],
			[ 'namecheap', 'namecheap', 'http://www.namecheap.com/', 'ut', 'Domain' ],
			[ 'noip', 'No-IP.com', 'http://www.no-ip.com/', 'uh', 'Email Address', null, 'Hostname / Group' ],
			[ 'opendns', 'OpenDNS', 'http://www.opendns.com/', 'uhoz', null, null, 'Network <small>(optional)</small>' ],
			[ 'tzo', 'TZO', 'http://www.tzo.com/', 'uh', 'Email Address', 'Password' ],
			[ 'zoneedit', 'ZoneEdit', 'http://www.zoneedit.com/', 'uh' ],
			[ 'szoneedit', 'ZoneEdit (https)', 'http://www.zoneedit.com/', 'uh' ],
			[ 'pairnic', 'pairNIC', 'http://www.pairnic.com/', 'uh' ],
			[ 'spairnic', 'pairNIC (https)', 'http://www.pairnic.com/', 'uh' ],
			[ 'ovh', 'OVH', 'http://www.ovh.com/', 'uh' ],
			[ 'sovh', 'OVH (https)', 'https://www.ovh.com/', 'uh' ],
			[ 'schangeip', 'ChangeIP (https)', 'https://www.changeip.com/', 'uh' ],
			[ 'custom', 'Custom URL', '', 'c' ] ];

		var opendns = [ '208.67.222.222', '208.67.220.220' ];
		var opendnsInUse = 0;

		function msgLoc( s ) {
			var r;

			s = s.trim().replace( /\n+/g, ' ' );
			if ( r = s.match( /^(.*?): (.*)/ ) ) {
				r[ 2 ] = r[ 2 ].replace( /#RETRY (\d+) (\d+)/,
				                         function( s, min, num ) {
					                         return '<br><small>(' + ((num >= 1) ? (num + '/3: ') : '') + 'Automatically retrying in ' + min + ' minutes)</small>';
				                         }
				);
				return '<small>' + (new Date( r[ 1 ] )).toLocaleString() + ':</small><br>' + r[ 2 ];
			}
			else if ( s.length == 0 ) {
				return '-';
			}
			return s;
		}

		function mop( s ) {
			var op, i;

			op = {};
			for ( i = s.length - 1; i >= 0; --i ) {
				op[ s.charAt( i ) ] = 1;
			}

			return op;
		}

		function verifyFields( focused, quiet ) {
			var i;
			var data, r, e;
			var op;
			var enabled;
			var b;

			b = E( '_f_ddnsx_ip' ).value == 'custom';
			elem.display( PR( '_f_custom_ip' ), b );
			if ( (b) && (!v_ip( '_f_custom_ip', quiet )) ) return 0;

			if ( !v_range( '_ddnsx_refresh', quiet, 0, 90 ) ) return 0;

			r = 1;
			for ( i = 0; i < 2; ++i ) {
				data = services[ E( '_f_service' + i ).selectedIndex ] || services[ 0 ];
				enabled = (data[ 0 ] != '');

				op = mop( data[ 3 ] );

				elem.display( PR( 'url' + i ), (enabled) && (data[ 0 ] != 'custom') );
				elem.display( 'row_z' + i, op.z );

				elem.display( PR( '_f_hosttop' + i ), op.t );
				elem.display( PR( '_f_user' + i ), op.u );
				elem.display( PR( '_f_pass' + i ), op.u || op.p );
				elem.display( PR( '_f_host' + i ), op.h || op.j );
				elem.display( PR( '_f_cust' + i ), 'custmsg' + i, op.c );

				elem.display( PR( '_f_wild' + i ), op.w );
				elem.display( PR( '_f_mx' + i ), op.m );
				elem.display( PR( '_f_bmx' + i ), op.b );
				elem.display( PR( '_f_opendns' + i ), op.o );
				elem.display( PR( '_f_afraid' + i ), op.a );
				elem.display( PR( '_f_ddnsx_save' + i ), op.s );

				elem.display( PR( '_f_force' + i ), 'last-response' + i, enabled );
				elem.display( 'last-update' + i, enabled && !op.z );

				if ( enabled ) {
					$( '[for="_f_user' + i + '"]' ).html( data[ 4 ] || 'Username' );
					$( '[for="_f_pass' + i + '"]' ).html( data[ 5 ] || 'Password' );
					$( '[for="_f_host' + i + '"]' ).html( data[ 6 ] || 'Hostname' );

					e = E( 'url' + i );
					e.href = data[ 2 ];
					e.innerHTML = data[ 2 ];

					if ( op.c ) {
						e = E( '_f_cust' + i );
						e.value = e.value.trim();
						if ( e.value == '' ) {
							e.value = 'http://';
						}
						if ( e.value.search( /http(s?):\/\/./ ) != 0 ) {
							ferror.set( e, 'Expecting a URL -- http://... or https://...', quiet );
							r = 0;
						}
						else {
							if ( !v_nodelim( '_f_cust' + i, quiet, 'URL' ) ) r = 0;
							else ferror.clear( e );
						}
					}
					else if ( op.a ) {
						e = E( '_f_afraid' + i );
						e.value = e.value.trim();
						if ( e.value.match( /freedns\.afraid\.org\/dynamic\/update\.php\?([a-zA-Z0-9]+)/ ) ) {
							e.value = RegExp.$1;
						}
						if ( e.value.search( /^[A-Za-z0-9]+/ ) == -1 ) {
							ferror.set( e, 'Invalid hash or URL', quiet );
							r = 0;
						}
						else {
							if ( !v_nodelim( '_f_afraid' + i, quiet, 'Token / URL' ) ) r = 0;
							else ferror.clear( e );
						}
					}
					else {
						if ( ((op.u) && (!v_length( '_f_user' + i, quiet, 1 ) || !v_nodelim( '_f_user' + i, quiet, 'Username' ))) ||
						     (!v_length( '_f_pass' + i, quiet, 1 ) || !v_nodelim( '_f_pass' + i, quiet, 'Password' )) ||
						     ((op.m) && (!v_nodelim( '_f_mx' + i, quiet, 'MX' ))) ||
						     ((op.h) && (!op.o) && (!v_length( '_f_host' + i, quiet, 1 ) || !v_nodelim( '_f_host' + i, quiet, 'Hostname' ))) ||
						     ((op.t) && (!v_length( '_f_hosttop' + i, quiet, 1 ) || !v_nodelim( '_f_hosttop' + i, quiet, 'Hostname' ))) ) {
							r = 0;
						}
					}
				}
			}

			// shouldn't do this twice, but...
			if ( E( '_f_opendns0' ) == focused ) E( '_f_opendns1' ).checked = E( '_f_opendns0' ).checked;
			if ( E( '_f_opendns1' ) == focused ) E( '_f_opendns0' ).checked = E( '_f_opendns1' ).checked;

			if ( E( '_f_ddnsx_save0' ) == focused ) E( '_f_ddnsx_save1' ).checked = E( '_f_ddnsx_save0' ).checked;
			if ( E( '_f_ddnsx_save1' ) == focused ) E( '_f_ddnsx_save0' ).checked = E( '_f_ddnsx_save1' ).checked;

			return r;
		}

		function save() {
			var fom;
			var i, j, s;
			var data, a, b;
			var setopendns;
			var op;

			if ( !verifyFields( null, 0 ) ) return;

			fom = E( '_fom' );
			fom.ddnsx_save.value = (nvram.ddnsx_save == 1) ? 1 : 0;

			fom.ddnsx_ip.value = (E( '_f_ddnsx_ip' ).value == 'custom') ? E( '_f_custom_ip' ).value : E( '_f_ddnsx_ip' ).value;

			setopendns = -1;
			for ( i = 0; i < 2; ++i ) {
				s = [];
				data = services[ E( '_f_service' + i ).selectedIndex ] || services[ 0 ];
				s.push( data[ 0 ] );
				if ( data[ 0 ] != '' ) {
					/* REMOVE-BEGIN

					 t = hostname (top)
					 u = username/password
					 h = hostname
					 c = custom url
					 w = wildcard
					 m = MX
					 b = backup MX
					 o = use OpenDNS
					 a = freedns.afraid
					 z = can't use client-given IP address
					 s = save state checkbox

					 */

					/*

					 username:password<
					 hostname<
					 wildcard<
					 mx<
					 backup mx<
					 custom url/afraid hash<

					 REMOVE-END */
					op = mop( data[ 3 ] );

					if ( (op.u) || (op.p) ) s.push( E( '_f_user' + i ).value + ':' + E( '_f_pass' + i ).value );
					else s.push( '' );

					if ( op.t ) {
						s.push( E( '_f_hosttop' + i ).value );
					}
					else if ( (op.h) || (op.j) ) {
						s.push( E( '_f_host' + i ).value );
					}
					else {
						s.push( '' );
					}

					if ( op.w ) s.push( E( '_f_wild' + i ).checked ? 1 : 0 );
					else s.push( '' );
					if ( op.m ) s.push( E( '_f_mx' + i ).value )
					else s.push( '' );
					if ( op.b ) s.push( E( '_f_bmx' + i ).checked ? 1 : 0 );
					else s.push( '' );

					if ( op.c ) {
						s.push( E( '_f_cust' + i ).value );
					}
					else if ( op.a ) {
						s.push( E( '_f_afraid' + i ).value );
					}
					else {
						s.push( '' );
					}

					if ( op.s ) {
						fom.ddnsx_save.value = E( '_f_ddnsx_save' + i ).checked ? 1 : 0;
					}

					if ( data[ 0 ] == 'opendns' ) setopendns = E( '_f_opendns' + i ).checked;
				}
				s = s.join( '<' );
				fom[ 'ddnsx' + i ].value = s;
				fom[ 'ddnsx' + i + '_cache' ].disabled = (!E( '_f_force' + i ).checked) && (s == nvram[ 'ddnsx' + i ]);
			}

			if ( setopendns != -1 ) {
				if ( setopendns ) {
					if ( opendnsInUse != opendns.length ) {
						fom.wan_dns.value = opendns.join( ' ' );
						fom.wan_dns.disabled = 0;
						fom._service.value += ',dns-restart';
					}
				}
				else {
					// not set if partial, do not remove if partial
					if ( opendnsInUse == opendns.length ) {
						a = nvram.wan_dns.split( /\s+/ );
						b = [];
						for ( i = a.length - 1; i >= 0; --i ) {
							for ( j = opendns.length - 1; j >= 0; --j ) {
								if ( a[ i ] == opendns[ j ] ) {
									a.splice( i, 1 );
									break;
								}
							}
						}
						fom.wan_dns.value = a.join( ' ' );
						fom.wan_dns.disabled = 0;
						fom._service.value += ',dns-restart';
					}
				}
			}

			form.submit( fom );
		}

		function init()
		{
			if ('<% psup("ddns-update"); %>' != 0) {
				var e = E('footer-msg');
				e.innerHTML = 'DDNS update is running. Please refresh after a few seconds.';
				e.style.visibility = 'visible';
			}
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">

		<input type="hidden" name="_nextpage" value="/#basic-ddns.asp">
		<input type="hidden" name="_service" value="ddns-restart">
		<input type="hidden" name="_nextwait" value="10">

		<input type="hidden" name="ddnsx0" value="">
		<input type="hidden" name="ddnsx1" value="">
		<input type="hidden" name="ddnsx0_cache" value="" disabled>
		<input type="hidden" name="ddnsx1_cache" value="" disabled>
		<input type="hidden" name="wan_dns" value="" disabled>
		<input type="hidden" name="ddnsx_ip" value="">
		<input type="hidden" name="ddnsx_save" value="">

		<div class="box">
			<div class="heading">Dynamic DNS Service</div>
			<div class="content">

				<div id="ddnsconf"></div>

				<script type="text/javascript">
					s = nvram.ddnsx_ip;
					a = (s != '') && (s != 'wan') && (s != 'wan2') && (s != 'wan3') && (s != 'wan4') && (s.indexOf( '@' ) != 0) && (s != '0.0.0.0') && (s != '1.1.1.1') && (s != '10.1.1.1');

					$( '#ddnsconf' ).forms(
							[
								{
									title: 'IP address', name: 'f_ddnsx_ip', type: 'select',
									options: [
										[ 'wan', 'Use WAN IP Address ' + ddnsx_ip + ' (recommended)' ],
										[ 'wan2', 'Use WAN2 IP Address ' + ddnsx2_ip ],
										/* MULTIWAN-BEGIN */
										[ 'wan3', 'Use WAN3 IP Address ' + ddnsx3_ip ],
										[ 'wan4', 'Use WAN4 IP Address ' + ddnsx4_ip ],
										/* MULTIWAN-END */
										[ '@', 'Use External IP Address Checker (every 10 minutes)' ],
										[ '0.0.0.0', 'Offline (0.0.0.0)' ],
										[ '1.1.1.1', 'Offline (1.1.1.1)' ],
										[ '10.1.1.1', 'Offline (10.1.1.1)' ],
										[ 'custom', 'Custom IP Address...' ]
									],
									value: a ? 'custom' : nvram.ddnsx_ip
								},
								{
									title: 'Custom IP address', indent: 2, name: 'f_custom_ip', type: 'text', maxlen: 15, size: 20,
									value: a ? nvram.ddnsx_ip : '', hidden: !a
								},
								{ title: 'Auto refresh every', name: 'ddnsx_refresh', type: 'text', maxlen: 8, size: 8, suffix: '<small> days (0 = disable)</small>', value: fixInt( nvram.ddnsx_refresh, 0, 90, 28 ) }
							]
					);

					a = nvram.wan_dns.split( /\s+/ );
					for ( i = 0; i < a.length; ++i ) {
						for ( j = 0; j < opendns.length; ++j ) {
							if ( a[ i ] == opendns[ j ] ) ++opendnsInUse;
						}
					}

					if ( nvram.dns_addget == 1 ) {

						dns = nvram.wan_dns + ' ' + nvram.wan_get_dns;

					} else if ( nvram.wan_dns != '' ) {

						dns = nvram.wan_dns;

					} else {

						dns = nvram.wan_get_dns;

					}

					dns = dns.split( /\s+/ );
					for ( i = 0; i < dns.length; ++i ) {
						for ( j = 0; j < opendns.length; ++j ) {
							if ( dns[ i ] == opendns[ j ] ) {
								dns[ i ] = '<i>' + dns[ i ] + '</i>';
								break;
							}
						}
					}


					dns = dns.join( ', ' );
					var $write = '';

					// Loop
					for (i = 0; i < 2; ++i) {

						v = nvram[ 'ddnsx' + i ].split( '<' );
						if ( v.length != 7 ) v = [ '', '', '', 0, '', 0, '' ];

						u = v[ 1 ].split( ':' );
						if ( u.length != 2 ) u = [ '', '' ];
						h = (v[ 0 ] == '');

						$('#ddnsconf').append('<br><h5>Dynamic DNS ' + (i + 1) + '</h5><br>');
						$( '#ddnsconf' ).forms(
								[
									{ title: 'Service', name: 'f_service' + i, type: 'select', options: services, value: v[ 0 ] },
									{ title: 'URL', indent: 2, text: '<a href="" id="url' + i + '" target="tomato-ext-ddns"></a>', hidden: 1 },
									{ title: '&nbsp;', text: '<small>* This service determines the IP address using its own method.</small>', hidden: 1, rid: 'row_z' + i },
									{ title: 'Hostname', name: 'f_hosttop' + i, type: 'text', maxlen: 96, size: 35, value: v[ 2 ], hidden: 1 },
									{ title: 'Username', name: 'f_user' + i, type: 'text', maxlen: 64, size: 35, value: u[ 0 ], hidden: 1 },
									{ title: 'Password', name: 'f_pass' + i, type: 'password', maxlen: 64, size: 35, peekaboo: 1, value: u[ 1 ], hidden: 1 },
									{ title: 'Hostname', name: 'f_host' + i, type: 'text', maxlen: 255, size: 80, value: v[ 2 ], hidden: 1 },
									{ title: 'URL', name: 'f_cust' + i, type: 'text', maxlen: 255, size: 80, value: v[ 6 ], hidden: 1 },
									{ title: ' ', text: '(Use @IP for the current IP address)', rid: ('custmsg' + i), hidden: 1 },
									{ title: 'Wildcard', indent: 2, name: 'f_wild' + i, type: 'checkbox', value: v[ 3 ] != '0', hidden: 1 },
									{ title: 'MX', name: 'f_mx' + i, type: 'text', maxlen: 32, size: 35, value: v[ 4 ], hidden: 1 },
									{ title: 'Backup MX', indent: 2, name: 'f_bmx' + i, type: 'checkbox', value: v[ 5 ] != '0', hidden: 1 },
									{
										title: 'Use as DNS', name: 'f_opendns' + i, type: 'checkbox', value: (opendnsInUse == opendns.length),
										suffix: '<br><small>(Current DNS: ' + dns + ')</small>', hidden: 1
									},
									{ title: 'Token / URL', name: 'f_afraid' + i, type: 'text', maxlen: 255, size: 80, value: v[ 6 ], hidden: 1 },
									{ title: 'Save state when IP changes (nvram commit)', name: 'f_ddnsx_save' + i, type: 'checkbox', value: nvram.ddnsx_save == '1', hidden: 1 },
									{ title: 'Force next update', name: 'f_force' + i, type: 'checkbox', value: 0, hidden: 1 },
									null,
									{ title: 'Last IP Address', custom: msgLoc( ddnsx_last[ i ] ), rid: 'last-update' + i, hidden: 1 },
									{ title: 'Last Result', custom: msgLoc( ddnsx_msg[ i ] ), rid: 'last-response' + i, hidden: h }
								]
						);

					}
				</script>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>
	</form>
	<script type="text/javascript">verifyFields(null, 1);</script>
</content>