
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

<HTML><HEAD><TITLE>Wireless MAC Filter</TITLE>
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capapp.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
document.title = wlantopmenu.macfilter;
var win_options = 'alwaysRaised,resizable,scrollbars,width=660,height=460' ;

var wl_filter_win = null;
var EN_DIS =  '<% nvram_get("wl_macmode"); %>'
function closeWin(win_var)
{
        if ( ((win_var != null) && (win_var.close)) || ((win_var != null) && (win_var.closed==false)) )
                win_var.close();
}

function ViewFilter()
{
	wl_filter_win = self.open('WL_FilterTable.asp','FilterTable','alwaysRaised,resizable,scrollbars,width=520,height=530');
	wl_filter_win.focus();
}
function to_submit(F)
{
        F.submit_button.value = "Wireless_MAC";
        F.action.value = "Apply";
        F.submit();
}

function SelMac(num,F)
{
        F.submit_button.value = "Wireless_MAC";
        F.change_action.value = "gozila_cgi";
        F.wl_macmode1.value = F.wl_macmode1.value;
        F.submit();
}
function exit()
{
        closeWin(wl_filter_win);
}

</SCRIPT>

</HEAD>
<BODY onunload=exit()>
<DIV align=center>
<FORM method=<% get_http_method(); %> name=wireless action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=action>
<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95><IMG src="image/UI_Linksys.gif" 
      border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT 
      style="FONT-SIZE: 7pt" color=#ffffff><FONT face=Arial><script>Capture(share.firmwarever)</script>:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></FONT></TD></TR>
  <TR>
    <TD width=808 bgColor=#6666cc colSpan=2><IMG height=11 
      src="image/UI_10.gif" width=809 border=0></TD></TR></TBODY></TABLE>
<TABLE height=77 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#000000 align=middle width=163 height=49>
      <H3 style="MARGIN-TOP: 1px; MARGIN-BOTTOM: 1px"><FONT 
      style="FONT-SIZE: 15pt" face=Arial color=#ffffff><script>Capture(bmenu.wireless)</script></FONT></H3></TD>
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
              <TR style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" align=middle bgColor=#6666cc>

<!--
                <TD width=83 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=73 height=1><IMG height=10 src="image/UI_07.gif" width=83 border=0></TD>
                <TD width=113 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=103 height=1><IMG height=10 src="image/UI_06.gif" width=103 border=0></TD>
                <TD width=85 height=1><IMG height=10 src="image/UI_06.gif" width=100 border=0></TD>
                <TD width=115 height=1><IMG height=10 src="image/UI_06.gif" width=115 border=0></TD>
                <TD width=74 height=1><IMG height=10 src="image/UI_06.gif" width=79 border=0></TD>
-->
                <script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_07.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_06.gif></TD>")</script>

              </TR>
              <TR>
                <TD align=middle bgColor=#000000 height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff><A 
                  style="TEXT-DECORATION: none" 
                  href="index.asp"><script>Capture(bmenu.setup)</script></A></FONT></TD>
                <TD align=middle bgColor=#6666cc height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff><script>Capture(bmenu.wireless)</script></FONT></TD>
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
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" 
                  color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Status_Router.asp"><script>Capture(bmenu.statu)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
              </TR>
              <TR>
                <TD width=643 bgColor=#6666cc colSpan=7 height=21>
                  <TABLE borderColor=black height=21 cellSpacing=0 cellPadding=0 width=643>
                    <TBODY>
                    <TR align=left>

		      <!-- TD width=40></TD -->
                      <script>document.write("<TD width=" + wl_width.w1 + "></TD>")</script> 

                      <!-- TD class=small width=210 -->
                      <script>document.write("<TD class=small width=" + wl_width.w2 + ">")</script> 
                      <a href="Wireless_Basic.asp"><script>Capture(wlantopmenu.basicset)</script></a></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=20></TD -->
                      <script>document.write("<TD width=" + wl_width.w3 + "></TD>")</script> 

                      <!-- TD class=small width=165 -->
                      <script>document.write("<TD class=small width=" + wl_width.w4 + ">")</script>
                      <a href="WL_WPATable.asp">&nbsp;<script>Capture(wlantopmenu.security)</script></a></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=20></TD -->
                      <script>document.write("<TD width=" + wl_width.w5 + "></TD>")</script> 

                      <!-- TD class=small width=165 -->
                      <script>document.write("<TD class=small width=" + wl_width.w6 + ">")</script>
                      <FONT style="COLOR: white">&nbsp;<script>Capture(wlantopmenu.macfilter)</script></FONT></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=22></TD -->
                      <script>document.write("<TD width=" + wl_width.w7 + "></TD>")</script>

                      <!-- TD class=small width=260 -->
                      <script>document.write("<TD class=small width=" + wl_width.w8 + ">")</script> 
                      <a href="Wireless_Advanced.asp"><script>Capture(wlantopmenu.advwireless)</script></a></TD>

                    </TR>
                    </TBODY>
                  </TABLE>
                </TD>
              </TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>
  <TBODY>
  <TR>
    <TD borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1><IMG height=15 
      src="image/UI_03.gif" width=164 border=0></TD>
    <TD width=646 height=1><IMG height=15 src="image/UI_02.gif" width=645 border=0></TD></TR></TBODY></TABLE>
