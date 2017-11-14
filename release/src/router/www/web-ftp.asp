<!--
Tomato GUI
FTP Server - !!TB

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>FTP Server</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,ftp_enable,ftp_super,ftp_anonymous,ftp_dirlist,ftp_port,ftp_max,ftp_ipmax,ftp_staytimeout,ftp_rate,ftp_anonrate,ftp_anonroot,ftp_pubroot,ftp_pvtroot,ftp_custom,ftp_users,ftp_sip,ftp_limit,log_ftp"); %>

		ftplimit = nvram.ftp_limit.split(',');
		if (ftplimit.length != 3) ftplimit = [0,3,60];

		var aftg = new TomatoGrid();

		aftg.exist = function(f, v)
		{
			var data = this.getAllData();
			for (var i = 0; i < data.length; ++i) {
				if (data[i][f] == v) return true;
			}
			return false;
		}

		aftg.existName = function(name)
		{
			return this.exist(0, name);
		}

		aftg.sortCompare = function(a, b) {
			var da = a.getRowData();
			var db = b.getRowData();
			var r = cmpText(da[this.sortColumn], db[this.sortColumn]);
			return this.sortAscending ? r : -r;
		}

		aftg.verifyFields = function(row, quiet)
		{
			var f, s;
			f = fields.getAll(row);

			ferror.clear(f[0]);
			ferror.clear(f[1]);
			ferror.clear(f[3]);

			if (!v_length(f[0], quiet, 1)) return 0;

			s = f[0].value.trim().replace(/\s+/g, ' ');
			if (s.length > 0) {
				if (s.search(/^[a-zA-Z0-9_\-]+$/) == -1) {
					ferror.set(f[0], 'Invalid user name. Only characters "A-Z 0-9 - _" are allowed.', quiet);
					return 0;
				}
				if (this.existName(s)) {
					ferror.set(f[0], 'Duplicate user name.', quiet);
					return 0;
				}
				if (s == 'root' || s == 'admin') {
					ferror.set(f[0], 'User names "root" and "admin" are not allowed.', quiet);
					return 0;
				}
				f[0].value = s;
			}

			if (!v_length(f[1], quiet, 1)) return 0;
			if (!v_nodelim(f[1], quiet, 'Password', 1)) return 0;
			if (f[2].value == 'Private') {
				f[3].value = '';
				f[3].disabled = true;
			}
			else {
				f[3].disabled = false;
				if (!v_nodelim(f[3], quiet, 'Root Directory', 1) || !v_path(f[3], quiet, 0)) return 0;
			}

			return 1;
		}

		aftg.resetNewEditor = function() {
			var f;

			f = fields.getAll(this.newEditor);
			ferror.clearAll(f);

			f[0].value = '';
			f[1].value = '';
			f[2].selectedIndex = 0;
			f[3].value = '';
		}

		aftg.setup = function()
		{
			this.init('aft-grid', 'sort', 50, [
				{ type: 'text', maxlen: 50 },
				{ type: 'password', maxlen: 50, peekaboo: 1 },
				{ type: 'select', options: [['Read/Write', 'Read/Write'],['Read Only', 'Read Only'],['View Only', 'View Only'],['Private', 'Private']] },
				{ type: 'text', maxlen: 128 }
			]);
			this.headerSet(['User Name', 'Password', 'Access', 'Root Directory*']);

			var s = nvram.ftp_users.split('>');
			for (var i = 0; i < s.length; ++i) {
				var t = s[i].split('<');
				if (t.length == 3) {
					t.push('');
				}
				if (t.length == 4) {
					this.insertData(-1, t);
				}
			}

			this.showNewEditor();
			this.resetNewEditor();
		}

		function verifyFields(focused, quiet)
		{
			var a, b;
			var ok = 1;

			a = E('_ftp_enable').value;
			b = E('_ftp_port');
			elem.display(PR(b), (a != 0));
			b = E('_f_ftp_sip');
			elem.display(PR(b), (a == 1));

			E('_f_limit').disabled = (a != 1);
			b = E('_f_limit').checked;
			elem.display(PR('_f_limit_hit'), PR('_f_limit_sec'), (a == 1 && b));
			E('_ftp_anonymous').disabled = (a == 0);
			E('_f_ftp_super').disabled = (a == 0);
			E('_f_log_ftp').disabled = (a == 0);
			E('_ftp_pubroot').disabled = (a == 0);
			E('_ftp_pvtroot').disabled = (a == 0);
			E('_ftp_anonroot').disabled = (a == 0);
			E('_ftp_dirlist').disabled = (a == 0);
			E('_ftp_max').disabled = (a == 0);
			E('_ftp_ipmax').disabled = (a == 0);
			E('_ftp_rate').disabled = (a == 0);
			E('_ftp_anonrate').disabled = (a == 0);
			E('_ftp_staytimeout').disabled = (a == 0);
			E('_ftp_custom').disabled = (a == 0);

			if (a != 0) {
				if (!v_port('_ftp_port', quiet || !ok)) ok = 0;
				if (!v_range('_ftp_max', quiet || !ok, 0, 12)) ok = 0;
				if (!v_range('_ftp_ipmax', quiet || !ok, 0, 12)) ok = 0;
				if (!v_range('_ftp_rate', quiet || !ok, 0, 99999)) ok = 0;
				if (!v_range('_ftp_anonrate', quiet || !ok, 0, 99999)) ok = 0;
				if (!v_range('_ftp_staytimeout', quiet || !ok, 0, 65535)) ok = 0;
				if (!v_length('_ftp_custom', quiet || !ok, 0, 2048)) ok = 0;
				if (!v_path('_ftp_pubroot', quiet || !ok, 0)) ok = 0;
				if (!v_path('_ftp_pvtroot', quiet || !ok, 0)) ok = 0;
				if (!v_path('_ftp_anonroot', quiet || !ok, 0)) ok = 0;
				if (a == 1 && b) {
					if (!v_range('_f_limit_hit', quiet || !ok, 1, 100)) ok = 0;
					if (!v_range('_f_limit_sec', quiet || !ok, 3, 3600)) ok = 0;
				}
			}

			if (a == 1) {
				b = E('_f_ftp_sip');
				if ((b.value.length) && (!_v_iptaddr(b, quiet || !ok, 15, 1, 1))) ok = 0;
				else ferror.clear(b);
			}

			return ok;
		}

		function save()
		{
			if (aftg.isEditing()) return;
			if (!verifyFields(null, 0)) return;

			var fom = E('_fom');

			var data = aftg.getAllData();
			var r = [];
			for (var i = 0; i < data.length; ++i) r.push(data[i].join('<'));
			fom.ftp_users.value = r.join('>');

			fom.ftp_sip.value = fom.f_ftp_sip.value.split(/\s*,\s*/).join(',');
			fom.ftp_super.value = E('_f_ftp_super').checked ? 1 : 0;
			fom.log_ftp.value = E('_f_log_ftp').checked ? 1 : 0;

			fom.ftp_limit.value = (E('_f_limit').checked ? 1 : 0) +
			',' + E('_f_limit_hit').value + ',' + E('_f_limit_sec').value;

			form.submit(fom, 1);
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#web-ftp.asp">
		<input type="hidden" name="_service" value="ftpd-restart">

		<input type="hidden" name="ftp_super">
		<input type="hidden" name="log_ftp">
		<input type="hidden" name="ftp_users">
		<input type="hidden" name="ftp_sip">
		<input type="hidden" name="ftp_limit">

		<div class="box" data-box="ftp-conf-main">
			<div class="heading">FTP Server Configuration</div>
			<div class="content">
				<div id="ftpconf"></div>
				<small class="text-danger">Note: Avoid using this option when FTP server is enabled for WAN. IT PROVIDES FULL ACCESS TO THE ROUTER FILE SYSTEM!</small>
				<script type="text/javascript">
					$('#ftpconf').forms([
						{ title: 'Enable FTP Server', name: 'ftp_enable', type: 'select',
							options: [['0', 'No'],['1', 'Yes, WAN and LAN'],['2', 'Yes, LAN only']],
							value: nvram.ftp_enable },
						{ title: 'FTP Port', indent: 2, name: 'ftp_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.ftp_port, 21) },
						{ title: 'Allowed Remote<br>Address(es)', indent: 2, name: 'f_ftp_sip', type: 'text', maxlen: 512, size: 64, value: nvram.ftp_sip,
							suffix: '<br><small>(optional; ex: "1.1.1.1", "1.1.1.0/24", "1.1.1.1 - 2.2.2.2" or "me.example.com")</small>' },
						{ title: 'Anonymous Users Access', name: 'ftp_anonymous', type: 'select',
							options: [['0', 'Disabled'],['1', 'Read/Write'],['2', 'Read Only'],['3', 'Write Only']],
							value: nvram.ftp_anonymous },
						{ title: 'Allow Admin Login*', name: 'f_ftp_super', type: 'checkbox',
							suffix: ' <small>Allows users to connect with admin account.</small>',
							value: nvram.ftp_super == 1 },
						{ title: 'Log FTP requests and responses', name: 'f_log_ftp', type: 'checkbox',
							value: nvram.log_ftp == 1 }
					]);
				</script>
			</div>
		</div>

		<div class="box" data-box="ftp-dirs">
			<div class="heading">Directories</div>
			<div class="content">

				<div id="ftpdirs"></div><hr>
				<small>
					*&nbsp;&nbsp;When no directory is specified, /mnt is used as a root directory.
					<br>**&nbsp;In private mode, the root directory is the directory under the "Private Root Directory" with the name matching the name of the user.
				</small>

				<script type="text/javascript">
					$('#ftpdirs').forms([
						{ title: 'Anonymous Root Directory*', name: 'ftp_anonroot', type: 'text', maxlen: 256, size: 32,
							suffix: ' <small>(for anonymous connections)</small>',
							value: nvram.ftp_anonroot },
						{ title: 'Public Root Directory*', name: 'ftp_pubroot', type: 'text', maxlen: 256, size: 32,
							suffix: ' <small>(for authenticated users access, if not specified for the user)</small>',
							value: nvram.ftp_pubroot },
						{ title: 'Private Root Directory**', name: 'ftp_pvtroot', type: 'text', maxlen: 256, size: 32,
							suffix: ' <small>(for authenticated users access in private mode)</small>',
							value: nvram.ftp_pvtroot },
						{ title: 'Directory Listings', name: 'ftp_dirlist', type: 'select',
							options: [['0', 'Enabled'],['1', 'Disabled'],['2', 'Disabled for Anonymous']],
							suffix: ' <small>(always enabled for Admin)</small>',
							value: nvram.ftp_dirlist }
					]);
				</script>
			</div>
		</div>

		<div class="box" data-box="ftp-limits">
			<div class="heading">Limits</div>
			<div class="content" id="ftp-limits"></div>
			<script type="text/javascript">
				$('#ftp-limits').forms([
					{ title: 'Maximum Users Allowed to Log in', name: 'ftp_max', type: 'text', maxlen: 5, size: 7,
						suffix: ' <small>(0 - unlimited)</small>',
						value: nvram.ftp_max },
					{ title: 'Maximum Connections from the same IP', name: 'ftp_ipmax', type: 'text', maxlen: 5, size: 7,
						suffix: ' <small>(0 - unlimited)</small>',
						value: nvram.ftp_ipmax },
					{ title: 'Maximum Bandwidth for Anonymous Users', name: 'ftp_anonrate', type: 'text', maxlen: 5, size: 7,
						suffix: ' <small>KBytes/sec (0 - unlimited)</small>',
						value: nvram.ftp_anonrate },
					{ title: 'Maximum Bandwidth for Authenticated Users', name: 'ftp_rate', type: 'text', maxlen: 5, size: 7,
						suffix: ' <small>KBytes/sec (0 - unlimited)</small>',
						value: nvram.ftp_rate },
					{ title: 'Idle Timeout', name: 'ftp_staytimeout', type: 'text', maxlen: 5, size: 7,
						suffix: ' <small>seconds (0 - no timeout)</small>',
						value: nvram.ftp_staytimeout },
					{ title: 'Limit Connection Attempts', name: 'f_limit', type: 'checkbox',
						value: ftplimit[0] != 0 },
					{ title: '', indent: 2, multi: [
						{ name: 'f_limit_hit', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; <small>every</small> &nbsp;', value: ftplimit[1] },
						{ name: 'f_limit_sec', type: 'text', maxlen: 4, size: 6, suffix: '&nbsp; <small>seconds</small>', value: ftplimit[2] }
					] }
				]);
			</script>
		</div>

		<div class="box" data-box="ftp-cust">
			<div class="heading">Custom Configuration</div>
			<div class="content" id="ftpcustom"></div>
			<script type="text/javascript">
				$('#ftpcustom').forms([
					{ title: 'Vsftpd Custom Configuration (<a href="http://vsftpd.beasts.org/vsftpd_conf.html" target="_new"><i class="icon-info"></i></a>)', name: 'ftp_custom', type: 'textarea', value: nvram.ftp_custom,
						style: 'width: 100%; height: 80px;' }
				]);
			</script>
		</div>

		<div class="box" data-box="ftp-usr-acc">
			<div class="heading">User Accounts</div>
			<div class="content">
				<table class="line-table" id="aft-grid"></table><br /><hr>
				<small>
					*&nbsp;&nbsp;When no Root Directory is specified for the user, the default "Public Root Directory" is used.
				</small>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">aftg.setup(); verifyFields(null, 1);</script>
</content>