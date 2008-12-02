
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

<HTML><HEAD><TITLE>Routing Table</TITLE>
<% no_cache(); %>
<% charset(); %>
<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capapp.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capsetup.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>
<script>
document.title = advroute.routetbllist;
</script>

</HEAD>
<BODY bgColor=white>
<FORM>
<CENTER>
<TABLE height=100 cellSpacing=1 width=673 border=0>
  <TBODY>
  <TR>
    <TD width=343 colSpan=2 height=30><B><FONT face=Arial color=#0000ff><SPAN STYLE="FONT-SIZE:14pt"><script>Capture(advroute.routetbllist)</script></SPAN></FONT></B></TD>
    <TD align=right width=140 height=30>&nbsp;</TD>
    <TD align=center><script>document.write("<INPUT onclick=window.location.reload() type=button value=\"" + sbutton.refresh + "\">");</script></TD></TR>
  <TR align=middle bgColor=#b3b3b3>
    <TH height=30><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(advroute.deslanip)</script></SPAN></FONT></TH>
    <TH height=30><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.submask)</script></SPAN></FONT></TH>
    <TH height=30><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.gateway)</script></SPAN></FONT></TH>
    <TH height=30><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(advroute.hop)</script></SPAN></FONT></TH>
    <TH height=30><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.inter_face)</script></SPAN></FONT></TH></TR>
<script language=javascript>
var table = new Array(
<% dump_route_table(""); %>
);
var i = 0;
for(;;){
	if(!table[i]){
		if(i == 0){
			document.write("<tr bgcolor=cccccc align=middle>");
			document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">wlanadv.none</td>");
			document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">wlanadv.none</td>");
			document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">wlanadv.none</td>");
                        document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">wlanadv.none</td>");
			document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">wlanadv.none</td></tr>");
		}
		break;
	}
	if(table[i+4] == "LAN")	table[i+4] = share.lanwireless;
	else if(table[i+4] == "WAN") table[i+4] = advroute.waninternet;

	document.write("<tr bgcolor=#cccccc align=middle>");
	document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">"+table[i]+"</td>");
	document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">"+table[i+1]+"</td>");
	document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">"+table[i+2]+"</td>");
        document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">"+table[i+3]+"</td>");
	document.write("<td><SPAN STYLE=\"FONT-SIZE: 10pt\">"+table[i+4]+"</td></tr>");
	i = i + 5;
}
</script>
  <TR align=middle bgColor=#cccccc>
    <TD bgcolor="#FFFFFF" height="35"></TD>
    <TD bgcolor="#FFFFFF" height="35"></TD>
    <TD bgcolor="#FFFFFF" height="35"></TD>
    <TD bgcolor="#FFFFFF" height="35"><script>document.write("<INPUT onclick=self.close() type=reset name=close value=\"" + sbutton.close + "\">");</script>

    </TD>
    </TR></TBODY></TABLE></CENTER></FORM></BODY></HTML>
