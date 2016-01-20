<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>Firmware Upgrade</title>
<content>
	<style>
		#afu-progress {
			display: block;
			position: fixed;
			top: 0;
			right: 0;
			left: 0;
			bottom: 0;
			z-index: 20;
			background: #fff;
			color: #5A5A5A;
			opacity: 0;
			transition: all 400ms ease-in;
		}

		#afu-progress .text-container {
			position: absolute;
			display: block;
			text-align: center;
			font-size: 14px;
			width: 100%;
			height: 150px;
			top: -150px;
			margin-top: -75px;
			transform: scale(0.2);
			transition: all 600ms ease-out;
		}

		#afu-progress.active {
			opacity: 1;
		}

		#afu-progress.active .text-container {
			transform: scale(1);
			top: 35%;
		}
		
		.line-table tr { background: transparent !important; }
		.line-table tr:last-child { border: 0; }
	</style>
	<script type="text/javascript">
		// <% nvram("jffs2_on"); %>

		function clock()
		{
			var t = ((new Date()).getTime() - startTime) / 1000;
			elem.setInnerHTML('afu-time', Math.floor(t / 60) + ':' + Number(Math.floor(t % 60)).pad(2));
		}
		function upgrade() {
			var name;
			var i;
			var fom = document.form_upgrade;
			var ext;
			name = fixFile(fom.file.value);

			if (name.search(/\.(bin|trx|chk)$/i) == -1) {
				alert('Expecting a ".bin", ".trx" or ".chk" file.');
				return false;
			}

			if (!confirm('Are you sure you want to upgrade using ' + name + '?')) return;
			E('afu-upgrade-button').disabled = true;

			// Some cool things
			$('#wrapper > .content').css('position', 'static');
			$('#afu-progress').clone().prependTo('#wrapper').show().addClass('active');
			startTime = (new Date()).getTime();
			setInterval('clock()', 800);
			fom.action += '?_reset=' + (E('f_reset').checked ? "1" : "0");
			form.addIdAction(fom);
			fom.submit();
		}
	</script>
	<div id="afu-input">

		<div class="alert alert-warning icon">
			<h5>Attention!</h5>There has been many reports how AdvancedTomato did not flash well or it came with many bugs. Reason for that is bad image files which can sometimes get corrupted at the download process.
			This message is here to warn you to check MD5 checksum (<a target="_blank" href="http://en.wikipedia.org/wiki/Checksum"><i class="icon-info"></i></a>) before flashing any images to your router.
			By using this process and learning if image is corrupted or not, you will eliminate many issues with the upgrade process.
			<a class="close"><i class="icon-cancel"></i></a>
		</div>

		<form name="form_upgrade" method="post" action="upgrade.cgi" encType="multipart/form-data">

			<div class="box">
				<div class="heading">Router Upgrade</div>
				<div class="content">

					<fieldset>
						<label class="control-left-label col-sm-3">Select new image:</label>
						<div class="col-sm-9"><input class="uploadfile" type="file" name="file" size="50">
							<button type="button" value="Upgrade" id="afu-upgrade-button" onclick="upgrade();" class="btn btn-danger">Upgrade <i class="icon-upload"></i></button>
						</div>
					</fieldset>

					<fieldset>
						<label class="control-left-label col-sm-3" for="f_reset">Restore defaults</label>
						<div class="col-sm-9">
							<div id="reset-input">
								<div class="checkbox c-checkbox"><label><input class="custom" type="checkbox" id="f_reset">
									<span class="icon-check"></span> &nbsp; After flashing, erase all data in NVRAM memory</label>
								</div>
							</div>
						</div>
					</fieldset>

				</div>
			</div>

			<div class="box">
				<div class="heading">Router Info</div>
				<div class="content">
					<table class="line-table" id="version-table">
						<tr><td>Current Version:</td><td>&nbsp; <% version(1); %> <small></td></tr>
					</table>
				</div>
			</div>

			<div id="afu-progress" style="display:none;">
				<div class="text-container">
					<div class="spinner spinner-large"></div><br /><br />
					<b id="afu-time">0:00</b><br />
					Please wait while new firmware is being uploaded and flashed...<br />
					<b>WARNING:</b> Do not interrupt browser or the router!
				</div>
			</div>
		</form>
	</div>

	/* JFFS2-BEGIN */
	<div class="alert alert-error" style="display:none;" id="jwarn">
		<h5>Upgrade forbidden!</h5>
		An upgrade may overwrite the JFFS partition currently in use. Before upgrading,
		please backup the contents of the JFFS partition, disable it, then reboot the router.
		<a href="/#admin-jffs2.asp">Disable &raquo;</a>
	</div>
	<script type="text/javascript">
		//	<% sysinfo(); %>
		$('#version-table').append('<tr><td>Free Memory:</td><td>&nbsp; ' + scaleSize(sysinfo.totalfreeram) + ' &nbsp; <small>(aprox. size that can be buffered completely in RAM)</small></td></tr>');

		if (nvram.jffs2_on != '0') {
			E('jwarn').style.display = '';
			E('afu-input').style.display = 'none';
		}
	</script>
	/* JFFS2-END */
</content>