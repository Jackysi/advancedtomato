
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

<HTML><HEAD><TITLE>DHCP Clients Table</TITLE>
<% no_cache(); %>
<% charset(); %>
<link rel="stylesheet" type="text/css" href="style.css">
<script src="common.js"></script>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capstatus.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<script language="JavaScript">
function DHCPAct(F)
{
       F.submit_button.value="DHCPTable";
       F.submit_type.value="delete";
       F.change_action.value="gozila_cgi";
       F.submit();
}
</script>
</HEAD>
<BODY bgColor=white onload={window.focus();}>
<form method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=submit_type>
<CENTER>
<TABLE height=108 cellSpacing=1 width=673 border=0>
  <TBODY>
  <TR>
    <TD width=566 colSpan=5 height=27><FONT face=Arial color=blue><B><SPAN STYLE="FONT-SIZE: 14pt"><script>Capture(dhcptbl.actip)</script></SPAN></B></FONT></TD>
    </TR>
  <TR>
    <TD align=left colSpan=4 height=30><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(dhcptbl.dhcpsrvip)</script>&nbsp;:&nbsp;</SPAN>
	<B>&nbsp; <% nvram_get("lan_ipaddr"); %> </B></FONT></TD>
    <TD align=center height=30>

<script>document.write("<INPUT onclick=window.location.replace(\"DHCPTable.asp\") type=button name=refresh_button value=\"" + sbutton.refresh + "\">");</script>

   </TD></TR>
  <TR align=middle bgColor=#b3b3b3>
    <TH height=32><FONT color=black><script>Capture(share.clihostname)</script></FONT></TH>
    <TH height=32><FONT color=black><script>Capture(share.ipaddr)</script></FONT></TH>
    <TH height=32><FONT color=black><script>Capture(share.macaddr)</script></FONT></TH>
    <TH height=32><FONT color=black><script>Capture(dhcptbl.expires)</script></FONT></TH>
    <TD height=32>

<script>document.write("<input type=button name=action onClick=DHCPAct(this.form) value=\"" + sbutton.del + "\">");</script>

    </TH>
<script language=javascript>
var table = new Array(
<% dumpleases(""); %>
);
var i = 0;
var count = 0;
for(;;){
	if(!table[i]){
		if(i == 0){
			document.write("<tr bgcolor=cccccc align=middle>");
			document.write("<td>"+wlanadv.none+"</td>");
			document.write("<td>"+wlanadv.none+"</td>");
			document.write("<td>"+wlanadv.none+"</td>");
			document.write("<td>"+wlanadv.none+"</td>");
			document.write("<td>&nbsp;</td></tr>");
		}
		break;
	}
	document.write("<tr bgcolor=cccccc align=middle>");
	document.write("<td>"+table[i]+"</td>");
	document.write("<td>"+table[i+1]+"</td>");
	document.write("<td>"+table[i+2]+"</td>");
	document.write("<td>"+table[i+3]+"</td>");
	document.write("<td><input type=checkbox name=d_"+count+" value="+table[i+4]+"></td></tr>");
	count ++;
	i = i + 5;
}
</script>
  <TR align=middle bgColor=#cccccc>
    <TD bgcolor="#FFFFFF" height="35"></TD>
    <TD bgcolor="#FFFFFF" height="35"></TD>
    <TD bgcolor="#FFFFFF" height="35"></TD>
    <TD bgcolor="#FFFFFF" height="35"></TD>
    <TD bgcolor="#FFFFFF" height="35"> 

<script>document.write("<INPUT onclick=self.close() type=reset name=close_button value=\"" + sbutton.close + "\">");</script>

    </TD></TR>
</TBODY>
</TABLE>
</CENTER>
</FORM>
</BODY>
</HTML>
