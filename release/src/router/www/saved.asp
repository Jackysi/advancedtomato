<!DOCTYPE html>
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
		<title>[<% ident(); %>] Tomato</title>
		<style>
			body {
				font-family: Verdana;
				font-size: 13px;
				background: #f0f0f0;
			}

			#loader {
				width:100%;
				max-width: 250px;
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

			.btn {
				font-family: Verdana;
				display: inline-block;
				text-align: center;
				cursor: pointer;
				background-image: none;
				padding: 5px 16px;
				margin: 0;
				font-size: 11px;
				font-weight: bold !important;
				line-height: 1.42857143;
				color: #fff !important;
				background: #69baf2;
				transition: 0.1s ease-out;
				border-radius: 4px;
				-webkit-border-radius: 4px;
				border: 0;
				border-bottom: 2px solid #4297d0;
			}

			.btn:hover {
				background: #77c1f6;
			}

			.btn:active, .btn:focus {
				transition: none;
				margin-top: 2px;
				border-width: 0;
			}

			/* Pure CSS preloader */
			.spinner {
				display: inline-block;
				float: left;
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
		<script language="javascript">
			wait = parseInt('<% cgi_get("_nextwait"); %>', 10);
			if (isNaN(wait)) wait = 5;
			function tick()
			{
				clock.innerHTML = wait;

				if (--wait >= 0) setTimeout(tick, 1000);
				else go();
			}
			function go()
			{
				clock.style.visibility = 'hidden';
				window.location.replace('<% cgi_get("_nextpage"); %>');
			}
			function setSpin(x)
			{
				document.getElementById('spin').style.visibility = x ? 'visible' : 'hidden';
				spun = x;
			}
			function init()
			{
				if (wait > 0) {
					spin = document.getElementById('spin');
					opacity = 1;
					step = 1 / wait;
					clock = document.getElementById('xclock');
					clock.style.visibility = 'visible';
					tick();
				}
			}
		</script>
	</head>
	<body onload="init()" onclick="go()">

		<div id="loader">
			<script type="text/javascript">
				if (wait <= 0) s = 'Changes Saved!<br /><br /> <button onclick="go()" class="btn">Continue</button>';
				else s = '<div class="spinner"></div> Working, please wait... <b>(<span id="xclock" style="visibility:hidden">&nbsp;</span>)</b>';
				document.write(s);
			</script>
		</div>

	</body>
</html>