
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

<HTML><HEAD><TITLE>Advanced Routing</TITLE>
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capsetup.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<meta http-equiv="content-type" content="text/html; charset=<script>document.write(lang_charset.set)</script>">

<SCRIPT language=javascript>
document.title = topmenu.advrouting;
var route_win = null;
function ViewRoute()
{
	route_win = self.open('RouteTable.asp', 'Route', 'alwaysRaised,resizable,scrollbars,width=720,height=600');
	route_win.focus();
}
function DeleteEntry(F)
{
//      if(confirm("Delete the Entry?")){
        if(confirm(errmsg2.err1)){
		F.submit_button.value = "Routing";
		F.change_action.value = "gozila_cgi";
		F.submit_type.value = 'del';
		F.submit();
	}
}
function to_submit(F)
{
	if(valid_value(F)){
		if( F.wk_mode.value != '<% nvram_get("wk_mode"); %>' ) {
			F.need_reboot.value = "1";
			F.wait_time.value = "10";
		}
		F.submit_button.value = "Routing";
		F.action.value = "Apply";
		F.submit();
	}
}
function valid_value(F)
{
	if(!valid_ip(F,"F.route_ipaddr","IP",0))
		return false;
	if(!valid_mask(F,"F.route_netmask",ZERO_OK))
		return false;
	if(!valid_ip(F,"F.route_gateway","Gateway",MASK_NO))
		return false;

	if(F.route_ipaddr_3.value != "0" && F.route_netmask_3.value == "0") {
		if(errmsg2.err14) {
			alert(errmsg2.err14);	
			return false;
		}
	}

	//if(F.route_ifname.selectedIndex == 0 &&
	//   !valid_ip_gw(F,"F.route_ipaddr","F.route_netmask","F.route_gateway"))
	//	return false;
	return true;
}
function SelRoute(num,F)
{
	F.submit_button.value = "Routing";
	F.change_action.value = "gozila_cgi";
	F.route_page.value=F.route_page.options[num].value;
	F.submit();
}
function SelMode(num,F)
{
        F.submit_button.value = "Routing";
        F.change_action.value = "gozila_cgi";
        F.wk_mode.value=F.wk_mode.options[num].value;
        F.submit();
}

function exit()
{
	closeWin(route_win);
}
</SCRIPT>

</HEAD>
<BODY onunload=exit()>
<DIV align=center>
<FORM name=static action=apply.cgi method=<% get_http_method(); %>>
<input type=hidden name=submit_button>
<input type=hidden name=submit_type>
<input type=hidden name=change_action>
<input type=hidden name=action>
<input type=hidden name=static_route>
<input type=hidden name=need_reboot value=0>
<input type=hidden name=wait_time value=0>
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
      style="FONT-SIZE: 15pt" face=Arial color=#ffffff><script>Capture(bmenu.setup)</script></FONT></H3></TD>
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
                <TD width=83 height=1><IMG height=10 src="image/UI_07.gif" width=83 border=0></TD>
                <TD width=73 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=113 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=103 height=1><IMG height=10 src="image/UI_06.gif" width=103 border=0></TD>
                <TD width=85 height=1><IMG height=10 src="image/UI_06.gif" width=100 border=0></TD>
                <TD width=115 height=1><IMG height=10 src="image/UI_06.gif" width=115 border=0></TD>
                <TD width=74 height=1><IMG height=10 src="image/UI_06.gif" width=79 border=0></TD>
-->
		<script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_07.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_06.gif></TD>")</script>

              </TR>
              <TR>
                <TD align=middle bgColor=#6666cc height=20><FONT 
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
                      <script>document.write("<TD width=" + set_width.w1 + "></TD>")</script>  

                      <!-- TD class=small width=135 -->
		      <script>document.write("<TD class=small width=" + set_width.w2 + ">")</script>	
		      <A href="index.asp"><script>Capture(topmenu.basicsetup)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=75></TD -->
		      <script>document.write("<TD width=" + set_width.w3 + "></TD>")</script>	

                      <!-- TD class=small width=100 -->
		      <script>document.write("<TD class=small width=" + set_width.w4 + ">")</script> 
                      <A href="DDNS.asp"><script>Capture(share.ddns)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=22></TD -->
		      <script>document.write("<TD width=" + set_width.w5 + "></TD>")</script> 

                      <!-- TD class=small width=153 -->
                      <script>document.write("<TD class=small width=" + set_width.w6 + ">")</script>
                      <A href="WanMAC.asp"><script>Capture(topmenu.macaddrclone)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=30></TD -->
                      <script>document.write("<TD width=" + set_width.w7 + "></TD>")</script>

                      <!-- TD width=140 --> 
                      <script>document.write("<TD class=small width=" + set_width.w8 + ">")</script>
                      <FONT style="COLOR: white"><script>Capture(topmenu.advrouting)</script></FONT></TD>

