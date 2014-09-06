<!DOCTYPE html>
<html lang="en">
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<title>[<% ident(); %>] Error</title>
		<style>
			body {
				font-family: 'Verdana';
				font-size: 13px;
			}
			.container {
				width: 700px;
				margin: 50px auto;
			}

			h2 {
				font-size: 20px;
			}

			.btn {
				font-family: Verdana;
				display: inline-block;
				text-align: center;
				cursor: pointer;
				background-image: none;
				padding: 8px 20px;
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

	</head>

	<body>

		<div class="container" style="text-align: center;">
			<div class="row">
				<h2>Somthing went wrong...</h2>
				<p><script type="text/javascript">
						//<% resmsg("Unknown error"); %>
						document.write(resmsg);
					</script></p>
				<p><a class="btn btn-primary" onclick="history.go(-1)">Go Back</a></p>
			</div>
		</div>

	</body>
</html>