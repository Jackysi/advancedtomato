
<!--
*********************************************************
*   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
*********************************************************

This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
the contents of this file may not be disclosed to third parties,
copied or duplicated in any form without the prior written
permission of CyberTAN Inc.

This software should be used as a reference only, and it not
intended for production use!


THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
-->

<html>
<head>
<% no_cache(); %>
<% charset(); %>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
<% langpack(); %>
var submit_button = '<% get_web_page_name(); %>';
function to_submit()
{
	if(submit_button == "")
		history.go(-1);
	else if(submit_button == 'WL_WEPTable.asp')
		self.close();
	else if(submit_button == 'Register_ok.asp'){
		document.location.href =  "HotSpot_Admin.asp";
	}
	else
		document.location.href =  submit_button;
	
}
</SCRIPT>
</head>
<body bgcolor="white">
<form>
<br><br><br><br>
<center>
<tr BGCOLOR="white">
<th><font face="Verdana" size=4  color="black"><script>Capture(other.setsuc)</script></font>
<p><p>

<script>document.write("<input type=button name=action" + " value=" + sbutton.continue1 + " onClick=to_submit()>");</script>

</th>
</tr></center>
</form>
</body>


