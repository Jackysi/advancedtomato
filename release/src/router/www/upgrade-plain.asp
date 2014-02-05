<html><head><title>Firmware Upgrade</title></head>
<body>
<h1>Firmware Upgrade</h1>
<b>WARNING:</b>
<ul>
<li>There is no upload status information in this page and there will be no
change in the display after the Upgrade button is pushed. You will be shown a
new page only after the upgrade completes.
<li>It may take up to 3 minutes for the upgrade to complete. Do not interrupt
the router or the browser during this time.
</ul>

<br>
<form name="firmware_upgrade" method="post" action="upgrade.cgi?<% nv(http_id) %>" encType="multipart/form-data">
<input type="hidden" name="submit_button" value="Upgrade">
Firmware: <input type="file" name="file"> <input type="submit" value="Upgrade">
</form>
</body></html>
