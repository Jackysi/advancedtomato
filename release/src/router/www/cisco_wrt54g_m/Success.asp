
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
var wait_time = '<% webs_get("wait_time"); %>';
function to_submit()
{
	var t2 = new Date().getTime();

	if(submit_button == "")
		history.go(-1);
	else if(submit_button == "WL_WEPTable.asp")
		self.close();
	else {
		if(wait_time != "0" && wait_time != "") {
			//delay(wait_time*1000 - (t2-t1));
			DelayTime = wait_time * 1000 - (t2-t1);
			choose_disable(document.success.action);
			if(DelayTime < 0)
				DelayTime = 0;
			setTimeout('returnpage()',DelayTime);
		}
		else
			document.location.href =  "<% get_http_prefix(""); %>"+submit_button;
	}
}

function auto_return()
{
	var t2 = new Date().getTime();
	//delay(wait_time*1000 - (t2-t1));
	DelayTime = wait_time * 1000 - (t2-t1);
	//choose_disable(document.success.action);
	if(DelayTime < 0)
		DelayTime = 0;
	setTimeout('returnpage()',DelayTime);
}

function returnpage()
{
	document.location.href =  "<% get_http_prefix(""); %>"+submit_button;
}
function init()
{
	if(wait_time != "0" && wait_time != "") {
		DelayTime = wait_time * 1000 ;
		setTimeout('returnpage()', DelayTime);		
	}
}
</SCRIPT>
</head>
<body bgcolor="black" onload=init()>
<form name=success>
<center><table BORDER=0 CELLSPACING=0 CELLPADDING=0 WIDTH=557 >
<tr BGCOLOR="white">
<th HEIGHT=400><font face="Verdana" size=4 color="black"><script>Capture(other.setsuc)</script></font>
<script>
var wait_time = '<% webs_get("wait_time"); %>';
	if(wait_time != "0" && wait_time != "") {
		document.write("<p><font face=Verdana size=2 color=black>" + succ.autoreturn);
	}
	else {
//		document.write("<p><p><input type=button name=action value=Continue OnClick=to_submit()>");
document.write("<p><p><input type=button name=action" + " value=" + sbutton.continue1 + " onClick=to_submit()>");
	}
</script>
</th>
</tr>
</table></center>
</form>
</body>