<% support_invmatch("HSIAB_SUPPORT", "1", "<!--"); %>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>
                      <TD width=30></TD>
                      <TD class=small width=140><A href="HotSpot_Admin.asp">Hot Spot</TD>
<% support_invmatch("HSIAB_SUPPORT", "1", "-->"); %>

		      <TD>&nbsp;</TD>		
	
</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1><IMG height=15 
      src="image/UI_03.gif" width=164 border=0></TD>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    width=646 height=1><IMG height=15 src="image/UI_02.gif" width=645 
      border=0></TD></TR></TBODY></TABLE>
<TABLE id=AutoNumber9 style="BORDER-COLLAPSE: collapse" borderColor=#111111 
height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE height=100% cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD align=right width=156 bgColor=#000000 colSpan=3 
            height=25><B><FONT style="FONT-SIZE: 9pt" face=Arial 
            color=#ffffff><script>Capture(topmenu.advrouting)</script></FONT></B></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD width=17 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
        height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>
            <P align=right><FONT style="FONT-WEIGHT: 700"><script>Capture(share.optmode)</script></FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><SELECT name="wk_mode" onChange=SelMode(this.form.wk_mode.selectedIndex,this.form)> 
		<OPTION value="gateway" <% nvram_selmatch("wk_mode", "gateway", "selected"); %>><script>Capture(share.gateway)</script>&nbsp;</OPTION> 
		<OPTION value="router" <% nvram_selmatch("wk_mode", "router", "selected"); %>><script>Capture(share.router)</script></OPTION>
	</SELECT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
        height=25>&nbsp;</TD></TR>

<% nvram_selmatch("wk_mode","gateway","<!--"); %>

        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=1>&nbsp;</TD>
          <TD colSpan=6>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=13 height=1>&nbsp;</TD>
                <TD width=410 colSpan=3 height=1>
                  <HR color=#b5b5e6 SIZE=1>
                </TD>
                <TD width=15>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif 
height=1>&nbsp;</TD></TR>

        
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>
            <P align=right><FONT style="FONT-WEIGHT: 700"><script>Capture(advroute.dynrouting)</script></FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=40>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><span ><script>Capture(advroute.rip)</script>:&nbsp;</span></TD>
          <TD width=296 height=25>
		<SELECT size=1 name=dr_setting> 
			<OPTION value=0 <% nvram_match("dr_setting", "0", "selected"); %>><script>Capture(share.disabled)</script></OPTION>
			<OPTION value=1 <% nvram_match("dr_setting", "1", "selected"); %>><script>Capture(share.lanwireless)</script></OPTION>
			<OPTION value=2 <% nvram_match("dr_setting", "2", "selected"); %>><script>Capture(advroute.waninternet)</script></OPTION> 
			<OPTION value=3 <% nvram_match("dr_setting", "3", "selected"); %>><script>Capture(share.both)</script></OPTION>
		</SELECT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
        height=25>&nbsp;</TD></TR>
<% nvram_selmatch("wk_mode","gateway","-->"); %>        
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=1>&nbsp;</TD>
          <TD colSpan=6>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=13 height=1>&nbsp;</TD>
                <TD width=410 colSpan=3 height=1>
                  <HR color=#b5b5e6 SIZE=1>
                </TD>
                <TD width=15>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif 
height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>
            <P align=right><FONT style="FONT-WEIGHT: 700"><script>Capture(lefemenu.staticroute)</script></FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><script>Capture(advroute.selsetnum)</script>:&nbsp;</TD>
          <TD width=296 height=25>
            <TABLE id=AutoNumber10 style="BORDER-COLLAPSE: collapse" 
            borderColor=#111111 cellSpacing=0 cellPadding=0 width=265 align=left 
            border=0>
              <TBODY>
              <TR>
                <TD width=120 colSpan=2>&nbsp;<SELECT size=1 name="route_page" onChange=SelRoute(this.form.route_page.selectedIndex,this.form)>
	<% static_route_table("select"); %></SELECT>
                <TD>
                <TD>&nbsp;&nbsp;

