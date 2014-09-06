<!DOCTYPE html>
<html lang="en">
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<title>[<% ident(); %>] Rebooting...</title>

		<style>
			body {
				font: 13px 'Verdana', sans-serif;
				background-color: #fafafa;
				color: #585858;
			}
			.progress {
				position: relative;
				overflow: hidden;
				height: 20px;
				margin: 10px 0;
				background-color: #f0f0f0;
				border-radius: 3px;
				-webkit-border-radius: 3px;
				box-shadow: inset 0 -1px 0 rgba(0,0,0,0.02);
				-webkit-box-shadow: inset 0 -1px 0 rgba(0,0,0,0.02);
			}
			.progress .bar {
				float: left;
				width: 0%;
				height: 100%;
				color: #ffffff;
				text-align: center;
				background-color: #69baf2;
				-webkit-transition: width 0.6s ease;
				transition: width 0.6s ease;
				box-shadow: inset 0 -1px 0 rgba(0,0,0,.15);
				-webkit-box-shadow: inset 0 -1px 0 rgba(0,0,0,.15);
			}

			.bar .txt {
				color: #000;
				position: absolute;
				width: 100%;
				top: 0;
				left: 0;
				text-align: center;
				font-size: 11px;
				line-height: 20px;
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

		</style>
		<script type="text/javascript">
			var Max = 80 + parseInt('0<% nv("wait_time"); %>');
			var n = 0;
			function tick()
			{

				var e = document.getElementById('prog');
				var d = document.getElementById('progTXT');
				var c = document.getElementById('continue');

				e.style.width = (((n++) / Max) * 100) + '%';
				d.innerHTML = (Max - n) + 's';

				if (n == Max) {
					d.innerHTML = '';
					e.style.width = '100%';
					go();
					return;
				}

				if (n == (Max-15)) c.style.display = 'block';
				setTimeout(tick, 1000);
			}
			function go()
			{
				window.location.replace('/');
			}
			function init()
			{
				resmsg = '';
				//<% resmsg(); %>
				if (resmsg.length) {
					e = document.getElementById('msg');
					e.innerHTML = resmsg;
					e.style.display = '';
				}
				tick()
			}

		</script>
	</head>

	<body>
		<div style="width:100%; max-width: 600px; margin: 10% auto; text-align: center;">
			<div style='width:90%; margin:5px auto;padding:5px 5%;'>
				<span id="msg"></span><br /><span id="re"><div class="spinner"></div> <b>Rebooting, please wait...</b></span><br />
				<div id="progbar" class="progress">
					<div class="bar success" id="prog">
						<span class="txt" id="progTXT"></span>
					</div>
				</div>
				<div id="continue" style="display: none;">
					<button class="btn" type="button" name="go" onclick="go()">Continue</button>
				</div>
				<script type="text/javascript">init();</script>
			</div>
		</div>
	</body>
</html>