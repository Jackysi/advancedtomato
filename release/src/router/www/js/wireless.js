function selectedBand(uidx) {
	if (bands[uidx].length > 1) {
		var u = wl_fface(uidx);
		var e = E('_f_wl'+u+'_nband');
		return (e.value + '' == '' ? eval('nvram["wl'+u+'_nband"]') : e.value);
	} else if (bands[uidx].length > 0) {
		return bands[uidx][0][0] || '0';
	} else {
		return '0';
	}
}

function refreshNetModes(uidx) {
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
	val = (!nm_loaded[uidx] || (e.value + '' == '')) ? eval('nvram["wl'+u+'_net_mode"]') : e.value;
	if (val == 'disabled') val = 'mixed';
	for (i = 0; i < m.length; ++i)
		buf += '<option value="' + m[i][0] + '"' + ((m[i][0] == val) ? ' selected' : '') + '>' + m[i][1] + '</option>';

	e = E('__wl'+u+'_net_mode');
	buf = '<select name="wl'+u+'_net_mode" onchange="verifyFields(this, 1)" id = "_wl'+u+'_net_mode">' + buf + '</select>';
	elem.setInnerHTML(e, buf);

	nm_loaded[uidx] = 1;
}

function refreshChannels(uidx) {
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
			val = (!ch_loaded[uidx] || (e.value + '' == '')) ? eval('nvram["wl'+u+'_channel"]') : e.value;
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
	if (e != null)
	{
		sb = (e.value + '' == '' ? eval('nvram["wl'+u+'_nctrlsb"]') : e.value);
		/* REMOVE-BEGIN */
// AB
//		sb = ((e.value + '' == '') ? (nvram['wl'+u+'_nctrlsb']) : e.value);
/* REMOVE-END */
e = E('_wl'+u+'_nbw_cap');
bw = (e.value + '' == '' ? eval('nvram["wl'+u+'_nbw_cap"]') : e.value) == '0' ? '20' : '40';
/* REMOVE-BEGIN */
// AB
//		bw = ((e.value + '' == '') ? (nvram['wl'+u+'_nbw_cap']) : e.value) == '0' ? '20' : '40';
/* REMOVE-END */
refresher[uidx].onError = function(ex) { alert(ex); refresher[uidx] = null; reloadPage(); }
refresher[uidx].post('update.cgi', 'exec=wlchannels&arg0=' + u + '&arg1=' + (nphy ? '1' : '0') +
	'&arg2=' + bw + '&arg3=' + selectedBand(uidx) + '&arg4=' + sb);
}
}

function scan() {
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

function scanButton(u) {
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

function random_x(max) {
	var c = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
	var s = '';
	while (max-- > 0) s += c.substr(Math.floor(c.length * Math.random()), 1);
	return s;
}

function random_psk(id) {
	var e = E(id);
	e.value = random_x(63);
	verifyFields(null, 1);
}

function random_wep(u) {
	E('_wl'+u+'_passphrase').value = random_x(16);
	generate_wep(u);
}

function v_wep(e, quiet) {
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
function generate_wep(u) {
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
