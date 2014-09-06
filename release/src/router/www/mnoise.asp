<html lang="en">
	<!--
	Tomato GUI
	Copyright (C) 2006-2010 Jonathan Zarate
	http://www.polarcloud.com/tomato/

	For use with Tomato Firmware only.
	No part of this file may be used without permission.
	-->
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<title>[<% ident(); %>] Measuring Noise...</title>

		<link href="css/bootstrap.min.css" rel="stylesheet">
		<style>
			body {
				font-family: Verdana;
				font-size: 12px;
				background: #f0f0f0;
			}

			#loader {
				width:100%;
				max-width: 450px;
				text-align: center;
				background: #fff;
				border: 1px solid #E1E1E1;
				margin: 15% auto;
				padding: 20px;
				border-width: 1px;
				border-style: solid;
				border-color: #EEE #EEE #D1D2D3;
				background: none repeat scroll 0% 0% #FFF;
				border-radius: 4px;
				-webkit-border-radius: 4px;
				box-shadow: 0px 1px 1px rgba(0, 0, 0, 0.05);
				-webkit-box-shadow: 0px 1px 1px rgba(0, 0, 0, 0.05);
			}

			/* Pure CSS preloader */
			.spinner {
				display: inline-block;
				width: 15px;
				height: 15px;
				vertical-align: middle;

				border: solid 1px #3498DB;
				border-color: #3498DB #3498DB transparent transparent;
				border-width: 0 2px 2px;
				border-radius: 50%;
				-webkit-border-radius: 50%;

				-webkit-animation: tomato-spinner 600ms linear infinite;
				animation: tomato-spinner 600ms linear infinite;
			}

			@-webkit-keyframes tomato-spinner { 0%   { -webkit-transform: rotate(0deg); }  100% { -webkit-transform: rotate(360deg); } }
			@keyframes tomato-spinner { 0%   { transform: rotate(0deg); }  100% { transform: rotate(360deg); } }
		</style>
		<script type="text/javascript">
			function tick()
			{
				t.innerHTML = tock + ' second' + ((tock == 1) ? '' : 's');
				if (--tock >= 0) setTimeout(tick, 1000);
				else history.go(-1);
			}
			function init()
			{
				t = document.getElementById('time');
				tock = 15;
				tick();
			}
		</script>
	</head>

	<body onload="init()" onclick="go()">

		<div id="loader">
			<div style="font-size: 14px; font-weight: bold; padding-bottom: 5px;"><div class="spinner"></div>&nbsp; Measuring radio noise floor...</div>
			Wireless access has been temporarily disabled for <span id="time">15 seconds</span>.
		</div>

	</body>
</html>