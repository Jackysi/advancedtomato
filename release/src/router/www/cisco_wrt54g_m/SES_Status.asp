
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/ses.js"></SCRIPT>
<SCRIPT language=JavaScript>
<% langpack(); %>

var ses = '<% get_ses_status(); %>';

function auto_return() 
{
	document.location.href =  "Wireless_Basic.asp";
	setTimeout('auto_return()', 10000);
}

function init()
{
	setTimeout('auto_return()', 10000);		
}

</SCRIPT>
</head>
<body bgcolor="black" onload=init()>
<form name=success>
<center><table BORDER=0 CELLSPACING=0 CELLPADDING=0 WIDTH=557 >
<tr BGCOLOR="white">
<th HEIGHT=400><font face="Verdana" size=4 color="black"></font>
<script>
ses += "!\n";
document.write("<p><font face=Verdana size=4 color=black><B>" + SW_SES_BTN.<% get_ses_status(); %> +"!\n</B>");
document.write("<p><font face=Verdana size=4 color=black>" + SW_SES_BTN.MSG1);
</script>
</th>
</tr>
</table></center>
</form>
</body>

