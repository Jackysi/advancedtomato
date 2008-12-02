
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

<HTML><HEAD><TITLE>Wireless Status</TITLE>
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
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>function ViewDHCP(){self.open('DHCPTable.asp', 'DHCP', 'alwaysRaised,resizable,scrollbars,width=560,height=400');}</SCRIPT>
<script>
document.title = bmenu.wireless;
</script>

</HEAD>
<BODY>
<DIV align=center>
<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95><IMG src="image/UI_Linksys.gif" border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT 
      style="FONT-SIZE: 7pt" color=#ffffff><FONT face=Arial><script>Capture(share.firmwarever)</script>:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></FONT></TD></TR>
  <TR>
    <TD width=808 bgColor=#6666cc colSpan=2><IMG height=11 src="image/UI_10.gif" width=809 border=0></TD></TR></TBODY></TABLE>
<TABLE height=77 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" borderColor=#000000 align=middle width=163 height=49>
      <H3 style="MARGIN-TOP: 1px; MARGIN-BOTTOM: 1px"><FONT 
      style="FONT-SIZE: 15pt" face=Arial color=#ffffff><script>Capture(bmenu.statu)</script></FONT></H3></TD>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" vAlign=center borderColor=#000000 width=646 bgColor=#000000 height=49>
      <TABLE style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" height=33 cellSpacing=0 cellPadding=0 bgColor=#6666cc border=0>
        <TBODY>
        <TR>
          <TD style="FONT-WEIGHT: bolder; FONT-SIZE: 10pt" align=right 
          bgColor=#6666cc height=33><FONT color=#ffffff><script>productname()</script>&nbsp;&nbsp;</FONT></TD>
          <TD borderColor=#000000 borderColorLight=#000000 align=middle 
          width=109 bgColor=#000000 borderColorDark=#000000 height=12 
            rowSpan=2><FONT color=#ffffff><SPAN style="FONT-SIZE: 8pt"><B><% get_model_name(); %></B></SPAN></FONT></TD></TR>
        <TR>
          <TD 
          style="FONT-WEIGHT: normal; FONT-SIZE: 1pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=537 bgColor=#000000 height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=646 bgColor=#000000 colSpan=2 height=32>
            <TABLE id=AutoNumber1 
            style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" 
            height=6 cellSpacing=0 cellPadding=0 width=646 border=0>
              <TBODY>
              <TR style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" align=middle bgColor=#6666cc>
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

                      <!-- TD class=small width=65 -->
                      <script>document.write("<TD class=small width=" + sta_width.w2 + ">")</script>
                      <A href="Status_Router.asp"><script>Capture(share.router)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=25></TD -->
                      <script>document.write("<TD width=" + sta_width.w3 + "></TD>")</script>

                      <!-- TD class=small width=100 -->
                      <script>document.write("<TD class=small width=" + sta_width.w4 + ">")</script>
                      <A href="Status_Lan.asp"><script>Capture(statopmenu.localnet)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=25></TD -->
                      <script>document.write("<TD width=" + sta_width.w5 + "></TD>")</script>

                      <!-- TD width=100 -->
                      <script>document.write("<TD width=" + sta_width.w6 + ">")</script>
                      <FONT style="COLOR: white"><span >&nbsp;</span><script>Capture(bmenu.wireless)</script></FONT></TD>
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
              </TR>
              </TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
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
            <P align=right><B><FONT style="FONT-SIZE: 9pt" face=Arial color=#ffffff><span><script>Capture(bmenu.wireless)</script></span></FONT></B></P></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=44 height=25>&nbsp;</TD>
          <TD width=120 height=25>&nbsp;</TD>
          <TD width=277 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25><p align="right">&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=44 height=25>&nbsp;</TD>
          <TD width=120><FONT style="FONT-SIZE: 8pt"><span><script>Capture(share.macaddr)</script>:&nbsp;</span></FONT></TD>
          <TD width=277>&nbsp;<FONT style="FONT-SIZE: 8pt"><B><% nvram_get("wl0_hwaddr"); %></B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=44 height=25>&nbsp;</TD>
          <TD width=120><FONT style="FONT-SIZE: 8pt"><span><script>Capture(stawlan.mode)</script>:&nbsp;</span></FONT></TD>
          <TD width=277><b>&nbsp;<% nvram_match("wl_net_mode", "mixed", "<script>Capture(wlansetup.mixed)</script>"); %><% nvram_match("wl_net_mode", "g-only", "<script>Capture(wlansetup.gonly)</script>"); %><% nvram_match("wl_net_mode", "disabled", "<script>Capture(share.disabled)</script>"); %><% nvram_match("wl_net_mode", "b-only", "<script>Capture(wlansetup.bonly)</script>"); %>&nbsp;</b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=44 height=25>&nbsp;</TD>
          <TD width=120 height=25><FONT style="FONT-SIZE: 8pt"><span>SSID:&nbsp;</span></FONT></TD>
          <TD width=277 height=25><b>&nbsp;<% nvram_get("wl_ssid"); %>&nbsp;</b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=44 height=25>&nbsp;</TD>
          <TD width=120 height=25><FONT style="FONT-SIZE: 8pt"><script>Capture(share.dhcpsrv)</script>:&nbsp;</FONT></TD>
          <TD width=277 height=25>&nbsp;<FONT style="FONT-SIZE: 8pt"><B><% nvram_match("lan_proto", "dhcp", "<script>Capture(share.enabled)</script>"); %><% nvram_match("lan_proto", "static", "<script>Capture(share.disabled)</script>"); %></B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=44 height=25>&nbsp;</TD>
          <TD width=120 height=25><font style="font-size: 8pt"><script>Capture(stawlan.channel)</script>:&nbsp;</font></TD>
          <TD width=277 height=25><b>&nbsp;<% nvram_get("wl_channel"); %>&nbsp;</b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=44 height=25>&nbsp;</TD>
          <TD width=120 height=25><font style="font-size: 8pt"><script>Capture(stawlan.encryfun)</script>:&nbsp;</font></TD>
          <TD width=277 height=25><b>&nbsp;<% nvram_match("security_mode", "disabled", "<script>Capture(share.disabled)</script>"); %><% nvram_invmatch("security_mode", "disabled", "<script>Capture(share.enabled)</script>"); %>&nbsp;</b>
          </TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

	<TR>
          <TD width=156 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=44>&nbsp;</TD>
          <TD width=120></TD>
          <TD width=277><p>&nbsp;</p></TD>
          <TD width=13>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD>
        </TR>

        
        </TBODY></TABLE></FORM></TD>
    <TD vAlign=top width=176 bgColor=#6666cc><font color="#FFFFFF"><span style="font-family: Arial"><br>
<script>Capture(hstatwl2.right1)</script><br><br>
<script>Capture(hstatwl2.right2)</script><br>
<b><a target="_blank" href="help/HStatus.asp#wireless"><script>Capture(share.more)</script></a></b></span></font></TD></TR>
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

<script>document.write("<INPUT onclick=window.location.replace('Status_Wireless.asp') type=button name=refresh_button value=\"" + sbutton.refresh + "\">");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>

</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
