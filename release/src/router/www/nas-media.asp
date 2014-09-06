<!--
Tomato GUI
Media Server Settings - !!TB

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Media Server</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,ms_enable,ms_port,ms_dirs,ms_dbdir,ms_tivo,ms_stdlna,ms_sas,cifs1,cifs2,jffs2_on"); %>

		changed = 0;
		mdup = parseInt('<% psup("minidlna"); %>');

		var mediatypes = [['', 'All Media Files'], ['A', 'Audio only'], ['V', 'Video only'], ['P', 'Images only']];
		var msg = new TomatoGrid();

		msg.dataToView = function(data) {
			var b = [];
			var i;

			b.push(escapeHTML('' + data[0]));
			for (i = 0; i < mediatypes.length; ++i)
				if (mediatypes[i][0] == ('' + data[1])) {
					b.push(mediatypes[i][1]);
					break;
			}
			if (b.length < 2) b.push(mediatypes[0][1]);
			return b;
		}

		msg.verifyFields = function(row, quiet)
		{
			var ok = 1;
			var f;
			f = fields.getAll(row);

			if (!v_nodelim(f[0], quiet, 'Directory', 1) || !v_path(f[0], quiet, 1))
				ok = 0;

			changed |= ok;
			return ok;
		}

		msg.resetNewEditor = function()
		{
			var f;

			f = fields.getAll(this.newEditor);
			ferror.clearAll(f);
			f[0].value = '';
			f[1].selectedIndex = 0;
		}

		msg.setup = function()
		{
			this.init('ms-grid', 'sort', 50, [
				{ type: 'text', maxlen: 256 },
				{ type: 'select', options: mediatypes }
			]);
			this.headerSet(['Directory', 'Content Filter']);

			var s = ('' + nvram.ms_dirs).split('>');
			for (var i = 0; i < s.length; ++i) {
				var t = s[i].split('<');
				if (t.length == 2) this.insertData(-1, t);
			}

			this.sort(0);
			this.showNewEditor();
			this.resetNewEditor();
		}

		function getDbPath()
		{
			var s = E('_f_loc').value;
			return (s == '*user') ? E('_f_user').value : s;
		}

		function verifyFields(focused, quiet)
		{
			var ok = 1;
			var a, b, v;
			var eLoc, eUser;

			elem.display('_restart_button', nvram.ms_enable == '1');

			a = E('_f_ms_enable').checked ? 1 : 0;

			eLoc = E('_f_loc');
			eUser = E('_f_user');

			eLoc.disabled = (a == 0);
			eUser.disabled = (a == 0);
			E('_ms_port').disabled = (a == 0);
			E('_f_ms_sas').disabled = (a == 0);
			E('_f_ms_rescan').disabled = (a == 0);
			E('_f_ms_tivo').disabled = (a == 0);
			E('_f_ms_stdlna').disabled = (a == 0);
			E('_restart_button').disabled = (a == 0);

			ferror.clear(eLoc);
			ferror.clear(eUser);

			v = eLoc.value;
			b = (v == '*user');
			elem.display(eUser, b);
			elem.display(PR('_f_ms_sas'), (v != ''));

			if (a == 0) {
				if (focused != E('_f_ms_rescan'))
					changed |= ok;
				return ok;
			}
			if (b) {
				if (!v_path(eUser, quiet || !ok, 1)) ok = 0;
			}
			/* JFFS2-BEGIN */
			else if (v == '/jffs/dlna') {
				if (nvram.jffs2_on != '1') {
					ferror.set(eLoc, 'JFFS is not enabled.', quiet || !ok);
					ok = 0;
				}
				else ferror.clear(eLoc);
			}
			/* JFFS2-END */
			/* REMOVE-BEGIN */
			/* CIFS-BEGIN */
			else if (v.match(/^\/cifs(1|2)\/dlna$/)) {
				if (nvram['cifs' + RegExp.$1].substr(0, 1) != '1') {
					ferror.set(eLoc, 'CIFS #' + RegExp.$1 + ' is not enabled.', quiet || !ok);
					ok = 0;
				}
				else ferror.clear(eLoc);
			}
			/* CIFS-END */
			/* REMOVE-END */

			if (focused != E('_f_ms_rescan'))
				changed |= ok;
			return ok;
		}

		function save()
		{
			if (msg.isEditing()) return;
			if (!verifyFields(null, 0)) return;

			var fom = E('_fom');

			fom.ms_enable.value = E('_f_ms_enable').checked ? 1 : 0;
			fom.ms_tivo.value = E('_f_ms_tivo').checked ? 1 : 0;
			fom.ms_stdlna.value = E('_f_ms_stdlna').checked ? 1 : 0;
			fom.ms_rescan.value = E('_f_ms_rescan').checked ? 1 : 0;
			fom.ms_sas.value = E('_f_ms_sas').checked ? 1 : 0;

			fom.ms_dbdir.value = getDbPath();

			var data = msg.getAllData();
			var r = [];
			for (var i = 0; i < data.length; ++i) r.push(data[i].join('<'));
			fom.ms_dirs.value = r.join('>');

			form.submit(fom, 1);
		}

		function restart(isup)
		{
			if (changed) {
				if (!confirm("Unsaved changes will be lost. Continue anyway?")) return;
			}
			$('.minidlna-control').html('<div class="spinner spinner-small"></div>');
			E('_restart_button').disabled = true;
			form.submitHidden('tomato.cgi', {
				ms_rescan: E('_f_ms_rescan').checked ? 1 : 0,
				_reboot: 0, _commit: 0, _nvset: 1,
				_redirect: '/#nas-media.asp',
				_sleep: '3',
				_service: 'media-' + (isup ? 're' : '') + 'start'
			});
		}

		function submit_complete()
		{
			reloadPage();
		}

		var xob = null;

		function setNoticeText(s)
		{
			if (s.length)
				s = '<div class="alert info" id="notice1">' + s.replace(/\n/g, '<br>') + '</div>';
			elem.setInnerHTML('notice-msg', s);
		}

		function updateNotice()
		{
			if (xob) return;

			xob = new XmlHttp();
			xob.onCompleted = function(text, xml) {
				setNoticeText(text);
				xob = null;
				setTimeout(updateNotice, 5000);
			}
			xob.onError = function(ex) { xob = null; }
			xob.post('/update.cgi', 'exec=notice&arg0=dlna');
		}

		function init()
		{
			changed = 0;
			updateNotice();
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#nas-media.asp">
		<input type="hidden" name="_service" value="media-restart">

		<input type="hidden" name="ms_enable">
		<input type="hidden" name="ms_dirs">
		<input type="hidden" name="ms_dbdir">
		<input type="hidden" name="ms_tivo">
		<input type="hidden" name="ms_stdlna">
		<input type="hidden" name="ms_rescan">
		<input type="hidden" name="ms_sas">

		<span id="notice-msg"></span>

		<div class="box" id="dlna" data-box="dlna-media-serv">
			<div class="heading">Media / DLNA Server </div>
			<div class="content mediadlna"></div>
			<script type="text/javascript">

				switch (nvram.ms_dbdir) {
					case '':
					case '/jffs/dlna':
					case '/cifs1/dlna':
					case '/cifs2/dlna':
						loc = nvram.ms_dbdir;
						break;
					default:
						loc = '*user';
						break;
				}

				$('.content.mediadlna').forms([
					{ title: 'Enable', name: 'f_ms_enable', type: 'checkbox', value: nvram.ms_enable == '1' },
					{ title: 'Port', name: 'ms_port', type: 'text', maxlen: 5, size: 6, value: nvram.ms_port, suffix: '<small>(range: 0 - 65535; default (random) set 0)</small>' },
					{ title: 'Database Location', multi: [
						{ name: 'f_loc', type: 'select', options: [['','RAM (Temporary)'],
							/* JFFS2-BEGIN */
							['/jffs/dlna','JFFS'],
							/* JFFS2-END */
							/* REMOVE-BEGIN */
							/* CIFS-BEGIN */
							['/cifs1/dlna','CIFS 1'],['/cifs2/dlna','CIFS 2'],
							/* CIFS-END */
							/* REMOVE-END */
							['*user','Custom Path']], value: loc },
						{ name: 'f_user', type: 'text', maxlen: 256, size: 60, value: nvram.ms_dbdir }
					] },
					{ title: 'Scan Media at Startup*', indent: 2, name: 'f_ms_sas', type: 'checkbox', value: nvram.ms_sas == '1', hidden: 1 },
					{ title: 'Rescan on the next run*', indent: 2, name: 'f_ms_rescan', type: 'checkbox', value: 0,
						suffix: '&nbsp; <small>(Media scan may take considerable time to complete.)</small>' },
					{ title: 'TiVo Support', name: 'f_ms_tivo', type: 'checkbox', value: nvram.ms_tivo == '1' },
					{ title: 'Strictly adhere to DLNA standards', name: 'f_ms_stdlna', type: 'checkbox', value: nvram.ms_stdlna == '1' }
				]);

				$('#dlna .heading').append('<a href="#" class="minidlna-control pull-right" data-toggle="tooltip" title="' + (mdup ? 'Res' : 'S') + 'tart Now" onclick="restart(mdup)"' +
					'id="_restart_button">' + (mdup ? '<i class="icon-reboot"></i>' : '<i class="icon-play"></i>') + '</a>');
				$('#dlna .heading').append('<small ' + (mdup ? 'style="color: green;">(Running' : 'style="color: red;">(Stopped') + ')</small>');
			</script>
		</div>

		<div class="box" data-box="dlna-dirs">
			<div class="heading">Media Directories</div>
			<div class="content">
				<table class="line-table" id="ms-grid"></table>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert info" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">msg.setup(); init(); verifyFields(null, 1);</script>
</content>