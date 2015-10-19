<!DOCTYPE html>
<html lang="en">
	<head>
		<meta charset="utf-8">
		<meta name="viewport" content="width=960">
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<title>[<% ident(); %>]: Basic</title>
		<link href="css/reset.css" rel="stylesheet">
		<link href="css/style.css" rel="stylesheet">
		<link rel="shortcut icon" href="/favicon.ico" />
		<% css(); %>

		<script type="text/javascript" src="js/jquery.min.js"></script>
		<script type="text/javascript" src="tomato.js"></script>
		<script type="text/javascript" src="js/advancedtomato.js"></script>

		<script type="text/javascript">

			var routerName = '[<% ident(); %>] ';
			//<% nvram("web_nav,at_update,at_navi,tomatoanon_answer"); %>
			//<% anonupdate(); %>

			// Fix for system data display
			var refTimer, wl_ifaces = {}, ajaxLoadingState = false, gui_version = "<% version(0); %>";

			$(document).ready(function() {

				gui_version = gui_version.match(/^1.28\.0000 (MIPSR2\-|\-)?(.*)/)[2] || '';
				$('#gui-version').html('<i class="icon-info-alt"></i> <span class="nav-collapse-hide">' + gui_version + '</span>');
				AdvancedTomato();

			});
		</script>
	</head>
	<body>
		<div id="wrapper">

			<div class="top-header">
			
				<a href="/">
					<div class="logo">

						<svg version="1.1" id="logo" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
							width="26px" height="26px" viewBox="0 0 32 32" enable-background="new 0 0 32 32" xml:space="preserve">
							<path fill-rule="evenodd" clip-rule="evenodd" fill="#fff" d="M19.4,10.5C19.4,10.5,19.4,10.5,19.4,10.5c0-0.1,0-0.1,0-0.2
								C19.4,10.2,19.4,10.5,19.4,10.5z M25.2,5.3c-0.4,0.5-1,0.9-1.7,1.4c1.2,0.9,1.8,2,3.2,1.7c0,0-0.3,1.9-2.5,2.7
								c-1.7,0.6-3.3,0.4-4.7-0.6c0,1.5-0.5,4.4-0.4,5.8c-0.1,0-4.9-1.4-5.3-5.7c-1.7,1.1-3.4,1.6-6.1,0.8c-1.3-0.4-2.5-2.1-2.4-2.7
								C7.4,8.8,7.3,8.4,9.4,6c0,0,0.2-0.2,0.2-0.3C9.3,5.6,9,5.4,8.7,5.2c-0.3,0-0.6,0-0.9,0C2.4,5.2,0,10.4,0,16s1.9,16,16,16
								c14.1,0,16-10.5,16-16C32,10.8,29.3,6,25.2,5.3z M14.1,5.4c-0.7,0.7-2,0.2-2.5,0.6C9.5,8.1,9.2,9.4,7.5,9.4c0,0.4,2,0.9,2.6,0.9
								c2.4,0,4.9-2.4,4.9-3.8c0,0,0.1,1.4,0.1,2.8c0,3.2,2.5,4.6,2.6,4.6c0-1,0.3-2.8,0.3-3.7c0-2.9-1-2.7-1-3.8c0.7,0,2.8,3.5,5.4,3.5
								c1.6,0,2.3-0.8,2.3-0.8C23,9.1,21.3,6,19.9,6c2,0,4.4-1.4,4.4-2.6c-0.1,0-0.4,0.3-2.5,0.3c-4.2,0-4,0.9-4.6,0.9
								c0-2.5,1.1-3.9,1.6-4.2c0.6-0.3,0.2-0.5-0.4-0.4c-1.4,0.3-3.3,3.7-3.3,4.7c-0.2-1.4-2.9-2.1-4-2.1c-1.6,0-2.1-0.7-2.6-1.2
								C8.6,1.6,7.9,5,14.1,5.4z"/>
						</svg>
						<h1 class="nav-collapse-hide">Advanced<span>Tomato</span></h1>

						<h2 class="currentpage nav-collapse-hide"></h2>
					</div>
				</a>
				
				<div class="left-container">
					<a data-toggle="tooltip" title="Toggle Collapsed Navigation" href="#" class="toggle-nav"><i class="icon-align-left"></i></a>
				</div>

				<div class="pull-right links">
					<ul>
						<li><a title="Tools" href="#tools-ping.asp">Tools <i class="icon-tools"></i></a></li>
						<li><a title="Bandwidth" href="#bwm-realtime.asp">Bandwidth <i class="icon-graphs"></i></a></li>
						<li><a title="IP Traffic" href="#bwm-ipt-realtime.asp">IP Traffic <i class="icon-globe"></i></a></li>
						<li><a title="System" id="system-ui" href="#system">System <i class="icon-system"></i></a></li>
					</ul>
					<div class="system-ui">

						<div class="datasystem align center"></div>

						<div class="router-control">
							<a href="#" class="btn btn-primary" onclick="reboot();">Reboot <i class="icon-reboot"></i></a>
							<a href="#" class="btn btn-danger" onclick="shutdown();">Shutdown <i class="icon-power"></i></a>
							<a href="#" onclick="logout();" class="btn">Logout <i class="icon-logout"></i></a>
						</div>
					</div>
				</div>
			</div>

			<div class="navigation">
				<ul>
					<li class="nav-footer" id="gui-version" style="cursor: pointer;" onclick="loadPage('#about.asp');"></li>
				</ul>
			</div>


			<div class="container">
				<div class="ajaxwrap"></div>
				<div class="clearfix"></div>
			</div>

		</div>
	</body>
</html>