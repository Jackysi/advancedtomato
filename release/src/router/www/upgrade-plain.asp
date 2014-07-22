<html>
    <head><title>Firmware Upgrade</title>
    <script type="text/javascript">
    function upgrade() {
        document.form_upgrade.submit();
    }
    </script>
    </head>
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
        <form name="form_upgrade" method="post" action="upgrade.cgi?_http_id=<% nv(http_id); %>" encType="multipart/form-data">
            <label>Select the file to use:</label>
            <input type="file" name="file" size="50"> <button type="button" value="Upgrade" id="afu-upgrade-button" onclick="upgrade()" class="btn btn-danger">Upgrade</button>
        </form>
    </body>
</html>