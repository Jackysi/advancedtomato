<!--
Tomato GUI
For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>UPS Monitor</title>
<content>
	<script type="text/javascript">
		//      <% nvram("at_update,tomatoanon_answer"); %>

		function init() {
			clientSideInclude('ups-status', '/ext/cgi-bin/tomatoups.cgi');
			clientSideInclude('ups-data', '/ext/cgi-bin/tomatodata.cgi');
		}

		function clientSideInclude(id, url) {
			var req = false;
			// For Safari, Firefox, and other non-MS browsers
			if (window.XMLHttpRequest) {
				try {
					req = new XMLHttpRequest();
				} catch (e) {
					req = false;
				}
			} else if (window.ActiveXObject) {
				// For Internet Explorer on Windows
				try {
					req = new ActiveXObject("Msxml2.XMLHTTP");
				} catch (e) {
					try {
						req = new ActiveXObject("Microsoft.XMLHTTP");
					} catch (e) {
						req = false;
					}
				}
			}

			var element = document.getElementById(id);
			if (!element) {
				alert("Bad id " + id +
					"passed to clientSideInclude." +
					"You need a div or span element " +
					"with this id in your page.");
				return;
			}
			if (req) {
				// Synchronous request, wait till we have it all
				req.open('GET', url, false);
				req.send(null);
				element.innerHTML = req.responseText;
				$('.tomato-grid').addClass('line-table');
			} else {
				element.innerHTML =
				"Sorry, your browser does not support " +
				"XMLHTTPRequest objects. This page requires " +
				"Internet Explorer 5 or better for Windows, " +
				"or Firefox for any system, or Safari. Other " +
				"compatible browsers may also exist.";
			}
		}
	</script>

	<input type="hidden" name="_nextpage" value="/#nas-ups.asp">
	<div class="box">
		<div class="heading">APC UPS Status</div>
		<div class="content">
			<span id="ups-status"></span>
		</div>
	</div>

	<div class="box">
		<div class="heading">UPS Response</div>
		<div class="content">
			<span id="ups-data"></span>
		</div>
	</div>

	<script type="text/javascript">init();</script>
</content>