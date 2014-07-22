<!DOCTYPE html>
<html lang="en">
	<head>
		<meta http-equiv="content-type" content="text/html;charset=utf-8">
		<meta name="robots" content="noindex,nofollow">
		<title>[<% ident(); %>] Error</title>
		<style>
			body {
				font-family: 'Segoe UI', 'Verdana';
			}
			.container {
				width: 700px;
				margin: 0 auto;
			}

			.btn {
				display: inline-block;
				padding: 10px 14px;
				margin: 0;
				line-height: 14px;
				font-size: 12px;
				color: #fff;
				background: #505050;
				cursor: pointer;
				transition: 0.1s ease-out;
				-webkit-transition: 0.1s ease-out;
				border-radius: 3px;
				-webkit-border-radius: 3px;
				border: 0;
			}

			.btn:hover,
			.btn:focus {
				background: #646464;
				color: #fff;
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