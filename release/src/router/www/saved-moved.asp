<!DOCTYPE html>
<!--
Tomato GUI
Copyright (C) 2006-2010 Jonathan Zarate
http://www.polarcloud.com/tomato/

For use with Tomato Firmware only.
No part of this file may be used without permission.
-->
<html lang="en">
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">

		<title>[<% ident(); %>] Restarting...</title>
		<style>
			body {
				font-family: Verdana;
				font-size: 12px;
				background-image: url(data:image/png;base64,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);
				background-color: #fff;
			}

			#loader {
				width:95%;
				max-width: 600px;
				background: #fff;
				border: 1px solid #E6E6E6;
				margin: 15% auto;
				padding: 15px;
				text-align: center;
				border-radius: 5px;
				-moz-border-radius: 5px;
				-webkit-border-radius: 5px;
				-o-border-radius: 5px;
				box-shadow: 0 0 2px #fff;
				-moz-box-shadow: 0 0 2px #fff;
				-webkit-box-shadow: 0 0 2px #fff;
				-o-box-shadow: 0 0 2px #fff;
			}

			/* Pure CSS preloader */
			.spinner {
				display: inline-block;
				width: 18px;
				height: 18px;
				box-sizing: border-box;
				vertical-align: middle;

				border: solid 2px transparent;
				border-top-color: #3C3C3C;
				border-bottom-color: #3C3C3C;
				border-radius: 50%;
				-webkit-border-radius: 50%;

				-webkit-animation: tomato-spinner 600ms linear infinite;
				animation: tomato-spinner 600ms linear infinite; 
			}

			@-webkit-keyframes tomato-spinner { 0%   { -webkit-transform: rotate(0deg); }  100% { -webkit-transform: rotate(360deg); } }
			@keyframes tomato-spinner { 0%   { transform: rotate(0deg); }  100% { transform: rotate(360deg); } }
		</style>
		<script language='javascript'>
			var n = 20;
			function tick()
			{
				var e = document.getElementById('continue');
				e.innerHTML = n;
				if (n == 10) {
					e.disabled = false;
				}
				if (n == 0) {
					e.innerHTML = 'Continue';
				}
				else {
					--n;
					setTimeout(tick, 1000);
				}
			}
			function go()
			{
				window.location = window.location.protocol + '//<% nv("lan_ipaddr"); %>/';
			}
		</script>
	</head>
	<body onload="tick()" onclick="go()">

		<div id="loader">
			<div class="spinner"></div>
			The router's new IP address is <% nv("lan_ipaddr"); %>.<br /> You may need to release then renew your computer's DHCP lease before continuing.
			<br /><br />
			Please wait while the router restarts... &nbsp;
			<button class="btn" id="continue" onclick="go()" disabled>Continue</button>
		</div>

	</body>
</html>