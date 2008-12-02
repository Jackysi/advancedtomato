
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

<HTML><HEAD><TITLE>Wireless Client MAC List</TITLE>
<% no_cache(); %>
<% charset(); %>
<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capapp.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=javascript>
document.title = wlanbutton.climacls;
function MACAct(F)
{
	if(valid_value(F)){
		F.submit_button.value="WL_ActiveTable";
		F.change_action.value="gozila_cgi";
		F.submit_type.value="add_mac";
		F.submit();
	}
}
function valid_value(F)
{	
	var num = F.elements.length;
	var count = 0;

	for(i=0;i<num;i++){
		if(F.elements[i].type == "checkbox"){
			if(F.elements[i].checked == true)
				count = count + 1;
		}
	}
	if(count > 40){
//              alert("The total checks exceed 40 counts!");
                alert(errmsg2.err9);
		return false;
	}
	return true;
}
function init()
{
	<% onload("WL_ActiveTable", "setTimeout('opener.window.location.reload();',500);"); %>
	window.focus();
}
</SCRIPT>
</HEAD>
<BODY bgColor=white onload=init()>
<form method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=submit_type>
<input type=hidden name=change_action>
<CENTER>
<TABLE height=9 cellSpacing=1 width=606 border=0>
  <TBODY>
  <TR>
    <TD width=310 colSpan=2 height=30><b>
    <font face="Arial" color="#0000FF"><SPAN STYLE="FONT-SIZE=14pt">
    <script>Capture(wlanbutton.climacls)</script></SPAN></font></b></TD>
    <TD align=right width=156 height=30>&nbsp;</TD>
    <TD align=center width=141 height=30>

<script>document.write("<INPUT onclick=window.location.reload() type=button name=refresh value=\"" + sbutton.refresh + "\">");</script>

    </TD></TR>
  <TR align=middle bgColor=#b3b3b3>
    <TH height=19 width="167" bgcolor="#FFFFFF" valign="bottom">
    <font face="Arial" color="#0000FF"><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(wlanmaclist.actpc)</script></font></TH>
    <TH height=19 width="140" bgcolor="#FFFFFF" valign="bottom">&nbsp;</TH>
    <TH height=19 width="156" bgcolor="#FFFFFF" valign="bottom">&nbsp;</TH>
    <TH height=19 width="141" bgcolor="#FFFFFF" valign="bottom">&nbsp;</TH></TR>
  <TR align=middle bgColor=#b3b3b3>
    <TH height=30 width="167"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.clihostname)</script></FONT></TH>
    <TH height=30 width="140"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.ipaddr)</script></FONT></TH>
    <TH height=30 width="156"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.macaddr)</script></FONT></TH>
    <TH height=30 width="141"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(wlanmaclist.enablefilter)</script></FONT></TH></TR>
<% wireless_active_table("online"); %>
  <TR align=middle bgColor=#cccccc>
    <TD height="19" width="167" bgcolor="#FFFFFF" valign="bottom"><b>
    <font face="Arial" color="#0000FF"><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(wlanmaclist.inactpc)</script></font></b></TD>
    <TD height="19" width="140" bgcolor="#FFFFFF" valign="bottom">&nbsp;</TD>
    <TD height="28" width="156" bgcolor="#FFFFFF" valign="bottom">&nbsp;</TD>
    <TD height="28" width="141" bgcolor="#FFFFFF" valign="bottom"> 
    &nbsp;</TD></TR>
  <tr>
    <TH height=30 width="167" bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.clihostname)</script></FONT></TH>
    <TH height=30 width="140" bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.ipaddr)</script></FONT></TH>
    <TH height=30 width="156" bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(share.macaddr)</script></FONT></TH>
    <TH height=30 width="141" bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(wlanmaclist.enablefilter)</script></FONT></TH>
  </tr>
<% wireless_active_table("offline"); %>
  <TR align=middle bgColor=#cccccc>
    <TD height="10" width="167" bgcolor="#FFFFFF">&nbsp;</TD>
    <TD height="10" width="140" bgcolor="#FFFFFF">&nbsp;</TD>
    <TD height="19" width="297" bgcolor="#FFFFFF" colspan="2" align="right" valign="bottom">&nbsp;</TD>
    </TR>
  <TR align=middle bgColor=#cccccc>
    <TD height="24" width="167" bgcolor="#FFFFFF">&nbsp;</TD>
    <!-- TD height="24" width="140" bgcolor="#FFFFFF">&nbsp;</TD -->
    <!-- TD height="33" width="297" bgcolor="#FFFFFF" colspan="2" align="right" valign="bottom"><font face=verdana size=2 -->
    <TD height="33" width="437" bgcolor="#FFFFFF" colspan="3" align="right" valign="bottom"><font face=verdana size=2>

<script>document.write("<input type=button name=action onClick=MACAct(this.form) value=\"" + wlanbutton.updatels + "\">");</script>

&nbsp;
    </font> 
    
<script>document.write("<INPUT onclick=self.close() type=reset name=close value=\"" + sbutton.close + "\">");</script>


    </TD>
    </TR></TBODY></TABLE></CENTER>
</BODY></HTML>
