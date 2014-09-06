<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

Tomato VLAN GUI
Copyright (C) 2011 Augusto Bott
http://code.google.com/p/tomato-sdhc-vlan/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Web Server Menu</title>
<content>
	<script type='text/javascript'>

		//	<% nvram("nginx_enable,nginx_php,nginx_keepconf,nginx_port,nginx_fqdn,nginx_docroot,nginx_priority,nginx_custom"); %>

		changed = 0;
		nginxup = parseInt ('<% psup("nginx"); %>');

		function toggle(service, isup)
		{
			if (changed) {
				if (!confirm("Unsaved changes will be lost. Continue anyway?")) return;
			}

			$('.nginx-control').html('<div class="spinner spinner-small"></div>');

			E('_' + service + '_button').disabled = true;
			form.submitHidden('/service.cgi', {
				_redirect: '/#nas-nginx.asp',
				_sleep: ((service == 'nginxfp') && (!isup)) ? '10' : '5',
				_service: service + (isup ? '-stop' : '-start')
			});
		}

		function verifyFields(focused, quiet)
		{
			var ok = 1;
			var a, b, c;
			var i;
			var vis = {
				_f_nginx_php : 1,
				_f_nginx_port : 1,
				_f_nginx_keepconf : 1,
				_f_nginx_fqdn : 1,
				_f_nginx_docroot : 1,
				_f_nginx_priority : 1,
				_f_nginx_custom : 1,
			}
			if (!E('_f_nginx_enable').checked) {
				for (i in vis) {
					vis[i] = 2;
				}
				vis._f_nginx_enable = 1;
			}

			if (!v_port('_f_nginx_port', quiet))
			{
				ok = 0;
				W('port');
			}

			if ((!v_length('_f_nginx_docroot', quiet, 1, 255)) || (!v_length('_f_nginx_fqdn', quiet, 1, 255)) || (!v_length('_f_nginx_custom', quiet, 1, 4096)))
			{
				ok = 0;
				W('others');
			}

			changed |= ok;
			return ok;
		}

		function save()
		{
			var fom = E('_fom');
			if (!verifyFields(null, false)) return;

			fom.nginx_enable.value = E('_f_nginx_enable').checked ? 1 : 0;
			if (fom.nginx_enable.value) {
				fom.nginx_php.value = fom.f_nginx_php.checked ? 1 : 0;
				fom.nginx_keepconf.value = fom.f_nginx_keepconf.checked ? 1 : 0;
				fom.nginx_port.value = fom.f_nginx_port.value;
				fom.nginx_fqdn.value = fom.f_nginx_fqdn.value;
				fom.nginx_docroot.value = fom.f_nginx_docroot.value;
				fom.nginx_priority.value = fom.f_nginx_priority.value;
				fom.nginx_custom.value = fom.f_nginx_custom.value;
				fom._service.value = 'nginx-restart';
			} else {
				fom._service.value = 'nginx-stop';
			}
			form.submit(fom, 1);
		}

		function init()
		{
			verifyFields(null, 1);
			$('.nginx-status').html((!nginxup ? '<small style="color: red;">(Stopped)</small>' : '<small style="color: green;">(Running)</small>'));
			$('.nginx-status').after('<a href="#" data-toggle="tooltip" class="pull-right nginx-control" title="' +
			(nginxup ? 'Stop NGINX Server' : 'Start NGINX Server') + '" onclick="toggle(\'nginxfp\', nginxup); return false;" id="_nginxfp_button">' + (nginxup ? '<i class="icon-stop"></i>' : '<i class="icon-play"></i>') + '</a>');
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#nas-nginx.asp">
		<input type="hidden" name="_service" value="enginex-restart">
		<input type="hidden" name="_nextwait" value="10">
		<input type="hidden" name="_reboot" value="0">

		<input type="hidden" name="nginx_enable">
		<input type="hidden" name="nginx_php">
		<input type="hidden" name="nginx_keepconf">
		<input type="hidden" name="nginx_port">
		<input type="hidden" name="nginx_fqdn">
		<input type="hidden" name="nginx_docroot">
		<input type="hidden" name="nginx_priority">
		<input type="hidden" name="nginx_custom">

		<div class="box">
			<div class="heading">NGINX Web Server <span class="nginx-status"></span></div>
			<div class="content">

				<div id="config-section-div"></div><hr>
				<h4>User Manual</h4>
				<ul>
					<li><b> Status Button:</b> Quick Start-Stop Service. Enable Web Server must be checked to modify settings.<br>
					<li><b> Enable Server on Start:</b> To activate the Web Server tick and save this screen.<br>
					<li><b> Keep Config Files:</b> Have you modified the configuration file manually? Tick this box and changes will be maintained.<br>
					<li><b> Web Server Port:</b> The Port used by the Web Server to be accessed. Check conflict when the port is used by other services.<br>
					<li><b> Web Server Name:</b> Name that will appear on top of your Internet Browser.<br>
					<li><b> Document Root Path:</b> The path in your router where documents are stored.<br>
					<li><b> Examples:<br></b>
					/tmp/mnt/HDD/www/ as you can find in USB mount path.<br>
					<li><b> NGINX Custom Configuration:</b> You can add other values to nginx.conf to suit your needs.</li>
					<li>
						<b> Server Priority:</b> Sets the service priority over other processes running on the router.<br>
						The operating system kernel has priority -5.<br>
						Never select a lower value than the kernel uses. Do not use the service test page to adjust the<br>
						server performance, it's performance is lower than the definitive media where files will be <br>
						located, i.e; USB Stick, Hard Drive or SSD.<br>
					</li>
				</ul>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert info" style="visibility: hidden;"></span>
	</form>

	<script type="text/javascript">
		$('#config-section-div').forms([
			{ title: 'Enable Server on Start', name: 'f_nginx_enable', type: 'checkbox', value: nvram.nginx_enable == '1'},
			{ title: 'Enable PHP support', name: 'f_nginx_php', type: 'checkbox', value: (nvram.nginx_php != '0') },
			{ title: 'Keep Config Files', name: 'f_nginx_keepconf', type: 'checkbox', value: (nvram.nginx_keepconf != '0') },
			{ title: 'Web Server Port', name: 'f_nginx_port', type: 'text', maxlen: 5, size: 7, value: fixPort(nvram.nginx_port, 85), suffix: '<small> default: 85</small>' },
			{ title: 'Web Server Name', name: 'f_nginx_fqdn', type: 'text', maxlen: 255, size: 20, value: nvram.nginx_fqdn },
			{ title: 'Server Root Path', name: 'f_nginx_docroot', type: 'text', maxlen: 255, size: 40, value: nvram.nginx_docroot, suffix: '<span>&nbsp;/index.html / index.htm / index.php</span>' },
			{ title: 'Server Priority', name: 'f_nginx_priority', type: 'text', maxlen: 8, size:3, value: nvram.nginx_priority, suffix:'<small> Max. Perfor: -20, Min.Perfor: 19, default: 10</small>' },
			{ title: '<a href="http://wiki.nginx.org/Configuration" target="_new">NGINX</a><br>Custom configuration', name: 'f_nginx_custom', type: 'textarea', value: nvram.nginx_custom, style: 'width: 100%; height: 100px;' }
		]);
	</script>
	<script type='text/javascript'>init(); verifyFields(null, 1);</script>
</content>