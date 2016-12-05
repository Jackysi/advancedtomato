<!DOCTYPE html>
<html lang="en">
	<head>
		<meta charset="utf-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<meta http-equiv="content-type" content="text/html; charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<title>[<% ident(); %>]: Home</title>

		<!-- Interface Design -->
		<link href="/css/interface.css" rel="stylesheet">
		<% css(); %>

		<!-- Load Favicon (icon) -->
		<link rel="shortcut icon" href="/favicon.ico">

		<!-- Variables which we keep through whole GUI, also determine Tomato version here -->
		<script type="text/javascript">

			// AdvancedTomato related object
			var gui = {
				'version'      : "<% version(0); %>",
			};

			// On DOM Ready, parse GUI version and create navigation
			window.onload = function() {

				// Attempt match
				match_regex = gui.version.match( /^1\.28\.0000.*?([0-9]{1,3}\.[0-9]{1}\-[0-9]{3}).* ([a-z0-9\-]+)$/i );

				// Check matches
				if ( match_regex != null && match_regex[ 1 ] != null ) {

					gui.version = 'v' + match_regex[ 1 ];

				}

				// Write version & initiate GUI functions & binds
				document.getElementById('version_container').innerHTML = gui.version;

				// Animate login
				var div_elm = document.getElementById('login-container-div');
				div_elm.className += ' login-form-onload';

				// Set redirect if browser will show url
				if ( window.location.hash != '' ) {

					document.getElementById('redirect_to').value = '/' + window.location.hash;

				}

			};

		</script>
	</head>

	<body class="login-screen">

		<section class="col-sm-6 login-form" id="login-container-div">

			<form class="form-horizontal" method="POST" action="/login.cgi">
				<input type="hidden" name="_redirect" value="" id="redirect_to">

				<div class="login-container" >

					<div class="login-header">
						<h2><svg version="1.1" id="logo-icon" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
		                    width="26px" height="26px" viewBox="0 0 32 32" xml:space="preserve">
		                    <path fill-rule="evenodd" clip-rule="evenodd" fill="#E74C3C" d="M19.4,10.5C19.4,10.5,19.4,10.5,19.4,10.5c0-0.1,0-0.1,0-0.2
		                        C19.4,10.2,19.4,10.5,19.4,10.5z M25.2,5.3c-0.4,0.5-1,0.9-1.7,1.4c1.2,0.9,1.8,2,3.2,1.7c0,0-0.3,1.9-2.5,2.7
		                        c-1.7,0.6-3.3,0.4-4.7-0.6c0,1.5-0.5,4.4-0.4,5.8c-0.1,0-4.9-1.4-5.3-5.7c-1.7,1.1-3.4,1.6-6.1,0.8c-1.3-0.4-2.5-2.1-2.4-2.7
		                        C7.4,8.8,7.3,8.4,9.4,6c0,0,0.2-0.2,0.2-0.3C9.3,5.6,9,5.4,8.7,5.2c-0.3,0-0.6,0-0.9,0C2.4,5.2,0,10.4,0,16s1.9,16,16,16
		                        c14.1,0,16-10.5,16-16C32,10.8,29.3,6,25.2,5.3z M14.1,5.4c-0.7,0.7-2,0.2-2.5,0.6C9.5,8.1,9.2,9.4,7.5,9.4c0,0.4,2,0.9,2.6,0.9
		                        c2.4,0,4.9-2.4,4.9-3.8c0,0,0.1,1.4,0.1,2.8c0,3.2,2.5,4.6,2.6,4.6c0-1,0.3-2.8,0.3-3.7c0-2.9-1-2.7-1-3.8c0.7,0,2.8,3.5,5.4,3.5
		                        c1.6,0,2.3-0.8,2.3-0.8C23,9.1,21.3,6,19.9,6c2,0,4.4-1.4,4.4-2.6c-0.1,0-0.4,0.3-2.5,0.3c-4.2,0-4,0.9-4.6,0.9
		                        c0-2.5,1.1-3.9,1.6-4.2c0.6-0.3,0.2-0.5-0.4-0.4c-1.4,0.3-3.3,3.7-3.3,4.7c-0.2-1.4-2.9-2.1-4-2.1c-1.6,0-2.1-0.7-2.6-1.2
		                        C8.6,1.6,7.9,5,14.1,5.4z"/>
		                </svg> AdvancedTomato</h2>
					</div>

					<div class="login-content">

						<div>Fill the details bellow to login</div><br>

						<div class="form-group">
							<input type="text" name="username" class="form-control" placeholder="Username" id="username" autofocus required value="admin">
						</div>

						<div class="form-group">
							<input type="password" name="password" class="form-control" placeholder="Password" id="password" autocomplete="off" required value="password">
						</div>

						<div class="divider"></div>
						<button type="submit" name="submit" value="sign-in" class="btn btn-success pull-right">Login <svg width="14" height="14" fill="#fff" style="display: inline-block; vertical-align: middle; margin-top: -4px;" viewBox="0 0 1000 1000" xmlns="http://www.w3.org/2000/svg"><path d="M556.9,646.7h119.6v183.2c0,21.2-3.2,39.7-9.7,55.5S652,914,641.4,924c-10.6,10-23,17.3-37.1,22.1s-29.3,7.2-45.3,7.2H132.5c-15.4,0-30.5-3.1-45.3-9.2C72.3,938,59.3,929.5,48,918.6S27.7,894.8,20.6,880C13.5,865.1,10,848.8,10,830.8V165.2c0-15.4,3.7-30.3,11.1-44.4c7.4-14.1,16.8-26.7,28.5-37.6c11.5-11,24.5-19.8,38.6-26.5s28.3-10.1,42.4-10.1H554c16.7,0,32.5,3.6,47.2,11.1c14.8,7.4,27.8,16.7,39.1,28c11.3,11.3,20.1,23.9,26.5,38.1c6.5,14.1,9.7,28,9.7,41.4v177.4H556.9l0.9-184.3H131.5l-0.9,672.3h426.4V646.7L556.9,646.7z M971.6,466.4c23.8,19.3,24.5,38.3,1.9,56.9c-12.8,9.7-26.8,21.5-41.9,35.7c-15.1,14.1-30.8,28.7-47.2,43.9c-16.4,15.1-32.6,30-48.7,44.9c-16.1,14.8-30.8,27.7-44.4,38.6c-14.8,11.5-27.4,17.3-38.2,17.3c-10.6,0-15.9-7.8-15.9-23.2c1.3-8.9,1.4-20.3,0.5-33.8c-0.9-13.5-1.4-25.1-1.4-34.7c0-11-3.9-17.7-11.5-20.3c-7.8-2.6-17.1-3.9-28-3.9h-38.6h-58.9c-21.9,0-44.9-0.1-69-0.5c-24.1-0.4-47.1-0.5-69-0.5h-58.9h-39.6c-5.2,0-10.7-0.5-16.8-1.4c-6.1-0.9-11.5-3.1-16.4-6.2c-4.8-3.2-8.7-7.8-11.5-13.5c-2.9-5.8-4.4-13.2-4.4-22.1c0-12.2-0.1-26.7-0.5-43.4c-0.4-16.7-0.1-31.6,0.5-44.4c0.1-17.3,4.4-29.2,12.7-35.7c8.4-6.5,21.9-9.3,40.5-8.7h31.8h55.9c20.6,0,42.6,0.1,66.1,0.5c23.4,0.4,45.8,0.5,67,0.5h57.9h36.7c16.1,0.6,29.8-0.6,41-3.9c11.3-3.2,16.8-10.2,16.8-21.2c0-8.9,0.4-19.1,0.9-30.4c0.6-11.3,0.9-21.4,0.9-30.4c0-14.8,4-24.1,12-28s18.1-0.9,30.4,8.7c12.2,10.2,26.5,22.4,43,36.1c16.4,13.8,33,28,49.7,42.4c16.7,14.5,33.3,28.7,49.7,43C943.3,442.9,958.2,455.5,971.6,466.4L971.6,466.4z"/></svg></button>
						<div class="clearfix"></div>

					</div>
				</div>

				<div class="text-center version-info"><b id="version_container"></b></div>

			</form>
		</section>

	</body>
</html>
