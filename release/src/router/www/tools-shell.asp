<!--
Tomato GUI

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>System Commands</title>
<content>
	<style>
		.sectionshell { margin-top: 10px; }
		textarea { width: 80%; height: 35px; }
		table tr td:first-child { width: 150px; }
		pre { max-height: 400px; overflow: auto; }
	</style>
	<script type="text/javascript">
		var cmdresult = '';
		var cmd = null;

		var ref = new TomatoRefresh('/update.cgi', '', 0, 'tools-shell_refresh');

		ref.refresh = function(text)
		{
			execute();
		}


		function verifyFields(focused, quiet)
		{
			return 1;
		}

		function escapeText(s)
		{
			function esc(c) {
				return '&#' + c.charCodeAt(0) + ';';
			}
			return s.replace(/[&"'<>]/g, esc).replace(/\n/g, ' <br>').replace(/ /g, '&nbsp;');
		}

		function spin(x)
		{
			E('execb').disabled = x;
			E('_f_cmd').disabled = x;
			E('wait').style.visibility = x ? 'visible' : 'hidden';
			if (!x) cmd = null;
		}

		function updateResult()
		{
			E('result').innerHTML = '<tt>' + escapeText(cmdresult) + '</tt>';
			cmdresult = '';
			spin(0);
		}

		function execute()
		{
			// Opera 8 sometimes sends 2 clicks
			if (cmd) return;
			spin(1);

			cmd = new XmlHttp();
			cmd.onCompleted = function(text, xml) {
				eval(text);
				updateResult();
			}
			cmd.onError = function(x) {
				cmdresult = 'ERROR: ' + x;
				updateResult();
			}

			var s = E('_f_cmd').value;
			cmd.post('shell.cgi', 'action=execute&command=' + escapeCGI(s.replace(/\r/g, '')));
			cookie.set('shellcmd', escape(s));
		}

		function init()
		{
			var s;
			if ((s = cookie.get('shellcmd')) != null) E('_f_cmd').value = unescape(s);

			if (((s = cookie.get('tools_shell_notes_vis')) != null) && (s == '1')) {
				toggleVisibility("notes");
			}

			$('#refresh').append(genStdRefresh(1,1,"ref.toggle()"));
		}

		function toggleVisibility(whichone) {
			if (E('sesdiv_' + whichone).style.display == '') {
				E('sesdiv_' + whichone).style.display = 'none';
				E('sesdiv_' + whichone + '_showhide').innerHTML = '<i class="icon-chevron-up"></i>';
				cookie.set('status_overview_' + whichone + '_vis', 0);
			} else {
				E('sesdiv_' + whichone).style.display='';
				E('sesdiv_' + whichone + '_showhide').innerHTML = '<i class="icon-chevron-down"></i>';
				cookie.set('status_overview_' + whichone + '_vis', 1);
			}
		}

	</script>

	<ul class="nav-tabs">
		<li><a class="ajaxload" href="tools-ping.asp"><i class="icon-ping"></i> Ping</a></li>
		<li><a class="ajaxload" href="tools-trace.asp"><i class="icon-gauge"></i> Trace</a></li>
		<li><a class="active"><i class="icon-cmd"></i> System Commands</a></li>
		<li><a class="ajaxload" href="tools-survey.asp"><i class="icon-signal"></i> Wireless Survey</a></li>
		<li><a class="ajaxload" href="tools-wol.asp"><i class="icon-forward"></i> WOL</a></li>
	</ul>

	<div class="box">
		<div class="heading">System Shell</div>
		<div class="content">
			<div id="command-form"></div><hr>

			<div style="visibility:hidden;" id="wait">Please wait... <span class="spinner"></span></div>
			<pre id="result"></pre>

			<script type="text/javascript">
				$('#command-form').forms([
					{ title: 'Command', help:'Use the command &quot;nvram export --set&quot; or &quot;nvram export --set | grep qos&quot; to cut and paste configuration',
						name: 'f_cmd', type: 'textarea', wrap: 'off', value: '', style: 'width: 100%; height: 80px;' }
					], { grid: ['col-sm-2', 'col-sm-10'] });
			</script>
		</div>
	</div>

	<div id="refresh"></div><button type="button" value="Execute" onclick="execute()" id="execb" class="btn">Execute <i class="icon-cmd"></i></button>

	<script type="text/javascript">init();</script>
</content>