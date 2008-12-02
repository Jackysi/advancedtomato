
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

<HTML><HEAD><TITLE>Advanced Wireless Settings</TITLE>
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/ses.js"></SCRIPT>

<SCRIPT language=JavaScript>
re1 = /<br>/gi;
str = wlantopmenu.advwireless.replace(re1, " ");
document.title = str;
var win_options = 'alwaysRaised,resizable,scrollbars,width=660,height=460' ;

var wl_filter_win = null;
var EN_DIS =  '<% nvram_get("wl_macmode"); %>'

function to_submit(F)
{
        F.submit_button.value = "Wireless_Advanced";
        F.action.value = "Apply";
        F.submit();
}
function SelWME(num,F)
{
	wme_enable_disable(F,num);
}
function wme_enable_disable(F,I)
{
	var start = '';
	var end = '';
	var total = F.elements.length;
	for(i=0 ; i < total ; i++){
                if(F.elements[i].name == "wl_wme_no_ack")  start = i;
                if(F.elements[i].name == "wl_wme_sta_vo5")  end = i;
        }
        if(start == '' || end == '')    return true;

	if( I == "0" || I == "off") {
		EN_DIS = 0;
		for(i = start; i<=end ;i++)
                        choose_disable(F.elements[i]);
	}
	else {
		EN_DIS = 1;
                for(i = start; i<=end ;i++)
                        choose_enable(F.elements[i]);
	}
}
function auth_enable_disable(F,I)
{
	if(I == "wep" || I == "radius" || I == "disabled") 
                        choose_enable(F.wl_auth);
	else
                        choose_disable(F.wl_auth);
}
function init()
{
	wme_enable_disable(document.wireless, '<% nvram_get("wl_wme"); %>');
	auth_enable_disable(document.wireless, '<% nvram_get("security_mode"); %>');
}

</SCRIPT>

</HEAD>
<BODY onload=init()>
<DIV align=center>
<FORM method=<% get_http_method(); %> name=wireless action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=action>
<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95><IMG src="image/UI_Linksys.gif" border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT style="FONT-SIZE: 7pt" color=#ffffff><FONT face=Arial><script>Capture(share.firmwarever)</script>:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></FONT></TD></TR>
  <TR>
    <TD width=808 bgColor=#6666cc colSpan=2><IMG height=11 src="image/UI_10.gif" width=809 border=0></TD></TR></TBODY></TABLE>
