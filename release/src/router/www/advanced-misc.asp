<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Miscellaneous</title>
<content>
	<script type="text/javascript">
		//	<% nvram("at_update,tomatoanon_answer,t_features,wait_time,wan_speed,clkfreq,jumbo_frame_enable,jumbo_frame_size,ctf_disable"); %>

		et1000 = features('1000et');

		function verifyFields(focused, quiet)
		{
			E('_jumbo_frame_size').disabled = !E('_f_jumbo_frame_enable').checked;
			return 1;
		}

		function save()
		{
			var fom = E('_fom');
			fom.jumbo_frame_enable.value = E('_f_jumbo_frame_enable').checked ? 1 : 0;
			/* CTF-BEGIN */
			fom.ctf_disable.value = E('_f_ctf_disable').checked ? 0 : 1;
			/* CTF-END */

			if ((fom.wan_speed.value != nvram.wan_speed) ||
				/* CTF-BEGIN */
				(fom.ctf_disable.value != nvram.ctf_disable) ||
				/* CTF-END */
				(fom.jumbo_frame_enable.value != nvram.jumbo_frame_enable) ||
				(fom.jumbo_frame_size.value != nvram.jumbo_frame_size)) {
				fom._reboot.value = '1';
				form.submit(fom, 0);
			}
			else {
				form.submit(fom, 1);
			}
		}
	</script>

	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#advanced-misc.asp">
		<input type="hidden" name="_reboot" value="0">

		<input type="hidden" name="jumbo_frame_enable">
		<!-- CTF-BEGIN -->
		<input type="hidden" name="ctf_disable">
		<!-- CTF-END -->

		<div class="box">
			<div class="heading">Miscellaneous Settings</div>
			<div class="content">

				<div id="form-fields"></div><hr>
				<script type="text/javascript">
					a = [];
					for (i = 3; i <= 20; ++i) a.push([i, i + ' seconds']);

					$('#form-fields').forms([
						{ title: 'Boot Wait Time *', name: 'wait_time', type: 'select', options: a, value: fixInt(nvram.wait_time, 3, 20, 3) },
						{ title: 'WAN Port Speed *', name: 'wan_speed', type: 'select', options: [[0,'10Mb Full'],[1,'10Mb Half'],[2,'100Mb Full'],[3,'100Mb Half'],[4,'Auto']], value: nvram.wan_speed },
						null,

						/* CTF-BEGIN */
						{ title: 'CTF (Cut-Through Forwarding)', name: 'f_ctf_disable', type: 'checkbox', value: nvram.ctf_disable != '1' },
						null,
						/* CTF-END */

						{ title: 'Enable Jumbo Frames *', name: 'f_jumbo_frame_enable', type: 'checkbox', value: nvram.jumbo_frame_enable != '0', hidden: !et1000 },
						{ title: 'Jumbo Frame Size *', name: 'jumbo_frame_size', type: 'text', maxlen: 4, size: 6, value: fixInt(nvram.jumbo_frame_size, 1, 9720, 2000),
							suffix: ' <small>Bytes (range: 1 - 9720; default: 2000)</small>', hidden: !et1000 }

					]);
				</script>

				<h4>Notes</h4>
				<ul>
					<li>Not all models support these options</li>
				</ul>
			</div>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>
	</form>

	<script type="text/javascript">verifyFields(null, 1);</script>
</content>