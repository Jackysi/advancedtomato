<!--
Tomato GUI
Copyright (C) 2012 Shibby
http://openlinksys.info
For use with Tomato Firmware only.
No part of this file may be used without permission.
--><title>TomatoAnon Project</title>
<content>
	<script type="text/javascript">
		//	<% nvram("tomatoanon_enable,tomatoanon_answer,tomatoanon_id"); %>
		$('.anonlink').append('<a title="Check out my router" class="pull-right" href="http://anon.groov.pl/?search=9&routerid=<% nv('tomatoanon_id'); %>" target="_blank"><i class="icon-forward"></i></a>');
		function verifyFields(focused, quiet)
		{
			var o = (E('_tomatoanon_answer').value == '1');
			E('_tomatoanon_enable').disabled = !o;
			var s = (E('_tomatoanon_enable').value == '1');
			return 1;
		}
		function save()
		{
			if (verifyFields(null, 0)==0) return;
			var fom = E('_fom');
			fom._service.value = 'tomatoanon-restart';
			form.submit('_fom', 1);
		}
		function init()
		{
			var anon = true;
		}

		function submit_complete()
		{
			document.location.reload();
		}
	</script>


	<form id="_fom" method="post" action="tomato.cgi">
		<input type="hidden" name="_nextpage" value="/#admin-tomatoanon.asp">
		<input type="hidden" name="_service" value="tomatoanon-restart">

		<div class="box">
			<div class="heading">TomatoAnon Project</div>
			<div class="content">
				<p>I would like to present you with a new project I've been working on, called TomatoAnon.
					The TomatoAnon script will send information to an online database about your router's model and installed version of Tomato.
					The information submitted is 100% anonymous and will ONLY be used for statistical purposes.
					<b>This script does NOT collect or transmit any private or personal information whatsoever (e.g MAC addresses, IP addresses, etc.)!</b>
					The TomatoAnon script is fully open, and written in bash. Everyone is free to look at the information collected and transmitted to the database..
				</p>

				<p>The collected data can be viewed on the <a href="http://anon.groov.pl/" target="_blank"><b>TomatoAnon statistics</b></a> page.<br>
					This information can assist in helping you select the best and most popular router available in your country or region.
					You can find the most commonly used version of Tomato and which version is the most stable on each router model.
					The TomatoAnon script can be disabled in cases where you prefer not to contribute data or are uncomfortable with the data being collected..
					You can always re-enable it at any time.
				</p>

				<p>The following data is collected and transmitted by TomatoAnon:</p>
				<ul>
					<li>MD5SUM of WAN+LAN MAC addresses - this provides a unique identifier for each router, e.g: 1c1dbd4202d794251ec1acf1211bb2c8</li>
					<li>Model of router, e.g: Asus RT-N66U</li>
					<li>Installed version of Tomato, e.g: 102 K26 USB</li>
					<li>Build Type, e.g: Mega-VPN-64K</li>
					<li>Country, e.g: POLAND</li>
					<li>ISO Country Code, e.g: PL</li>
					<li>Router uptime, e.g: 3 days</li>
					<li>That`s it!!</li>
				</ul>

				<p>Thank you for reading and please make the right choice to help this project.</p><br />

				<h3>Tomato Update Notifications</h3>
				<p>
					AdvancedTomato includes additions to the TomatoAnon script to provide an automatic update check mechanism.
					As the TomatoAnon script transmits collected data back to the TomatoAnon project, the script will also check the installed version of AdvancedTomato with the latest version available.
					Just like TomatoAnon, the code is freely available for viewing to ensure no other sensitive information is being collected.
				</p><br />

				<h4>How does it work?</h4>
				<p>AdvancedTomato retrieves your router's current Tomato version and creates a small link on the page which looks like this: <b><a target="_blank" href="http://advancedtomato.com/update.php?v=1.06.08">http://advancedtomato.com/update.php?v=1.06.08</a></b>.
					Your web browser will follow the link and the AdvancedTomato server sends a response indicating whether or not a newer version is available.<br>
					That's it!
				</p>
			</div>
		</div>

		<div class="box anon">
			<div class="heading anonlink">TomatoAnon Settings</div>
			<div class="content"></div>
			<script type="text/javascript">
				$('.box.anon .content').forms([
					{ title: 'Do you understand what TomatoAnon does?', name: 'tomatoanon_answer', type: 'select', options: [ ['0','No, I don\'t. I will need to read the above information and make an informed decision.'], ['1','Yes, I have and would like to make a decision.'] ], value: nvram.tomatoanon_answer, suffix: ' '},
					{ title: 'Do you want to enable TomatoAnon?', name: 'tomatoanon_enable', type: 'select', options: [ ['-1','I`m not sure right now.'], ['1','Yes, I\'m sure I would like to enable it'], ['0','No, I would not like to enable it'] ], value: nvram.tomatoanon_enable, suffix: ' '}
				]);
			</script>
		</div>

		<button type="button" value="Save" id="save-button" onclick="save()" class="btn btn-primary">Save <i class="icon-check"></i></button>
		<button type="button" value="Cancel" id="cancel-button" onclick="javascript:reloadPage();" class="btn">Cancel <i class="icon-cancel"></i></button>
		<span id="footer-msg" class="alert alert-warning" style="visibility: hidden;"></span>

	</form>

	<script type="text/javascript">verifyFields(null, 1); init();</script>
</content>