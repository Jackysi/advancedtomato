
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

<% show_status("init"); %>
<HTML><HEAD><TITLE>Router Status</TITLE>
<% no_cache(); %>
<% charset(); %>
<link rel="stylesheet" type="text/css" href="style.css">
<style fprolloverstyle>
A:hover {color: #00FFFF}
.small A:hover {color: #00FFFF}
</style>

<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capstatus.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsetup.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
document.title = share.router;
function DHCPAction(F,I)
{
	F.submit_type.value = I;
	F.submit_button.value = "Status_Router";
	F.change_action.value = "gozila_cgi";
	F.submit();
}
function Connect(F,I)
{
	F.submit_type.value = I;
	F.submit_button.value = "Status_Router";
	F.change_action.value = "gozila_cgi";
	F.submit();
}
function init()
{
	<% show_status("onload");%>
}
function ShowAlert(M)
{
	var str = "";
	var mode = "";
	var wan_ip = "<% nvram_status_get("wan_ipaddr"); %>";
	var wan_proto = "<% nvram_get("wan_proto"); %>";
	var ac_name = "<% nvram_get("ppp_get_ac"); %>";
	var srv_name = "<% nvram_get("ppp_get_srv"); %>";

	if(document.status.wan_proto.value == "pppoe")
		mode = "PPPoE";
	else if(document.status.wan_proto.value == "l2tp")
		mode = "L2TP";
	else if(document.status.wan_proto.value == "heartbeat")
		mode = "HBS";
	else
		mode = "PPTP";

	if(M == "AUTH_FAIL" || M == "PAP_AUTH_FAIL" || M == "CHAP_AUTH_FAIL")
                str = mode + hstatrouter2.authfail;
//              str = mode + " authentication fail";
	else if(M == "IP_FAIL" || (M == "TIMEOUT" && wan_ip == "0.0.0.0")) {
		if(mode == "PPPoE") {
			if(hstatrouter2.pppoenoip)	// For DE
				str = hstatrouter2.pppoenoip;
			else
				str = hstatrouter2.noip + mode + hstatrouter2.server;
		}
		else
                	str = hstatrouter2.noip + mode + hstatrouter2.server;
	}
//              str = "Can not get an IP address from " + mode + " server";
        else if(M == "NEG_FAIL")
                str = mode + hstatrouter2.negfail;
//              str = mode + " negotication fail";
        else if(M == "LCP_FAIL")
                str = mode + hstatrouter2.lcpfail;
//              str = mode + " LCP negotication fail";
        else if(M == "TCP_FAIL" || (M == "TIMEOUT" && wan_ip != "0.0.0.0" && wan_proto == "heartbeat"))
                str = hstatrouter2.tcpfail + mode + hstatrouter2.server;
//              str = "Can not build a TCP connection to " + mode + " server";
	else 
                str = hstatrouter2.noconn + mode + hstatrouter2.server;
//              str = "Can not connect to " + mode + " server";

	alert(str);

	Refresh();
}
var value=0;
function Refresh()
{
	var refresh_time = "<% show_status("refresh_time"); %>";
	if(refresh_time == "")	refresh_time = 60000;
	if (value>=1)
	{
		window.location.replace("Status_Router.asp");
	}
	value++;
	timerID=setTimeout("Refresh()",refresh_time);
}
function ViewDHCP()
{
	dhcp_win = self.open('DHCPTable.asp','inLogTable','alwaysRaised,resizable,scrollbars,width=720,height=600');
	dhcp_win.focus();
}
function localtime()
{
        tmp = "<% localtime(); %>";
        if( tmp == "Not Available")
                document.write(satusroute.localtime);
        else
                document.write(tmp);
}
</SCRIPT>

<BODY onload=init()>
<DIV align=center>
<FORM name=status method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=submit_type>
<input type=hidden name=change_action>
<input type=hidden name=wan_proto value='<% nvram_get("wan_proto"); %>'>

<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95><IMG src="image/UI_Linksys.gif" border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT 
      style="FONT-SIZE: 7pt" color=#ffffff><FONT face=Arial><script>Capture(share.firmwarever)</script>:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></FONT></TD></TR>
  <TR>
    <TD width=808 bgColor=#6666cc colSpan=2><IMG height=11 
      src="image/UI_10.gif" width=809 
border=0></TD></TR></TBODY></TABLE>
<TABLE height=77 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#000000 align=middle width=163 height=49>
      <H3 style="MARGIN-TOP: 1px; MARGIN-BOTTOM: 1px"><FONT 
      style="FONT-SIZE: 15pt" face=Arial color=#ffffff><script>Capture(bmenu.statu)</script></FONT></H3></TD>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    vAlign=center borderColor=#000000 width=646 bgColor=#000000 height=49>
      <TABLE 
      style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" 
      height=33 cellSpacing=0 cellPadding=0 bgColor=#6666cc border=0>
        <TBODY>
        <TR>
          <TD style="FONT-WEIGHT: bolder; FONT-SIZE: 10pt" align=right 
          bgColor=#6666cc height=33><FONT color=#ffffff><script>productname()</script>&nbsp;&nbsp;</FONT></TD>
          <TD borderColor=#000000 borderColorLight=#000000 align=middle 
          width=109 bgColor=#000000 borderColorDark=#000000 height=12 
            rowSpan=2><FONT color=#ffffff><SPAN 
            style="FONT-SIZE: 8pt"><B><% get_model_name(); %></B></SPAN></FONT></TD></TR>
        <TR>
          <TD 
          style="FONT-WEIGHT: normal; FONT-SIZE: 1pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
          width=537 bgColor=#000000 height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=646 bgColor=#000000 colSpan=2 height=32>
            <TABLE id=AutoNumber1 
            style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" 
            height=6 cellSpacing=0 cellPadding=0 width=646 border=0>
              <TBODY>
              <TR 
              style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" align=middle bgColor=#6666cc>

<!--
                <TD width=83 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=73 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=113 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=103 height=1><IMG height=10 src="image/UI_06.gif" width=103 border=0></TD>
                <TD width=85 height=1><IMG height=10 src="image/UI_06.gif" width=100 border=0></TD>
                <TD width=115 height=1><IMG height=10 src="image/UI_06.gif" width=115 border=0></TD>
                <TD width=74 height=1><IMG height=10 src="image/UI_07.gif" width=79 border=0></TD>
-->
                <script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_07.gif></TD>")</script>

              </TR>
              <TR>
                <TD align=middle bgColor=#000000 height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff><A 
                  style="TEXT-DECORATION: none" 
                  href="index.asp"><script>Capture(bmenu.setup)</script></A></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="Wireless_Basic.asp"><script>Capture(bmenu.wireless)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="Firewall.asp"><script>Capture(bmenu.security)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="<% support_elsematch("PARENTAL_CONTROL_SUPPORT", "1", "Parental_Control.asp", "Filters.asp"); %>"><script>Capture(bmenu.accrestriction)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" 
                  color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Forward.asp"><script>Capture(bmenu.applications)</script> <BR>&amp; <script>Capture(bmenu.gaming)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" 
                  color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Management.asp"><script>Capture(bmenu.admin)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#6666cc height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" 
                  color=#ffffff><script>Capture(bmenu.statu)</script>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
              </TR>
              <TR>
                <TD width=643 bgColor=#6666cc colSpan=7 height=21>
                  <TABLE borderColor=black height=21 cellSpacing=0 cellPadding=0 width=643>
                    <TBODY>
                    <TR align=left>

                      <!-- TD width=25></TD -->
                      <script>document.write("<TD width=" + sta_width.w1 + "></TD>")</script>  

                      <!-- TD width=65 -->
                      <script>document.write("<TD width=" + sta_width.w2 + ">")</script>  
                      <FONT style="COLOR: white"><script>Capture(share.router)</script></FONT></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=25></TD -->
                      <script>document.write("<TD width=" + sta_width.w3 + "></TD>")</script>  

                      <!-- TD class=small width=100 -->
                      <script>document.write("<TD class=small width=" + sta_width.w4 + ">")</script>  
                      <A href="Status_Lan.asp"><script>Capture(statopmenu.localnet)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=25></TD -->
                      <script>document.write("<TD width=" + sta_width.w5 + "></TD>")</script>  

                      <!-- TD class=small width=100 -->
                      <script>document.write("<TD class=small width=" + sta_width.w6 + ">")</script>  
                      <span >&nbsp;</span><A href="Status_Wireless.asp"><script>Capture(bmenu.wireless)</script></A></TD>
<% support_invmatch("PERFORMANCE_SUPPORT", "1", "<!--"); %>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <script>document.write("<TD width=" + sta_width.w7 + "></TD>")</script>  

                      <script>document.write("<TD class=small width=" + sta_width.w8 + ">")</script>  
                      <A href="Status_Performance.asp">System Performance</A></TD>
<% support_invmatch("PERFORMANCE_SUPPORT", "1", "-->"); %>
                      <TD>&nbsp;</TD>
		    </TR>
                    </TBODY>
                  </TABLE>
                </TD>
              </TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1><IMG height=15 
      src="image/UI_03.gif" width=164 border=0></TD>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    width=646 height=1><IMG height=15 src="image/UI_02.gif" 
      width=645 border=0></TD></TR></TBODY></TABLE>
<TABLE id=AutoNumber9 style="BORDER-COLLAPSE: collapse" borderColor=#111111 
height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE height=100% cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD width=156 bgColor=#000000 height=25>
            <P align=right><B><FONT style="FONT-SIZE: 9pt" face=Arial 
            color=#ffffff><script>Capture(staleftmenu.routerinfo)</script></B></P></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD width=17 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><script>Capture(share.firmwarever)</script>:&nbsp;</TD>
          <TD><B><% get_firmware_version(); %>, <% compile_date(); %></B></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><script>Capture(stacontent.curtime)</script>:&nbsp;</TD>
          <!-- TD><b><% localtime(); %></b></TD -->
          <TD><b><script>localtime();</script></b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><script>Capture(share.macaddr)</script>:&nbsp;</TD>
          <TD><b><% nvram_get("wan_hwaddr"); %></b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><script>Capture(share.routename)</script>:&nbsp;</TD>
          <TD><b><% nvram_get("router_name"); %>&nbsp;</b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><script>Capture(share.hostname)</script>:&nbsp;</TD>
          <TD><b><% nvram_get("wan_hostname"); %>&nbsp;</b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><script>Capture(share.domainname)</script>:&nbsp;</TD>
          <TD><b>
<script language=javascript>
if("<% nvram_get("wan_domain"); %>" != "") {
	document.write("<% nvram_get_len("wan_domain", "44"); %>");
}
else {
	document.write("<% nvram_get_len("wan_get_domain", "44"); %>");
}
</script>
</b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#000000 height=25>
            <P align=right><B><FONT style="FONT-SIZE: 9pt" 
            color=#ffffff><span ><script>Capture(share.internet)</script></span></FONT></B></P></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD colSpan=6>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
          height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>
          <p align="right"><FONT 
style='FONT-WEIGHT: 700'><span ><script>Capture(share.cfgtype)</script></span></FONT></TD>
          <TD width=8 background=image/UI_04.gif 
          height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><span><script>Capture(stacontent.logtype)</script></span>:&nbsp;</TD>
          <TD><b><% nvram_match("wan_proto","dhcp","<script>Capture(setupcontent.dhcp)</script>"); %><% nvram_match("wan_proto","static","<script>Capture(hstatrouter2.wan_static)</script>"); %><%
nvram_match("wan_proto","pppoe","<script>Capture(share.pppoe)</script>"); %><% nvram_match("wan_proto","pptp","<script>Capture(share.pptp)</script>"); %><% nvram_match("wan_proto","l2tp","<script>Capture(hstatrouter2.l2tp)</script>"); %><%
nvram_match("wan_proto","heartbeat","<script>Capture(hstatrouter2.hb)</script>"); %></b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% show_status_setting(); %>

        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
	  <TD width=14 height=25></TD>
          <TD colSpan=4 height=25><HR color=#b5b5e6 SIZE=1></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>                

        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD colSpan=2 height=25>

<!-- % nvram_match("wan_proto", "dhcp", "<INPUT onclick=DHCPAction(this.form,'release') type=button value='DHCP Release'>&nbsp;&nbsp;&nbsp;&nbsp;<INPUT onclick=DHCPAction(this.form,'renew') type=button value='DHCP Renew'>"); % -->

<% nvram_invmatch("wan_proto", "dhcp", "<!--"); %> 

<script>document.write("<INPUT onclick=DHCPAction(this.form,\'release\') type=button name=dhcp_release value=\"" + stabutton.dhcprelease + "\">");</script>

<script>document.write("<INPUT onclick=DHCPAction(this.form,\'renew\') type=button name=dhcp_renew value=\"" + stabutton.dhcprenew + "\">");</script>

<% nvram_invmatch("wan_proto", "dhcp", "-->"); %> 

    &nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>  
        <TR>
          <TD width=156 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD colSpan=6>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR></TBODY></TABLE></TD>

    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25>&nbsp;</TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><span ><br>
<script>Capture(hstatrouter2.right1)</script><br><br>
<script>Capture(hstatrouter2.right2)</script><br><br>
<script>Capture(hstatrouter2.right3)</script><br><br>
<script>Capture(hstatrouter2.right4)</script><br>
<b><a target="_blank" href="help/HStatus.asp"><script>Capture(share.more)</script></a></b></span><br><br>
<script>Capture(hstatrouter2.right5)</script><br>
<b><a target="_blank" href="help/HStatus.asp"><script>Capture(share.more)</script></a></b></span></font></TD>
          <TD width=9 bgColor=#6666cc 
  height=25>&nbsp;</TD></TR></TBODY></TABLE></TD></TR>
  <TR>
    <TD width=809 colSpan=2>
      <TABLE cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=30>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=454>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD>
          <TD width=176 bgColor=#6666cc rowSpan=2>
          <IMG src="image/UI_Cisco.gif" border=0 width="176" height="64"></TD></TR>
        <TR>
          <TD width=156 bgColor=#000000>&nbsp;</TD>
          <TD width=8 bgColor=#000000>&nbsp;</TD>
          <TD width=454 bgColor=#6666cc align="right">

<!-- INPUT onclick="window.location.replace('Status_Router.asp')" type=button name="refresh_button" -->
<script>document.write("<INPUT onclick=window.location.replace('Status_Router.asp') type=button name=refresh_button value=\"" + sbutton.refresh + "\">");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>
</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