<TABLE height=77 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#000000 align=middle width=163 height=49>
      <H3 style="MARGIN-TOP: 1px; MARGIN-BOTTOM: 1px">
      <font color="#FFFFFF" face="Arial" style="font-size: 15pt"><script>Capture(bmenu.wireless)</script></font></H3></TD>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    vAlign=center borderColor=#000000 width=646 bgColor=#000000 height=49>
      <TABLE style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" 
      height=33 cellSpacing=0 cellPadding=0 bgColor=#6666cc border=0>
        <TBODY>
        <TR>
          <TD style="FONT-WEIGHT: bolder; FONT-SIZE: 10pt" align=right bgColor=#6666cc height=33><FONT color=#ffffff><script>productname()</script>&nbsp;&nbsp;</FONT></TD>
          <TD borderColor=#000000 borderColorLight=#000000 align=middle width=109 bgColor=#000000 borderColorDark=#000000 height=12 rowSpan=2><FONT color=#ffffff><SPAN style="FONT-SIZE: 8pt"><B><% get_model_name(); %></B></SPAN></FONT></TD></TR>
        <TR>
          <TD style="FONT-WEIGHT: normal; FONT-SIZE: 1pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=537 bgColor=#000000 height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=646 bgColor=#000000 colSpan=2 height=32>
            <TABLE id=AutoNumber1 style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" height=6 cellSpacing=0 cellPadding=0 width=646 border=0><TBODY>
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
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><A style="TEXT-DECORATION: none" href="index.asp"><script>Capture(bmenu.setup)</script></A></FONT></TD>
                <TD align=middle bgColor=#6666cc height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><script>Capture(bmenu.wireless)</script></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Firewall.asp"><script>Capture(bmenu.security)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="<% support_elsematch("PARENTAL_CONTROL_SUPPORT", "1", "Parental_Control.asp", "Filters.asp"); %>"><script>Capture(bmenu.accrestriction)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Forward.asp"><script>Capture(bmenu.applications)</script> <BR>&amp; <script>Capture(bmenu.gaming)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Management.asp"><script>Capture(bmenu.admin)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Status_Router.asp"><script>Capture(bmenu.statu)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
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

                      <TD width=1><P class=bar><font color='white'><b>|</b></font></P></TD>

		      <!-- TD width=20></TD -->
                      <script>document.write("<TD width=" + wl_width.w3 + "></TD>")</script>	

		      <!-- TD class=small width=165 -->
                      <script>document.write("<TD class=small width=" + wl_width.w4 + ">")</script>
                      <a href="WL_WPATable.asp">&nbsp;<script>Capture(wlantopmenu.security)</script></a></TD>

                      <TD width=1><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=20></TD -->
                      <script>document.write("<TD width=" + wl_width.w5 + "></TD>")</script>

                      <!-- TD class=small width=165 -->
                      <script>document.write("<TD class=small width=" + wl_width.w6 + ">")</script>
                      <a href="Wireless_MAC.asp">&nbsp;<script>Capture(wlantopmenu.macfilter)</script></a></TD>

                      <TD width=1><P class=bar><font color='white'><b>|</b></font></P></TD>

		      <!-- TD width=22></TD -->
                      <script>document.write("<TD width=" + wl_width.w7 + "></TD>")</script>

		      <!-- TD class=small width=260 -->
                      <script>document.write("<TD class=small width=" + wl_width.w8 + ">")</script>
                      <FONT style="COLOR: white"><script>Capture(wlantopmenu.advwireless)</script></FONT></TD>

                    </TR>
                    </TBODY>
                  </TABLE>
                </TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1><IMG height=15 src="image/UI_03.gif" width=164 border=0></TD>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=646 height=1><IMG height=15 src="image/UI_02.gif" width=645 border=0></TD></TR></TBODY></TABLE>