<TABLE id=AutoNumber9 style="BORDER-COLLAPSE: collapse" borderColor=#111111 
height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE cellSpacing=0 cellPadding=0 border=0 width="633">
        <TBODY>
        <TR>
          <TD width=156 bgColor=#000000 height=25>
            <P align=right><B>
                <font face="Arial" color="#FFFFFF" style="font-size: 9pt">
                <script>Capture(wlantopmenu.macfilter)</script></font></B></P></TD>
          <TD width=8 bgColor=black height=25>&nbsp;</TD>
          <TD width=21>&nbsp;</TD>
          <TD width=116>&nbsp;</TD>
          <TD width=317>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR>
         <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD height=25 width="21">&nbsp;</TD>
          <TD width=116 height=25>&nbsp;<font face="Arial" style="font-size: 8pt"><script>Capture(wlantopmenu.macfilter)</script>:&nbsp;</font></TD>
          <TD width=317 height=25>
                <table border="1" cellpadding="0" cellspacing="0" style="border-collapse: collapse; border-width: 0" bordercolor="#111111" width="200">
                  <tr>
                    <td style="border-style: none; border-width: medium" height="25" width="200">
         &nbsp;&nbsp;<INPUT onClick=SelMac('other',this.form) type=radio value=other name=wl_macmode1 <% nvram_selmatch("wl_macmode1","other","checked"); %>><B><script>Capture(share.enable)</script></B>&nbsp; 
	<INPUT onClick=SelMac('disabled',this.form) type=radio value=disabled name=wl_macmode1 <% nvram_selmatch("wl_macmode1","disabled","checked"); %>><B><script>Capture(share.disable)</script></B> </td>
                  </tr>
                </table>
          </TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% nvram_selmatch("wl_macmode1","disabled","<!--"); %>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD height=25 width="21">&nbsp;</TD>
          <TD width=116 height=25 valign=top>&nbsp;<script>Capture(wlanfilter.prevent)</script>:</TD>
          <TD width=317 height=25>
	  <TABLE height=30 cellSpacing=0 cellPadding=0 border=0>
              <TBODY>
              <tr>
                <td heigh=30 valign=baseline>
                        &nbsp;&nbsp;<INPUT type=radio value="deny" name="wl_macmode" <% nvram_invmatch("wl_macmode","allow","checked"); %>>
                </td>
                <td heigh=30 valign=baseline>
                        <b><script>Capture(wlanfilter.prevent)</script></b> <script>Capture(wlanfilter.pclist)</script>
                </td>
              </tr>
              </TBODY>
          </TABLE>
          </TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD height=25 width="21">&nbsp;</TD>
          <TD width=116 height=25 valign=top>&nbsp;<script>Capture(wlanfilter.permitonly)</script>:</TD>
          <TD width=317 height=25>
	  <TABLE height=30 cellSpacing=0 cellPadding=0 border=0>
              <TBODY>
              <tr>
                <td heigh=30 valign=baseline>
                        &nbsp;&nbsp;<INPUT type=radio value="allow" name="wl_macmode" <% nvram_match("wl_macmode","allow","checked"); %>>
                </td>
                <td heigh=30 valign=baseline>
                        <b><script>Capture(wlanfilter.permitonly)</script></b> <script>Capture(wlanfilter.pclisttoacc)</script>
                </td>
              </tr>
              </TBODY>
          </TABLE>
          </TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD height=25 width="21">&nbsp;</TD>
          <TD width=116 height=25>&nbsp;</TD>
          <TD width=317 height=25>&nbsp;<B><INPUT type=hidden value=0 name=login_status></B></TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=40>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD height=25 width="21">&nbsp;</TD>
          <TD width=432 height=25 colspan=2>&nbsp;<B><INPUT type=hidden value=0 name=login_status>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;

<script>document.write("<INPUT type=button onclick=ViewFilter() name=mac_filter_button value=\"" + wlanbutton.editlist + "\">");</script>

            </B></TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% nvram_selmatch("wl_macmode1","disabled","-->"); %>                                                      


	<TR>
          <TD bgColor=#e7e7e7></TD>
          <TD background=image/UI_04.gif></TD>
          <TD></TD>
          <TD></TD>
          <TD></TD>
          <TD background=image/UI_05.gif></TD>
        </TR>



        </TBODY></TABLE></TD>
    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25>&nbsp;</TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><span style="font-family: Arial">
<b><a target="_blank" href="help/HWireless.asp#wl_mac_filters"><% nvram_else_match("language","DE","Hilfe...","<script>Capture(share.more)</script>"); %></a></b></span></font></TD>
          <TD width=9 bgColor=#6666cc 
  height=25>&nbsp;</TD></TR></TBODY></TABLE></TD></TR>
  <TR>
    <TD width=809 colSpan=2>
      <TABLE cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD width=154 bgColor=#e7e7e7 height=30>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=454>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD>
          <TD width=176 bgColor=#6666cc rowSpan=2>
          <IMG src="image/UI_Cisco.gif" border=0 width="176" height="64"></TD></TR>
        <TR>
          <TD width=156 bgColor=#000000>&nbsp;</TD>
          <TD width=8 bgColor=#000000>&nbsp;</TD>
          <TD width=454 bgColor=#6666cc align=right>

<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;

<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"Wireless_MAC.asp\")>");</script>&nbsp;&nbsp;


          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>

</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