<script>document.write("<INPUT onclick=DeleteEntry(this.form) type=button name=delentry value=\"" + advroute.delentries + "\">");</script>

                </TD></TR></TBODY></TABLE></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
        height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=45>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=45>&nbsp;</TD>
          <TD colSpan=3 height=45>&nbsp;</TD>
          <TD width=101 height=45><script>Capture(advroute.routename)</script>:&nbsp;</TD>
          <TD width=296 height=45>&nbsp;<input name="route_name" value='<% static_route_setting("name",""); %>' size="20" maxlength="20" onBlur=valid_name(this,"Route%20Name") class=num> </TD>
          <TD width=13 height=45>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=45>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=24>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=24><FONT style="FONT-SIZE: 8pt"><script>Capture(advroute.deslanip)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=24><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<input type="hidden" name="route_ipaddr" value="4"><input name="route_ipaddr_0" value="<% static_route_setting("ipaddr","0"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,223,"IP") class=num>.
	<input name="route_ipaddr_1" value="<% static_route_setting("ipaddr","1"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>.
	<input name="route_ipaddr_2" value="<% static_route_setting("ipaddr","2"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>.
	<input name="route_ipaddr_3" value="<% static_route_setting("ipaddr","3"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,254,"IP") class=num>
</SPAN></TD>
          <TD width=13 height=24>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
        height=24>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=24>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=24><FONT style="FONT-SIZE: 8pt"><SPAN 
            ><script>Capture(share.submask)</script>:&nbsp;</SPAN></FONT></TD>
          <TD width=296 height=24>&nbsp;<input type="hidden" name="route_netmask" value="4"><input name="route_netmask_0" value="<% static_route_setting("netmask","0"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>.
	<input name="route_netmask_1" value="<% static_route_setting("netmask","1"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>.
	<input name="route_netmask_2" value="<% static_route_setting("netmask","2"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>.
	<input name="route_netmask_3" value="<% static_route_setting("netmask","3"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>
</TD>
          <TD width=13 height=24>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
        height=24>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=24>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=24><FONT style="FONT-SIZE: 8pt"><SPAN 
            ><script>Capture(share.defgateway)</script>:&nbsp;</SPAN></FONT></TD>
          <TD width=296 height=24>&nbsp;<input type="hidden" name="route_gateway" value="4"><input name="route_gateway_0" value="<% static_route_setting("gateway","0"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,223,"IP") class=num>.
	<input name="route_gateway_1" value="<% static_route_setting("gateway","1"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>.
	<input name="route_gateway_2" value="<% static_route_setting("gateway","2"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,255,"IP") class=num>.
	<input name="route_gateway_3" value="<% static_route_setting("gateway","3"); %>" size="3" maxlength="3" onBlur=valid_range(this,0,254,"IP") class=num>
</TD>
          <TD width=13 height=24>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
        height=24>&nbsp;</TD></TR>
        
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt"><script>Capture(share.inter_face)</script>:&nbsp; 
            </FONT></TD>
          <TD align=left width=296 height=25>
            <TABLE cellSpacing=0 cellPadding=0 width=265 border=0>
              <TBODY>
              <TR>
                <TD width=265>&nbsp;<select name="route_ifname">
          <option value="lan" <% static_route_setting("lan","0"); %>><script>Capture(share.lanwireless)</script></option>
          <option value="wan" <% static_route_setting("wan","0"); %>><script>Capture(advroute.waninternet)</script></option>
        </select>
                <TD>
                <TD align=middle width=100 height=25></TD>
                <TD>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=1>&nbsp;</TD>
          <TD colSpan=6>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=13 height=1>&nbsp;</TD>
                <TD width=410 colSpan=3 height=1>
                  <HR color=#b5b5e6 SIZE=1>
                </TD>
                <TD width=15>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif 
height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=1>&nbsp;</TD>
          <TD colSpan=6>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=13 height=1>&nbsp;</TD>
                <TD width=410 colSpan=3 height=1>

<script>document.write("<INPUT onclick=ViewRoute() type=button name=button2 value=\"" + advroute.showroutetbl + "\">");</script>&nbsp;
                </TD>
                <TD width=15>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>

        <TR>
          <TD width=15 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=65 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=47 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=454 colSpan=6>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR>


	</TBODY></TABLE></TD>

    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25>&nbsp;</TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><span ><br>

<script>Capture(hrouting2.right1)</script><br>
<script>document.write("<br>");</script>
<script>Capture(hrouting2.right2)</script><br>
<script>document.write("<br>");</script>
<script>Capture(hrouting2.right3)</script><br>
<script>document.write("<br>");</script>
<script>Capture(hrouting2.right4)</script><br>
<script>document.write("<br>");</script>
<script>Capture(hrouting2.right5)</script><br>
<b><a target="_blank" href="help/HRouting.asp"><script>Capture(share.more)</script></a></span></font></TD>

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
          <TD width=454 bgColor=#6666cc align=right>
			<INPUT type=hidden value=0 name=Route_reload>

<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;
<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"Routing.asp\")>");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>

</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