<TABLE id=AutoNumber9 style="BORDER-COLLAPSE: collapse" borderColor=#111111 height=23 cellSpacing=0 cellPadding=0 width=809 border=0><TBODY>
  <TR>
    <TD width=633>
      <TABLE cellSpacing=0 cellPadding=0 border=0>
        <TBODY>

        <TR>
          <TD width=156 bgColor=#000000 height=25>
		<P align=right><b><font face="Arial" color="#FFFFFF" style="font-size: 9pt"><script>Capture(wlanleftmenu.advwireless)</script></font></b></P>
          </TD>
          <TD width=8 height=25 bgColor=#000000>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(hwlad2.authtyp)</script>:&nbsp;</TD>
          <TD width=296 height=25><SELECT name="wl_auth"> 
    <option value="0" <% nvram_selmatch("wl_auth", "0", "selected"); %>><script>Capture(share.auto)</script></option>
    <option value="1" <% nvram_selmatch("wl_auth", "1", "selected"); %>><script>Capture(wlansec.sharekey)</script></option>
    </SELECT>&nbsp;&nbsp;<script>Capture(wlanadv.deftransrate)</script></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(hwlad2.basrate)</script>:&nbsp;</TD>
          <TD width=296 height=25><SELECT  name="wl_rateset"> 
     		 <OPTION value="12" <% nvram_selmatch("wl_rateset", "12", "selected"); %>>1-2 Mbps&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</OPTION> 
      		 <OPTION value="default" <% nvram_selmatch("wl_rateset", "default", "selected"); %>><script>Capture(hwlad2.def)</script></OPTION>  
        	 <OPTION value="all" <% nvram_selmatch("wl_rateset", "all", "selected"); %>><script>Capture(hwlad2.all)</script></OPTION>
    		</SELECT>&nbsp;&nbsp;<script>Capture(hwlad2.defdef)</script></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(wlanadv.transrate)</script>:&nbsp; </font></TD>
          <TD width=296 height=25>
                
      <SELECT name="wl_rate"> 
      	<OPTION value="1000000" <% nvram_selmatch("wl_rate", "1000000", "selected"); %>>1 Mbps</OPTION> 
	<OPTION value="2000000" <% nvram_selmatch("wl_rate", "2000000", "selected"); %>>2 Mbps</OPTION> 
	<OPTION value="5500000" <% nvram_selmatch("wl_rate", "5500000", "selected"); %>><% nvram_else_match("language","DE","5,5", "5.5"); %> Mbps</OPTION> 
        <OPTION value="6000000" <% nvram_selmatch("wl_rate", "6000000", "selected"); %>>6 Mbps</OPTION>
        <OPTION value="9000000" <% nvram_selmatch("wl_rate", "9000000", "selected"); %>>9 Mbps</OPTION>
        <OPTION value="11000000" <% nvram_selmatch("wl_rate", "11000000", "selected"); %>>11 Mbps</OPTION>
        <OPTION value="12000000" <% nvram_selmatch("wl_rate", "12000000", "selected"); %>>12 Mbps</OPTION>
        <OPTION value="18000000" <% nvram_selmatch("wl_rate", "18000000", "selected"); %>>18 Mbps</OPTION>
        <OPTION value="24000000" <% nvram_selmatch("wl_rate", "24000000", "selected"); %>>24 Mbps</OPTION>
        <OPTION value="36000000" <% nvram_selmatch("wl_rate", "36000000", "selected"); %>>36 Mbps</OPTION>
        <OPTION value="48000000" <% nvram_selmatch("wl_rate", "48000000", "selected"); %>>48 Mbps</OPTION>
        <OPTION value="54000000" <% nvram_selmatch("wl_rate", "54000000", "selected"); %>>54 Mbps</OPTION>
        <OPTION value="0" <% nvram_selmatch("wl_rate", "0", "selected"); %>><script>Capture(share.auto)</script>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</OPTION>
    </SELECT>&nbsp;&nbsp;<script>Capture(wlanadv.deftransrate)</script></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
         <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(wlanadv.protectmode)</script>:&nbsp;</TD>
          <TD width=296 height=25>
          <SELECT name="wl_gmode_protection"> 
    <option value="off" <% nvram_selmatch("wl_gmode_protection", "off", "selected"); %>><script>Capture(share.disable)</script>&nbsp;&nbsp;&nbsp;</option>
    <option value="auto" <% nvram_selmatch("wl_gmode_protection", "auto", "selected"); %>><script>Capture(share.auto)</script></option>
    </SELECT>&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;<% support_elsematch("SPEED_BOOSTER_SUPPORT", "1", "<script>Capture(share.disable)</script>", "<script>Capture(share.disable)</script>"); %>)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>   
         <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><span ><script>Capture(hwlad2.fburst)</script></span>:&nbsp;</span></TD>
          <TD width=296 height=25>
          <SELECT name="wl_frameburst"> 
    <option value="off" <% nvram_selmatch("wl_frameburst", "off", "selected"); %>><script>Capture(share.disable)</script>&nbsp;&nbsp;&nbsp;</option>
    <option value="on" <% nvram_selmatch("wl_frameburst", "on", "selected"); %>><script>Capture(share.enable)</script></option>
    </SELECT>&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;<% support_elsematch("SPEED_BOOSTER_SUPPORT", "1", "<script>Capture(share.enable)</script>", "<script>Capture(share.disable)</script>"); %>)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(wlanadv.beacon)</script>:&nbsp;</TD>
          <TD width=296 height=25><INPUT maxLength=5 onBlur=valid_range(this,1,65535,"Beacon%20Interval") size=6 value='<% nvram_selget("wl_bcn"); %>' name="wl_bcn">&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;100, <script>Capture(hwlad2.milli)</script>, <script>Capture(hwlad2.range)</script>&nbsp;:&nbsp;1 - 65535)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(wlanadv.dtim)</script>:&nbsp;</TD>
          <TD width=296 height=25><INPUT maxLength=3 onBlur=valid_range(this,1,255,"DTIM%20Interval") size=6 value='<% nvram_selget("wl_dtim"); %>' name="wl_dtim">&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;<% get_wl_value("default_dtim"); %>, <script>Capture(hwlad2.range)</script>&nbsp;:&nbsp;1 - 255)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(hwlad2.frathrh)</script>:&nbsp;</TD>
          <TD width=296 height=25><INPUT maxLength=4 onBlur=valid_range(this,256,2346,"Fragmentation%20Threshold") size=6 value='<% nvram_selget("wl_frag"); %>' name="wl_frag">&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;2346, <script>Capture(hwlad2.range)</script>&nbsp;:&nbsp;256 - 2346)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>       
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><script>Capture(wlanadv.rts)</script>:&nbsp;</TD>
          <TD width=296 height=25><INPUT maxLength=4 onBlur=valid_range(this,0,2347,"RTS%20Threshold") size=6 value='<% nvram_selget("wl_rts"); %>' name="wl_rts">&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;2347, <script>Capture(hwlad2.range)</script>&nbsp;:&nbsp;0 - 2347)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>    
         <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><span ><script>Capture(hwlad2.apiso)</script></span>:&nbsp;</span></TD>
          <TD width=296 height=25>
          <SELECT name="wl_ap_isolate"> 
    <option value="0" <% nvram_selmatch("wl_ap_isolate", "0", "selected"); %>><script>Capture(hwlad2.off)</script>&nbsp;&nbsp;&nbsp;</option>
    <option value="1" <% nvram_selmatch("wl_ap_isolate", "1", "selected"); %>><script>Capture(hwlad2.on)</script></option>
    </SELECT>&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;<script>Capture(hwlad2.off)</script>)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% support_invmatch("SES_SUPPORT", "1", "<!--"); %>
         <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><span ><script>Capture(ses.ses)</script></span>:&nbsp;</span></TD>
          <TD width=296 height=25>
          <SELECT name="ses_enable"> 
    <option value="0" <% nvram_selmatch("ses_enable", "0", "selected"); %>><script>Capture(share.disabled)</script>&nbsp;&nbsp;&nbsp;</option>
    <option value="1" <% nvram_selmatch("ses_enable", "1", "selected"); %>><script>Capture(share.enabled)</script></option>
    </SELECT>&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>:&nbsp;<script>Capture(share.enabled)</script>)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% support_invmatch("SES_SUPPORT", "1", "<!--"); %>
         <!--TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><span >Preabmle</span>:</TD>
          <TD width=296 height=25>
          <SELECT name="wl_plcphdr"> 
    <option value="long" <% nvram_selmatch("wl_plcphdr", "long", "selected"); %>>long&nbsp;</option>
    <option value="short" <% nvram_selmatch("wl_plcphdr", "short", "selected"); %>>short</option>
    </SELECT>&nbsp;&nbsp;(Default: long)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR-->

