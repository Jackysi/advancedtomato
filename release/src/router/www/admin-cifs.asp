<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>CIFS Client</title>
<content>
	<script type="text/javascript">

		//	<% nvram("at_update,tomatoanon_answer,cifs1,cifs2"); %>

		function verifyFields(focused, quiet)
		{
			var i, p, b;
			var unc, user, pass, dom, exec, servern, sec;

			for (i = 1; i <= 2; ++i) {
				p = '_f_cifs' + i;
				unc = E(p + '_unc');
				user = E(p + '_user');
				pass = E(p + '_pass');
				dom = E(p + '_dom');
				exec = E(p + '_exec');
				servern = E(p + '_servern');
				sec = E(p + '_sec');

				b = !E(p + '_enable').checked;
				unc.disabled = b;
				user.disabled = b;
				pass.disabled = b;
				exec.disabled = b;
				dom.disabled = b;
				servern.disabled = b;
				sec.disabled = b;
				if (!b) {
					if ((!v_nodelim(unc, quiet, 'UNC')) || (!v_nodelim(user, quiet, 'username')) || (!v_nodelim(pass, quiet, 'password')) ||
						(!v_nodelim(servern, quiet, 'Netbios name')) ||
						(!v_nodelim(dom, quiet, 'domain')) || (!v_nodelim(exec, quiet, 'exec path'))) return 0;
					if ((!v_length(user, quiet, 1)) || (!v_length(pass, quiet, 1))) return 0;
					unc.value = unc.value.replace(/\//g, '\\');
					if (!unc.value.match(/^\\\\.+\\/)) {
						ferror.set(unc, 'Invalid UNC', quiet);
						return 0;
					}
				}
				else {
					ferror.clear(unc, user, pass, dom, exec, servern, sec);
				}
			}

			return 1;
		}

		function save()
		{
			var i, p;

			if (!verifyFields(null, 0)) return;

			for (i = 1; i <= 2; ++i) {
				p = '_f_cifs' + i;
				E('cifs' + i).value = (E(p + '_enable').checked ? '1' : '0') + '<' + E(p + '_unc').value + '<' +
				E(p + '_user').value + '<' + E(p + '_pass').value + '<' + E(p + '_dom').value + '<' + E(p + '_exec').value
				+ '<' + E(p + '_servern').value + '<' + E(p + '_sec').value;
			}
			form.submit('_fom', 0);
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#admin-cifs.asp">
		<input type="hidden" name="_nextwait" value="10">
		<input type="hidden" name="_service" value="cifs-restart">

		<input type="hidden" name="cifs1" id="cifs1">
		<input type="hidden" name="cifs2" id="cifs2">

		<div class="fluid-gr" id="mounties">
			<script type="text/javascript">
				show_notice1('<% notice("cifs"); %>');

				a = b = [0, '\\\\192.168.1.5\\shared_example', '', '', '', '', '', ''];

				if (r = nvram.cifs1.match(/^(0|1)<(\\\\.+)<(.*)<(.*)<(.*)<(.*)<(.*)<(.*)$/)) a = r.splice(1, 8);
				if (r = nvram.cifs2.match(/^(0|1)<(\\\\.+)<(.*)<(.*)<(.*)<(.*)<(.*)<(.*)$/)) b = r.splice(1, 8);

				//	<% statfs("/cifs1", "cifs1"); %>
				//	<% statfs("/cifs2", "cifs2"); %>

				var cifs = '<div class="box" data-box="cifs-set-1"><div class="heading"><i class="icon-drive"></i> CIFS1 <small>(/cifs1)</small></div><div class="content">' +
				createFormFields([
					{ title: 'Enable', indent: 2, name: 'f_cifs1_enable', type: 'checkbox', value: a[0]*1 },
					{ title: 'UNC', indent: 2, name: 'f_cifs1_unc', type: 'text', maxlen: 128, size: 48, value: a[1] },
					{ title: 'Netbios Name', indent: 2, name: 'f_cifs1_servern', type: 'text', maxlen: 128, size: 48, value: a[6] },
					{ title: 'Username', indent: 2, name: 'f_cifs1_user', type: 'text', maxlen: 32, size: 34, value: a[2] },
					{ title: 'Password', indent: 2, name: 'f_cifs1_pass', type: 'password', maxlen: 32, size: 34, peekaboo: 1, value: a[3] },
					{ title: 'Domain', indent: 2, name: 'f_cifs1_dom', type: 'text', maxlen: 32, size: 34, value: a[4] },
					{ title: 'Execute When Mounted', indent: 2, name: 'f_cifs1_exec', type: 'text', maxlen: 64, size: 34, value: a[5] },
					{ title: 'Security', indent: 2, name: 'f_cifs1_sec', type: 'select',
						options: [['','Default (NTLM)'],['ntlmi','NTLM and packet signing'],['ntlmv2','NTLMv2'],['ntlmv2i','NTLMv2 and packet signing'],['nontlm','No NTLM'],['lanman','LANMAN'],['none','None']],
						value: a[7] },
					{ title: 'CIFS1 Usage', indent: 2, text: cifs1.size ? (scaleSize(cifs1.size - cifs1.free) + ' / ' + scaleSize(cifs1.size)) +
						' (<span class="txtcifs1"></span>) <div class="progress cifs1"><div class="bar"></div></div>' : '(not mounted)' },
				]) + '</div></div>';

				cifs += '<div class="box" data-box="cifs-set-2"><div class="heading"><i class="icon-drive"></i> CIFS2 <small>(/cifs2)</small></div><div class="content">' +
				createFormFields([
					{ title: 'Enable', indent: 2, name: 'f_cifs2_enable', type: 'checkbox', value: b[0]*1 },
					{ title: 'UNC', indent: 2, name: 'f_cifs2_unc', type: 'text', maxlen: 128, size: 48, value: b[1] },
					{ title: 'Netbios Name', indent: 2, name: 'f_cifs2_servern', type: 'text', maxlen: 128, size: 48, value: b[6] },
					{ title: 'Username', indent: 2, name: 'f_cifs2_user', type: 'text', maxlen: 32, size: 34, value: b[2] },
					{ title: 'Password', indent: 2, name: 'f_cifs2_pass', type: 'password', maxlen: 32, size: 34, peekaboo: 1, value: b[3] },
					{ title: 'Domain', indent: 2, name: 'f_cifs2_dom', type: 'text', maxlen: 32, size: 34, value: b[4] },
					{ title: 'Execute When Mounted', indent: 2, name: 'f_cifs2_exec', type: 'text', maxlen: 64, size: 34, value: b[5] },
					{ title: 'Security', indent: 2, name: 'f_cifs2_sec', type: 'select',
						options: [['','Default (NTLM)'],['ntlmi','NTLM and packet signing'],['ntlmv2','NTLMv2'],['ntlmv2i','NTLMv2 and packet signing'],['nontlm','No NTLM'],['lanman','LANMAN'],['none','None']],
						value: b[7] },
					{ title: 'CIFS2 Usage', indent: 2, text: cifs2.size ? (scaleSize(cifs2.size - cifs2.free) + ' / ' + scaleSize(cifs2.size)) +
						' (<span class="txtcifs2"></span>) <div class="progress cifs2"><div class="bar"></div></div>' : '(not mounted)' }
				]) + '</div></div>';

				$('#mounties').append(cifs);
			</script>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>

	</form>

	<script type='text/javascript'>verifyFields(null, 1);
		if (cifs1.size) {

			var calcCifs1 = (Math.round(((cifs1.size - cifs1.free) / cifs1.size) * 100)) + '%';
			$('.txtcifs1').html(calcCifs1);
			$('.progress.cifs1 .bar').css('width', calcCifs1);

		}

		if (cifs2.size) {

			var calcCifs2 = (Math.round(((cifs2.size - cifs2.free) / cifs2.size) * 100)) + '%';
			$('.txtcifs2').html(calcCifs2);
			$('.progress.cifs2 .bar').css('width', calcCifs2);

		}
	</script>
</content>