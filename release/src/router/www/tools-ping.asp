<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Ping</title>
<content>
	<style type="text/css">
		#tp-grid .co1 {
			text-align: right;
			width: 30px;
		}
		#tp-grid .co2 {
			width: 440px;
		}
		#tp-grid .co3, #tp-grid .co4, #tp-grid .co5, #tp-grid .co6 {
			text-align: right;
			width: 70px;
		}
		#tp-grid .header .co1 {
			text-align: left;
		}
		#stats {
			margin: 10px 0;
		}
	</style>
	<script type="text/javascript">
		//	<% nvram('at_update,tomatoanon_answer'); %>	// http_id
		var pingdata = '';

		var pg = new TomatoGrid();
		pg.setup = function() {
			this.init('tp-grid');
			this.headerSet(['Seq', 'Address', 'RX Bytes', 'TTL', 'RTT (ms)', '+/- (ms)']);
		}
		pg.populate = function()
		{
			var buf = pingdata.split('\n');
			var i;
			var r, s, t;
			var last = -1;
			var resolv = [];
			var stats = '';

			/* REMOVE-BEGIN
			1.9
			PING 192.168.1.3 (192.168.1.3): 56 data bytes
			64 bytes from 192.168.1.3: seq=0 ttl=64 time=1.165 ms
			64 bytes from 192.168.1.3: seq=1 ttl=64 time=0.675 ms
			64 bytes from 192.168.1.3: seq=2 ttl=64 time=0.683 ms
			64 bytes from 192.168.1.3: seq=3 ttl=64 time=0.663 ms
			64 bytes from 192.168.1.3: seq=4 ttl=64 time=0.682 ms

			--- 192.168.1.3 ping statistics ---
			5 packets transmitted, 5 packets received, 0% packet loss
			round-trip min/avg/max = 0.663/0.773/1.165 ms


			1.2
			PING 192.168.5.5 (192.168.1.5): 56 data bytes
			64 bytes from 192.168.1.5: icmp_seq=0 ttl=64 time=1.2 ms
			64 bytes from 192.168.1.5: icmp_seq=1 ttl=64 time=0.7 ms
			64 bytes from 192.168.1.5: icmp_seq=2 ttl=64 time=0.7 ms
			64 bytes from 192.168.1.5: icmp_seq=3 ttl=64 time=0.7 ms
			64 bytes from 192.168.1.5: icmp_seq=4 ttl=64 time=0.8 ms

			--- 192.168.5.5 ping statistics ---
			5 packets transmitted, 5 packets received, 0% packet loss
			round-trip min/avg/max = 0.7/0.8/1.2 ms
			REMOVE-END */

			this.removeAllData();
			for (i = 0; i < buf.length; ++i) {
				if (r = buf[i].match(/^(\d+) bytes from (.+): .*seq=(\d+) ttl=(\d+) time=(\d+\.\d+) ms/)) {
					r.splice(0, 1);
					t = r[0];
					r[0] = r[2];
					r[2] = t;
					if (resolv[r[1]]) r[1] = resolv[r[1]] + ' (' + r[1] + ')';
					r[4] *= 1;
					r[5] = (last > 0) ? (r[4] - last).toFixed(2) : '';
					r[4] = r[4].toFixed(2);
					this.insertData(-1, r)
					last = r[4];
				}
				else if (buf[i].match(/^PING (.+) \((.+)\)/)) {
					resolv[RegExp.$2] = RegExp.$1;
				}
				else if (buf[i].match(/^(\d+) packets.+, (\d+) packets.+, (\d+%)/)) {
					stats = 'Packets: ' + RegExp.$1 + ' transmitted, ' + RegExp.$2 + ' received, ' + RegExp.$3 + ' lost<br>';
				}
				else if (buf[i].match(/^round.+ (\d+\.\d+)\/(\d+\.\d+)\/(\d+\.\d+)/)) {
					stats = 'Round-Trip: ' + RegExp.$1 + ' min, ' + RegExp.$2 + ' avg, ' + RegExp.$3 + ' max (ms)<br>' + stats;
				}
			}

			E('stats').innerHTML = stats;
			E('debug').value = pingdata;
			pingdata = '';
			spin(0);
		}

		function verifyFields(focused, quiet)
		{
			var s;
			var e;

			e = E('_f_addr');
			s = e.value.trim();
			if (!s.match(/^[\w\-\.\:]+$/)) {
				ferror.set(e, 'Invalid hostname/address', quiet);
				return 0;
			}
			ferror.clear(e);

			return v_range('_f_count', quiet, 1, 50) && v_range('_f_size', quiet, 1, 10240);
		}


		var pinger = null;

		function spin(x)
		{
			E('pingb').disabled = x;
			E('_f_addr').disabled = x;
			E('_f_count').disabled = x;
			E('_f_size').disabled = x;
			E('wait').style.visibility = x ? 'visible' : 'hidden';
			if (!x) pinger = null;
		}

		function ping()
		{
			// Opera 8 sometimes sends 2 clicks
			if (pinger) return;

			if (!verifyFields(null, 0)) return;

			spin(1);

			pinger = new XmlHttp();
			pinger.onCompleted = function(text, xml) {
				eval(text);
				pg.populate();
			}
			pinger.onError = function(x) {
				alert('error: ' + x);
				spin(0);
			}

			var addr = E('_f_addr').value;
			var count = E('_f_count').value;
			var size = E('_f_size').value;
			pinger.post('ping.cgi', 'addr=' + addr + '&count=' + count + '&size=' + size);

			cookie.set('pingaddr', addr);
			cookie.set('pingcount', count);
			cookie.set('pingsize', size);
		}

		function init() {
			var s;

			if ((s = cookie.get('pingaddr')) != null) E('_f_addr').value = s;
			if ((s = cookie.get('pingcount')) != null) E('_f_count').value = s;
			if ((s = cookie.get('pingsize')) != null) E('_f_size').value = s;

			pg.setup()
			E('_f_addr').onkeypress = function(ev) { if (checkEvent(ev).keyCode == 13) ping(); }
		}
	</script>

	<ul class="nav-tabs">
		<li><a class="active">Ping</a></li>
		<li><a class="ajaxload" href="tools-trace.asp">Trace</a></li>
		<li><a class="ajaxload" href="tools-shell.asp">System Commands</a></li>
		<li><a class="ajaxload" href="tools-survey.asp">Wireless Survey</a></li>
		<li><a class="ajaxload" href="tools-wol.asp">WOL</a></li>
	</ul>

	<div class="box">
		<div class="heading">Ping</div>
		<div class="content">

			<div id="ping-forms"></div><hr>
			<script type='text/javascript'>
				$('#ping-forms').forms([
					{ title: 'Address', name: 'f_addr', type: 'text', maxlen: 64, size: 32, value: '',
						suffix: ' <button type="submit" value="Ping" onclick="ping()" id="pingb" class="btn">Ping <i class="icon-ping"></i></button>' },
					{ title: 'Ping Count', name: 'f_count', type: 'text', maxlen: 2, size: 7, value: '5' },
					{ title: 'Packet Size', name: 'f_size', type: 'text', maxlen: 5, size: 7, value: '56', suffix: ' <small>(bytes)</small>' }
				]);
			</script>

			<div style="visibility:hidden; text-align: right;" id="wait">Please wait... <div class="spinner"></div></div>

			<table id="tp-grid" class="line-table"></table>
			<pre id="stats"></pre>
		</div>
	</div>

	<div style="height:10px;" onclick="javascript:E('debug').style.display=''"></div>
	<textarea id="debug" style="width:99%;height:300px;display:none"></textarea>
	<script type="text/javascript">init();</script>
</content>