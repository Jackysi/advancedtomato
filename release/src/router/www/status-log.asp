<title>Router Logs</title>
<content>
<script type="text/javascript">
	//<% nvram("at_update,tomatoanon_answer,log_file"); %>

	function find()
	{
		var s = E('find-text').value;
		if (s.length) document.location = 'logs/view.cgi?find=' + escapeCGI(s) + '&_http_id=' + nvram.http_id;
	}

	function init()
	{
		var e = E('find-text');
		if (e) e.onkeypress = function(ev) {
			if (checkEvent(ev).keyCode == 13) find();
			}
	}
</script>

<h3>View Router Logs</h3>

<div id="logging">
	<div class="section">
		<a href="logs/view.cgi?which=25&_http_id=<% nv(http_id) %>">View Last 25 Lines</a><br>
		<a href="logs/view.cgi?which=50&_http_id=<% nv(http_id) %>">View Last 50 Lines</a><br>
		<a href="logs/view.cgi?which=100&_http_id=<% nv(http_id) %>">View Last 100 Lines</a><br>
		<a href="logs/view.cgi?which=all&_http_id=<% nv(http_id) %>">View All</a><br><br>
		<a href="logs/syslog.txt?_http_id=<% nv(http_id) %>">Download Log File</a><br><br>
		<div class="input-append"><input class="span3" type="text" maxsize="32" id="find-text"> <button value="Find" onclick="find()" class="btn">Find <i class="icon-search"></i></button></div>
		&nbsp; &raquo; &nbsp;<a class="ajaxload" href="admin-log.asp">Logging Configuration</a><br><br>
	</div>
</div>

<script type="text/javascript">
	if (nvram.log_file != '1') {
		$('#logging').after('<div class="alert alert-info">Internal logging disabled.</b><br><br><a href="admin-log.asp">Enable &raquo;</a></div>');
		E('logging').style.display = 'none';
	}
</script>
<script type="text/javascript">init()</script>