<!--
Tomato RAF Transmission GUI
Copyright (C) 2007-2011 Shibby
http://openlinksys.info
For use with Tomato RAF Firmware only.
No part of this file may be used without permission.
--><title>BitTorrent Client</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,bt_enable,bt_binary,bt_binary_custom,bt_custom,bt_port,bt_dir,bt_settings,bt_settings_custom,bt_incomplete,bt_rpc_enable,bt_rpc_wan,bt_auth,bt_login,bt_password,bt_port_gui,bt_dl_enable,bt_dl,bt_ul_enable,bt_ul,bt_peer_limit_global,bt_peer_limit_per_torrent,bt_ul_slot_per_torrent,bt_ratio_enable,bt_ratio,bt_ratio_idle_enable,bt_ratio_idle,bt_dht,bt_pex,bt_lpd,bt_utp,bt_blocklist,bt_blocklist_url,bt_sleep,bt_check,bt_check_time,bt_dl_queue_enable,bt_dl_queue_size,bt_ul_queue_enable,bt_ul_queue_size,bt_message"); %>

		var btgui_link = '&nbsp;&nbsp;<a href="http://' + location.hostname +':<% nv('bt_port_gui'); %>" target="_blank"><small>[Click here to open Transmission GUI]</small></a>';
		function verifyFields(focused, quiet)
		{
			var ok = 1;
			var a = E('_f_bt_enable').checked;
			var c = E('_f_bt_rpc_enable').checked;
			var d = E('_f_bt_dl_enable').checked;
			var e = E('_f_bt_ul_enable').checked;
			var g = E('_f_bt_ratio_enable').checked;
			var h = E('_f_bt_auth').checked;
			var i = E('_f_bt_blocklist').checked;
			var k = E('_f_bt_dl_queue_enable').checked;
			var l = E('_f_bt_ul_queue_enable').checked;
			var m = E('_f_bt_check').checked;
			var n = E('_f_bt_ratio_idle_enable').checked;
			E('_bt_custom').disabled = !a;
			E('_bt_binary').disabled = !a;
			E('_bt_dir').disabled = !a;
			E('_bt_port').disabled = !a;
			E('_bt_sleep').disabled = !a;
			E('_f_bt_incomplete').disabled = !a;
			E('_f_bt_check').disabled = !a;
			E('_bt_check_time').disabled = !a || !m;
			E('_bt_settings').disabled = !a;
			E('_f_bt_rpc_enable').disabled = !a;
			E('_bt_port_gui').disabled = !a || !c;
			E('_f_bt_auth').disabled = !a || !c;
			E('_bt_login').disabled = !a || !c || !h;
			E('_bt_password').disabled = !a || !c | !h;
			E('_f_bt_rpc_wan').disabled = !a || !c || !h;
			E('_f_bt_dl_enable').disabled = !a;
			E('_bt_dl').disabled = !a || !d;
			E('_f_bt_ul_enable').disabled = !a;
			E('_bt_ul').disabled = !a || !e;
			E('_bt_peer_limit_global').disabled = !a;
			E('_bt_peer_limit_per_torrent').disabled = !a;
			E('_bt_ul_slot_per_torrent').disabled = !a;
			E('_f_bt_ratio_enable').disabled = !a;
			E('_bt_ratio').disabled = !a || !g;
			E('_f_bt_ratio_idle_enable').disabled = !a;
			E('_bt_ratio_idle').disabled = !a || !n;
			E('_f_bt_dht').disabled = !a;
			E('_f_bt_pex').disabled = !a;
			E('_f_bt_lpd').disabled = !a;
			E('_f_bt_utp').disabled = !a;
			E('_f_bt_blocklist').disabled = !a;
			E('_bt_blocklist_url').disabled = !a || !i;
			E('_f_bt_dl_queue_enable').disabled = !a;
			E('_bt_dl_queue_size').disabled = !a || !k;
			E('_f_bt_ul_queue_enable').disabled = !a;
			E('_bt_ul_queue_size').disabled = !a || !l;
			E('_bt_message').disabled = !a;
			var o = (E('_bt_settings').value == 'custom');
			elem.display('_bt_settings_custom', o && a);
			var p = (E('_bt_binary').value == 'custom');
			elem.display('_bt_binary_custom', p && a);
			if (!v_length('_bt_custom', quiet, 0, 2048)) ok = 0;
			var s = E('_bt_custom');
			if (s.value.search(/"rpc-enable":/) == 0)  {
				ferror.set(s, 'Cannot set "rpc-enable" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"peer-port":/) == 0)  {
				ferror.set(s, 'Cannot set "peer-port" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"speed-limit-down-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "speed-limit-down-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"speed-limit-up-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "speed-limit-up-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"speed-limit-down":/) == 0)  {
				ferror.set(s, 'Cannot set "speed-limit-down" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"speed-limit-up":/) == 0)  {
				ferror.set(s, 'Cannot set "speed-limit-up" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"rpc-port":/) == 0)  {
				ferror.set(s, 'Cannot set "rpc-port" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"rpc-whitelist-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "rpc-whitelist-enabled" option here. Whitelist is always disabled', quiet);
				ok = 0; }
			if (s.value.search(/"rpc-username":/) == 0)  {
				ferror.set(s, 'Cannot set "rpc-username" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"rpc-password":/) == 0)  {
				ferror.set(s, 'Cannot set "rpc-password" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"download-dir":/) == 0)  {
				ferror.set(s, 'Cannot set "download-dir" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"incomplete-dir-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "incomplete-dir-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"incomplete-dir":/) == 0)  {
				ferror.set(s, 'Cannot set "incomplete-dir" option here. If incomplete dir is enabled, all incomplete files will be downloaded to "/download_dir/.incomplete" directory.', quiet);
				ok = 0; }
			if (s.value.search(/"peer-limit-global":/) == 0)  {
				ferror.set(s, 'Cannot set "peer-limit-global" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"peer-limit-per-torrent":/) == 0)  {
				ferror.set(s, 'Cannot set "peer-limit-per-torrent" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"upload-slots-per-torrent":/) == 0)  {
				ferror.set(s, 'Cannot set "upload-slots-per-torrent" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"dht-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "dht-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"pex-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "pex-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"lpd-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "lpd-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"utp-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "utp-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"ratio-limit-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "ratio-limit-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"ratio-limit":/) == 0)  {
				ferror.set(s, 'Cannot set "ratio-limit" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"rpc-authentication-required":/) == 0)  {
				ferror.set(s, 'Cannot set "rpc-authentication-required" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"blocklist-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "blocklist-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"blocklist-url":/) == 0)  {
				ferror.set(s, 'Cannot set "blocklist-url" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"download-queue-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "download-queue-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"download-queue-size":/) == 0)  {
				ferror.set(s, 'Cannot set "download-queue-size" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"seed-queue-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "seed-queue-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"seed-queue-size":/) == 0)  {
				ferror.set(s, 'Cannot set "seed-queue-size" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"idle-seeding-limit-enabled":/) == 0)  {
				ferror.set(s, 'Cannot set "idle-seeding-limit-enabled" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"idle-seeding-limit":/) == 0)  {
				ferror.set(s, 'Cannot set "idle-seeding-limit" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			if (s.value.search(/"message-level":/) == 0)  {
				ferror.set(s, 'Cannot set "message-level" option here. You can set it in Tomato GUI', quiet);
				ok = 0; }
			return ok;
		}
		function save()
		{
			if (verifyFields(null, 0)==0) return;
			var fom = E('_fom');
			fom.bt_enable.value = E('_f_bt_enable').checked ? 1 : 0;
			fom.bt_incomplete.value = E('_f_bt_incomplete').checked ? 1 : 0;
			fom.bt_check.value = E('_f_bt_check').checked ? 1 : 0;
			fom.bt_rpc_enable.value = E('_f_bt_rpc_enable').checked ? 1 : 0;
			fom.bt_auth.value = E('_f_bt_auth').checked ? 1 : 0;
			fom.bt_rpc_wan.value = E('_f_bt_rpc_wan').checked ? 1 : 0;
			fom.bt_dl_enable.value = E('_f_bt_dl_enable').checked ? 1 : 0;
			fom.bt_ul_enable.value = E('_f_bt_ul_enable').checked ? 1 : 0;
			fom.bt_ratio_enable.value = E('_f_bt_ratio_enable').checked ? 1 : 0;
			fom.bt_ratio_idle_enable.value = E('_f_bt_ratio_idle_enable').checked ? 1 : 0;
			fom.bt_dht.value = E('_f_bt_dht').checked ? 1 : 0;
			fom.bt_pex.value = E('_f_bt_pex').checked ? 1 : 0;
			fom.bt_lpd.value = E('_f_bt_lpd').checked ? 1 : 0;
			fom.bt_utp.value = E('_f_bt_utp').checked ? 1 : 0;
			fom.bt_blocklist.value = E('_f_bt_blocklist').checked ? 1 : 0;
			fom.bt_dl_queue_enable.value = E('_f_bt_dl_queue_enable').checked ? 1 : 0;
			fom.bt_ul_queue_enable.value = E('_f_bt_ul_queue_enable').checked ? 1 : 0;
			if (fom.bt_enable.value == 0) {
				fom._service.value = 'bittorrent-stop';
			}
			else {
				fom._service.value = 'bittorrent-restart';
			}
			form.submit('_fom', 1);
		}
		function init()
		{
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#nas-bittorrent.asp">
		<input type="hidden" name="_service" value="bittorrent-restart">
		<input type="hidden" name="bt_enable">
		<input type="hidden" name="bt_incomplete">
		<input type="hidden" name="bt_check">
		<input type="hidden" name="bt_rpc_enable">
		<input type="hidden" name="bt_auth">
		<input type="hidden" name="bt_rpc_wan">
		<input type="hidden" name="bt_dl_enable">
		<input type="hidden" name="bt_ul_enable">
		<input type="hidden" name="bt_blocklist">
		<input type="hidden" name="bt_ratio_enable">
		<input type="hidden" name="bt_ratio_idle_enable">
		<input type="hidden" name="bt_dht">
		<input type="hidden" name="bt_pex">
		<input type="hidden" name="bt_lpd">
		<input type="hidden" name="bt_utp">
		<input type="hidden" name="bt_dl_queue_enable">
		<input type="hidden" name="bt_ul_queue_enable">

		<div class="box" data-box="bit-basic-set">
			<div class="heading">BitTorrent Client - Basic Settings</div>
			<div class="content" id="torrent-config-section">

				<div class="clienttable"></div><hr>
				<script type="text/javascript">
					$('.clienttable').forms([
						{ title: 'Enable torrent client', name: 'f_bt_enable', type: 'checkbox', value: nvram.bt_enable == '1', suffix: ' <small>*</small>' },
						{ title: 'Transmission binary path', multi: [
							{ name: 'bt_binary', type: 'select', options: [
								/* BBT-BEGIN */
								['internal','Internal (/usr/bin)'],
								/* BBT-END */
								['optware','Optware (/opt/bin)'],
								['custom','Custom'] ], value: nvram.bt_binary, suffix: ' <small>*</small> ' },
							{ name: 'bt_binary_custom', type: 'text', maxlen: 40, size: 40, value: nvram.bt_binary_custom }
						] },
						{ title: 'Keep alive', name: 'f_bt_check', type: 'checkbox', value: nvram.bt_check == '1', suffix: ' <small>*</small>' },
						{ title: 'Check alive every', indent: 2, name: 'bt_check_time', type: 'text', maxlen: 5, size: 7, value: nvram.bt_check_time, suffix: ' <small>minutes (range: 1 - 55; default: 15)</small>' },
						{ title: 'Delay at startup', name: 'bt_sleep', type: 'text', maxlen: 5, size: 7, value: nvram.bt_sleep, suffix: ' <small>seconds (range: 1 - 60; default: 10)</small>' },
						{ title: 'Listening port', name: 'bt_port', type: 'text', maxlen: 5, size: 7, value: nvram.bt_port, suffix: ' <small>*</small>' },
						{ title: 'Download directory', name: 'bt_dir', type: 'text', maxlen: 40, size: 40, value: nvram.bt_dir },
						{ title: 'Use .incomplete/', indent: 2, name: 'f_bt_incomplete', type: 'checkbox', value: nvram.bt_incomplete == '1' }
					]);
				</script>
				<h4>Notes</h4>
				<ul>
					<li><b>Enable torrent client</b> - Caution! - If your router only has 32MB of RAM, you'll have to use swap.
					<li><b>Transmission binary path</b> Path to the directory containing transmission-daemon etc.
					<li><b>Keep alive</b> - If enabled, transmission-daemon will be checked at the specified interval and will re-launch after a crash.
					<li><b>Listening port</b> - Port used for torrent client. Make sure this port is not in use.
				</ul>
			</div>
		</div>

		<div class="box" data-box="bit-remote">
			<div class="heading">Remote Access <span class="bt-link"></span></div>
			<div class="content">
				<div class="remoteacc"></div><hr>
				<script type="text/javascript">
					$('.remoteacc').forms([
						{ title: 'Enable GUI', name: 'f_bt_rpc_enable', type: 'checkbox', value: nvram.bt_rpc_enable == '1' },
						{ title: 'Listening GUI port', indent: 2, name: 'bt_port_gui', type: 'text', maxlen: 32, size: 5, value: nvram.bt_port_gui, suffix: ' <small>*</small>' },
						{ title: 'Authentication required', name: 'f_bt_auth', type: 'checkbox', value: nvram.bt_auth == '1', suffix: ' <small>*</small>' },
						{ title: 'Username', indent: 2, name: 'bt_login', type: 'text', maxlen: 32, size: 15, value: nvram.bt_login },
						{ title: 'Password', indent: 2, name: 'bt_password', type: 'password', maxlen: 32, size: 15, value: nvram.bt_password },
						{ title: 'Allow remote access', name: 'f_bt_rpc_wan', type: 'checkbox', value: nvram.bt_rpc_wan == '1', suffix: ' <small>*</small>' }
					]);
				</script>

				<h4>Notes</h4>
				<ul>
					<li><b>Listening GUI port</b> - Port used for Transmission GUI. Make sure this port is not in use.
					<li><b>Authentication required</b> - Authentication is <b><i>highly recomended</i></b>. GUI will prompt for user/pass.
					<li><b>Allow remote access</b> - This option will open the Transmission GUI port from the WAN side and allow the GUI to be accessed from the internet.
				</ul>
			</div>
		</div>

		<div class="box" data-box="bit-limits">
			<div class="heading">Limits</div>
			<div class="content">
				<div class="limitsgui"></div>
				<script type="text/javascript">
					$('.limitsgui').forms([
						{ title: 'Download limit', multi: [
							{ name: 'f_bt_dl_enable', type: 'checkbox', value: nvram.bt_dl_enable == '1', suffix: '  ' },
							{ name: 'bt_dl', type: 'text', maxlen: 10, size: 7, value: nvram.bt_dl, suffix: ' <small>kB/s</small>' } ] },
						{ title: 'Upload limit', multi: [
							{ name: 'f_bt_ul_enable', type: 'checkbox', value: nvram.bt_ul_enable == '1', suffix: '  ' },
							{ name: 'bt_ul', type: 'text', maxlen: 10, size: 7, value: nvram.bt_ul, suffix: ' <small>kB/s</small>' } ] },
						{ title: 'Stop seeding at ratio', multi: [
							{ name: 'f_bt_ratio_enable', type: 'checkbox', value: nvram.bt_ratio_enable == '1', suffix: '  ' },
							{ name: 'bt_ratio', type: 'select', options: [['0.0000','0.0'],['0.1000','0.1'],['0.2000','0.2'],['0.5000','0.5'],['1.0000','1.0'],['1.5000','1.5'],['2.0000','2.0'],['2.5000','2.5'],['3.0000','3.0']], value: nvram.bt_ratio } ] },
						{ title: 'Stop seeding if idle for', multi: [
							{ name: 'f_bt_ratio_idle_enable', type: 'checkbox', value: nvram.bt_ratio_idle_enable == '1', suffix: '  ' },
							{ name: 'bt_ratio_idle', type: 'text', maxlen: 10, size: 7, value: nvram.bt_ratio_idle, suffix: ' <small>minutes (range: 1 - 55; default: 30)</small>' } ] },
						{ title: 'Global peer limit', name: 'bt_peer_limit_global', type: 'text', maxlen: 10, size: 7, value: nvram.bt_peer_limit_global, suffix: ' <small>(range: 10 - 1000; default: 150)</small>' },
						{ title: 'Peer limit per torrent', name: 'bt_peer_limit_per_torrent', type: 'text', maxlen: 10, size: 7, value: nvram.bt_peer_limit_per_torrent, suffix: ' <small>(range: 1 - 200; default: 30)</small>' },
						{ title: 'Upload slots per torrent', name: 'bt_ul_slot_per_torrent', type: 'text', maxlen: 10, size: 7, value: nvram.bt_ul_slot_per_torrent, suffix: ' <small>(range: 1 - 50; default: 10)</small>' }
					]);
				</script>
			</div>
		</div>

		<div class="box" data-box="bit-queue">
			<div class="heading">Queue torrents</div>
			<div class="content">
				<div class="queuetor"></div><hr>
				<script type="text/javascript">
					$('.queuetor').forms([
						{ title: 'Downloads queuing', multi: [
							{ name: 'f_bt_dl_queue_enable', type: 'checkbox', value: nvram.bt_dl_queue_enable == '1', suffix: '  ' },
							{ name: 'bt_dl_queue_size', type: 'text', maxlen: 5, size: 7, value: nvram.bt_dl_queue_size, suffix: ' <small>(range: 1 - 30; default: 5) *</small>' }
						] },
						{ title: 'Seeds queuing', multi: [
							{ name: 'f_bt_ul_queue_enable', type: 'checkbox', value: nvram.bt_ul_queue_enable == '1', suffix: '  ' },
							{ name: 'bt_ul_queue_size', type: 'text', maxlen: 5, size: 7, value: nvram.bt_ul_queue_size, suffix: ' <small>(range: 1 - 30; default: 5) *</small>' }
						] }
					]);
				</script>
				<h4>Notes</h4>
				<ul>
					<li><b>Downloads queuing</b> - If enabled, this option will limit how many torrents can be downloaded at once.
					<li><b>Seeds queuing</b> - If enabled, this option will limit how many torrents can be uploaded/seeded at once.
				</ul>
			</div>
		</div>

		<div class="box" data-box="bit-advanced">
			<div class="heading">Advanced Settings</div>
			<div class="content" id="tor-advancedsettings"></div>
			<script type="text/javascript">
				$('#tor-advancedsettings').forms([
					{ title: 'Find more peers using', multi: [
						{ suffix: '&nbsp; DHT &nbsp;&nbsp;&nbsp;', name: 'f_bt_dht', type: 'checkbox', value: nvram.bt_dht == '1' },
						{ suffix: '&nbsp; PEX &nbsp;&nbsp;&nbsp;', name: 'f_bt_pex', type: 'checkbox', value: nvram.bt_pex == '1' },
						{ suffix: '&nbsp; LPD &nbsp;&nbsp;&nbsp;', name: 'f_bt_lpd', type: 'checkbox', value: nvram.bt_lpd == '1' },
						{ suffix: '&nbsp; uTP &nbsp;&nbsp;&nbsp;', name: 'f_bt_utp', type: 'checkbox', value: nvram.bt_utp == '1' }
					] },
					{ title: 'Message level', name: 'bt_message', type: 'select', options: [ ['0','None'], ['1','Error'], ['2','Info'], ['3','Debug'] ], value: nvram.bt_message, suffix: ' ' },
					{ title: 'Save settings location', multi: [
						{ name: 'bt_settings', type: 'select', options: [
							['down_dir','In the Download directory (Recommended)'],
							['/jffs','JFFS2'],
							['/cifs1','CIFS 1'],['/cifs2','CIFS 2'],
							['/tmp','RAM (Temporary)'], ['custom','Custom'] ], value: nvram.bt_settings, suffix: ' ' },
						{ name: 'bt_settings_custom', type: 'text', maxlen: 60, size: 40, value: nvram.bt_settings_custom }
					] },
					{ title: 'Blocklist', multi: [
						{ name: 'f_bt_blocklist', type: 'checkbox', value: nvram.bt_blocklist == '1', suffix: '  ' },
						{ name: 'bt_blocklist_url', type: 'text', maxlen: 80, size: 80, value: nvram.bt_blocklist_url }
					] },
					null,
					{ title: '<a href="https://trac.transmissionbt.com/wiki/EditConfigFiles" target="_new">Transmission</a><br>Custom configuration', name: 'bt_custom', type: 'textarea', value: nvram.bt_custom, style: 'width: 100%; height: 80px' }
				]);
			</script>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert success" style="visibility: hidden;"></span>
	</form>

	<script type="text/javascript">init(); verifyFields(null, 1);</script>
</content>