<% support_invmatch("GOOGLE_SUPPORT", "1", "<!--"); %>
          <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25><span><script>Capture(gsa.titlestring)</script> :&nbsp;</span></TD>
	<TD width=296 height=25>
	<SELECT name="google_enable">
	<option value="0" <% nvram_selmatch("google_enable", "0", "selected"); %>><script>Capture(share.disable)</script></option>
	<option value="1" <% nvram_selmatch("google_enable", "1", "selected"); %>><script>Capture(share.enable)</script></option>
	</SELECT>
	&nbsp;&nbsp;(<script>Capture(hwlad2.def)</script>&nbsp;:&nbsp;<script>Capture(share.disable)</script>)</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

<% support_invmatch("GOOGLE_SUPPORT", "1", "-->"); %>


<!--  Wireless QoS is move to QoS.asp

        <TR>
          <TD width=156 bgColor=#000000 height=25>
		<P align=right><b><font face="Arial" color="#FFFFFF" style="font-size: 9pt">Wireless QoS</font></b></P>
          </TD>
          <TD width=8 height=25 bgColor=#000000>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>WME Support:</TD>
          <TD width=296 height=25><SELECT name="wl_wme" onChange=SelWME(this.form.wl_wme.selectedIndex,this.form)> 
    <option value="off" <% nvram_selmatch("wl_wme", "off", "selected"); %>>Off</option>
    <option value="on" <% nvram_selmatch("wl_wme", "on", "selected"); %>>On</option>
    </SELECT>&nbsp;&nbsp;(Default: Off)</TD></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>No-Acknowledgement:</TD>
          <TD width=296 height=25><SELECT name="wl_wme_no_ack"> 
    <option value="off" <% nvram_selmatch("wl_wme_no_ack", "off", "selected"); %>>Off</option>
    <option value="on" <% nvram_selmatch("wl_wme_no_ack", "on", "selected"); %>>On</option>
    </SELECT>&nbsp;&nbsp;(Default: Off)</TD></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25 align=right>EDCA AP Parameters:</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>&nbsp;</TD>
          <TD width=296 height=25>CWmin&nbsp;&nbsp;CWmax&nbsp;&nbsp;&nbsp;AIFSN&nbsp;&nbsp;TXOP(b)&nbsp;&nbsp;TXOP(a/g)&nbsp;Admin Forced</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_BK:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_ap_bk" value="5">
	  <input class=num name="wl_wme_ap_bk0" value="<% nvram_list("wl_wme_ap_bk", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_ap_bk1" value="<% nvram_list("wl_wme_ap_bk", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_ap_bk2" value="<% nvram_list("wl_wme_ap_bk", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_ap_bk3" value="<% nvram_list("wl_wme_ap_bk", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_ap_bk4" value="<% nvram_list("wl_wme_ap_bk", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_ap_bk5">
            <option value="off" <% wme_match_op("wl_wme_ap_bk", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_ap_bk", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_BE:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_ap_be" value="5">
	  <input class=num name="wl_wme_ap_be0" value="<% nvram_list("wl_wme_ap_be", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_ap_be1" value="<% nvram_list("wl_wme_ap_be", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_ap_be2" value="<% nvram_list("wl_wme_ap_be", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_ap_be3" value="<% nvram_list("wl_wme_ap_be", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_ap_be4" value="<% nvram_list("wl_wme_ap_be", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_ap_be5">
            <option value="off" <% wme_match_op("wl_wme_ap_be", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_ap_be", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_VI:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_ap_vi" value="5">
	  <input class=num name="wl_wme_ap_vi0" value="<% nvram_list("wl_wme_ap_vi", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_ap_vi1" value="<% nvram_list("wl_wme_ap_vi", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_ap_vi2" value="<% nvram_list("wl_wme_ap_vi", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_ap_vi3" value="<% nvram_list("wl_wme_ap_vi", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_ap_vi4" value="<% nvram_list("wl_wme_ap_vi", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_ap_vi5">
            <option value="off" <% wme_match_op("wl_wme_ap_vi", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_ap_vi", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_VO:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_ap_vo" value="5">
	  <input class=num name="wl_wme_ap_vo0" value="<% nvram_list("wl_wme_ap_vo", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_ap_vo1" value="<% nvram_list("wl_wme_ap_vo", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_ap_vo2" value="<% nvram_list("wl_wme_ap_vo", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_ap_vo3" value="<% nvram_list("wl_wme_ap_vo", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_ap_vo4" value="<% nvram_list("wl_wme_ap_vo", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_ap_vo5">
            <option value="off" <% wme_match_op("wl_wme_ap_vo", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_ap_vo", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25 align=right>EDCA STA Parameters:</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_BK:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_sta_bk" value="5">
	  <input class=num name="wl_wme_sta_bk0" value="<% nvram_list("wl_wme_sta_bk", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_sta_bk1" value="<% nvram_list("wl_wme_sta_bk", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_sta_bk2" value="<% nvram_list("wl_wme_sta_bk", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_sta_bk3" value="<% nvram_list("wl_wme_sta_bk", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_sta_bk4" value="<% nvram_list("wl_wme_sta_bk", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_sta_bk5">
            <option value="off" <% wme_match_op("wl_wme_sta_bk", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_sta_bk", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_BE:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_sta_be" value="5">
	  <input class=num name="wl_wme_sta_be0" value="<% nvram_list("wl_wme_sta_be", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_sta_be1" value="<% nvram_list("wl_wme_sta_be", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_sta_be2" value="<% nvram_list("wl_wme_sta_be", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_sta_be3" value="<% nvram_list("wl_wme_sta_be", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_sta_be4" value="<% nvram_list("wl_wme_sta_be", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_sta_be5">
            <option value="off" <% wme_match_op("wl_wme_sta_be", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_sta_be", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_VI:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_sta_vi" value="5">
	  <input class=num name="wl_wme_sta_vi0" value="<% nvram_list("wl_wme_sta_vi", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_sta_vi1" value="<% nvram_list("wl_wme_sta_vi", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_sta_vi2" value="<% nvram_list("wl_wme_sta_vi", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_sta_vi3" value="<% nvram_list("wl_wme_sta_vi", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_sta_vi4" value="<% nvram_list("wl_wme_sta_vi", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_sta_vi5">
            <option value="off" <% wme_match_op("wl_wme_sta_vi", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_sta_vi", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=20 height=25>&nbsp;</TD>
          <TD width=125 height=25>AC_VO:</TD>
          <TD width=296 height=25><input type="hidden" name="wl_wme_sta_vo" value="5">
	  <input class=num name="wl_wme_sta_vo0" value="<% nvram_list("wl_wme_sta_vo", 0); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmin")>
	  <input class=num name="wl_wme_sta_vo1" value="<% nvram_list("wl_wme_sta_vo", 1); %>" size="5" maxlength="6" onBlur=valid_range(this,0,32767,"AC%20CWmax")>
	  <input class=num name="wl_wme_sta_vo2" value="<% nvram_list("wl_wme_sta_vo", 2); %>" size="5" maxlength="6" onBlur=valid_range(this,1,15,"AC%20AIFSN")>
	  <input class=num name="wl_wme_sta_vo3" value="<% nvram_list("wl_wme_sta_vo", 3); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(b)")>
	  <input class=num name="wl_wme_sta_vo4" value="<% nvram_list("wl_wme_sta_vo", 4); %>" size="5" maxlength="6" onBlur=valid_range(this,0,65504,"AC%20TXOP(a/g)")>
				<select class=num name="wl_wme_sta_vo5">
            <option value="off" <% wme_match_op("wl_wme_sta_vo", "off", "selected"); %>>Off</option>
            <option value="on" <% wme_match_op("wl_wme_sta_vo", "on", "selected"); %>>On</option>
          </select>
</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

-->

         <TR>
          <TD bgColor=#e7e7e7></TD>
          <TD background=image/UI_04.gif></TD>
          <TD></TD>
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
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><span style="font-family: Arial"><br>

<script>Capture(hwlad2.right1)</script><br>
<b><a target="_blank" href="help/HWireless.asp"><script>Capture(share.more)</script></a></b><br>

</span></font>
          </TD>
          <TD width=9 bgColor=#6666cc height=25>&nbsp;</TD></TR></TBODY></TABLE></TD></TR>
  <TR>
    <TD width=809 colSpan=2>
      <TABLE cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=30>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=454>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD>
          <TD width=176 bgColor=#6666cc rowSpan=2><IMG src="image/UI_Cisco.gif" border=0 width="176" height="64"></TD></TR>
        <TR>
          <TD width=156 bgColor=#000000>&nbsp;</TD>
          <TD width=8 bgColor=#000000>&nbsp;</TD>
          <TD width=454 bgColor=#6666cc align=right>

<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;

<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"Wireless_Advanced.asp\")>");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>

</TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
</FORM></DIV></BODY></HTML>
