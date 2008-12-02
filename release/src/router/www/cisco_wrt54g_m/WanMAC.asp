
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

<HTML><HEAD><TITLE>MAC Address Clone</TITLE>
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

<SCRIPT language=JavaScript>
document.title = topmenu.macaddrclone;
function to_submit(F)
{	
	if(valid_macs_00(F))
        {
		F.submit_button.value = "WanMAC";
		F.action.value = "Apply";
	       	F.submit();
	}
}
function CloneMAC(F)
{
       	F.submit_button.value = "WanMAC";
      	F.change_action.value = "gozila_cgi";
	F.submit_type.value = "clone_mac";
       	F.action.value = "Apply";
       	F.submit();
}
function SelMac(num,F)
{
        mac_enable_disable(F,num);
}
function mac_enable_disable(F,I)
{
        EN_DIS3 = I;
        if ( I == "0" ){
                choose_disable(F.def_hwaddr);
                choose_disable(F.def_hwaddr_0);
                choose_disable(F.def_hwaddr_1);
                choose_disable(F.def_hwaddr_2);
                choose_disable(F.def_hwaddr_3);
                choose_disable(F.def_hwaddr_4);
                choose_disable(F.def_hwaddr_5);
                choose_disable(F.clone_b);
        }
        else{
                choose_enable(F.def_hwaddr);
                choose_enable(F.def_hwaddr_0);
                choose_enable(F.def_hwaddr_1);
                choose_enable(F.def_hwaddr_2);
                choose_enable(F.def_hwaddr_3);
                choose_enable(F.def_hwaddr_4);
                choose_enable(F.def_hwaddr_5);
                choose_enable(F.clone_b);
        }
}
function init() 
{               
        mac_enable_disable(document.mac,'<% nvram_get("mac_clone_enable"); %>');
	<% onload("MACClone", "document.mac.mac_clone_enable[0].checked = true; mac_enable_disable(document.mac,1);"); %>	
}

function valid_macs_00(F)
{
//        M1 = "The MAC Address is not correct!!";
        M1 = errmsg.err17;
        if(F.def_hwaddr_0.value == 00 && F.def_hwaddr_1.value == 00 && F.def_hwaddr_2.value == 00 &&  F.def_hwaddr_3.value == 00 && F.def_hwaddr_4.value == 00 && F.def_hwaddr_5.value == 00)
        {
                F.def_hwaddr_0.value = F.def_hwaddr_0.defaultValue;
                F.def_hwaddr_1.value = F.def_hwaddr_1.defaultValue;
                F.def_hwaddr_2.value = F.def_hwaddr_2.defaultValue;
                F.def_hwaddr_3.value = F.def_hwaddr_3.defaultValue;
                F.def_hwaddr_4.value = F.def_hwaddr_4.defaultValue;
                F.def_hwaddr_5.value = F.def_hwaddr_5.defaultValue;
                alert(M1);
                return false;
        }
        else
        {
                return true;
        }
}
</SCRIPT>

</HEAD>
<BODY onload=init()>
<DIV align=center>
<FORM name=mac method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=submit_type>
<input type=hidden name=action>
<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95><IMG src="image/UI_Linksys.gif" border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT 
      style="FONT-SIZE: 7pt" color=#ffffff><FONT face=Arial><script>Capture(share.firmwarever)</script>:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></FONT></TD></TR>
  <TR>
    <TD width=808 bgColor=#6666cc colSpan=2><IMG height=11 
      src="image/UI_10.gif" width=809 border=0></TD></TR></TBODY></TABLE>
<TABLE height=77 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" borderColor=#000000 align=middle width=163 height=49>
      <H3 style="MARGIN-TOP: 1px; MARGIN-BOTTOM: 1px"><FONT 
      style="FONT-SIZE: 15pt" face=Arial color=#ffffff><script>Capture(bmenu.setup)</script></FONT></H3></TD>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    vAlign=center borderColor=#000000 width=646 bgColor=#000000 height=49>
      <TABLE style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" 
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
          <TD style="FONT-WEIGHT: normal; FONT-SIZE: 1pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=537 bgColor=#000000 height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=646 bgColor=#000000 colSpan=2 height=32>
            <TABLE id=AutoNumber1 
            style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" 
            height=6 cellSpacing=0 cellPadding=0 width=646 border=0>
              <TBODY>
              <TR 
              style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" align=middle bgColor=#6666cc>

                <script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_07.gif></TD>")</script>  
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_06.gif></TD>")</script>  
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_06.gif></TD>")</script>  
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_06.gif></TD>")</script>  
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_06.gif></TD>")</script>  
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_06.gif></TD>")</script>  
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_06.gif></TD>")</script>  

<!--
                <TD width=83 height=8 background=image/UI_07.gif></TD>
                <TD width=83 height=8 background=image/UI_06.gif></TD>
                <TD width=83 height=8 background=image/UI_06.gif></TD>
                <TD width=103 height=8 background=image/UI_06.gif></TD>
                <TD width=100 height=8 background=image/UI_06.gif></TD>
                <TD width=115 height=8 background=image/UI_06.gif></TD>
                <TD width=79 height=8 background=image/UI_06.gif></TD>
-->
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

                      <TD width=1 align=center><font color='white'><b>|</b></font></TD>

                      <!-- TD width=75></TD -->
                      <script>document.write("<TD width=" + set_width.w3 + "></TD>")</script>  

                      <!-- TD class=small width=100 -->
                      <script>document.write("<TD class=small width=" + set_width.w4 + ">")</script>  
                      <A href="DDNS.asp"><script>Capture(share.ddns)</script></A></TD>

                      <TD width=1 align=center><font color='white'><b>|</b></font></TD>

                      <!-- TD width=22></TD -->
                      <script>document.write("<TD width=" + set_width.w5 + "></TD>")</script>  

                      <!-- TD width=153 -->
                      <script>document.write("<TD class=small width=" + set_width.w6 + ">")</script>  
                      <FONT style="COLOR: white"><script>Capture(topmenu.macaddrclone)</script></FONT></TD>

                      <TD width=1 align=center><font color='white'><b>|</b></font></TD>

                      <!-- TD width=30></TD -->
                      <script>document.write("<TD width=" + set_width.w7 + "></TD>")</script>  

                      <!-- TD class=small width=140 -->
                      <script>document.write("<TD class=small width=" + set_width.w8 + ">")</script>  
                      <A href="Routing.asp"><script>Capture(topmenu.advrouting)</script></A></TD>

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
          <TD width=156 bgColor=#000000 height=25>
            <P align=right><B><FONT style="FONT-SIZE: 9pt" face=Arial 
            color=#ffffff><script>Capture(share.macclone)</script></FONT></B></P></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD width=440 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>        
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=24>&nbsp;</TD>
          <TD width=14 height=24>&nbsp;</TD>
          <TD width=440 height=24><INPUT type=radio value=1 name=mac_clone_enable <% nvram_match("mac_clone_enable", "1", "checked"); %> OnClick=SelMac(1,this.form)><B><script>Capture(share.enable)</script></B>&nbsp;&nbsp;
                <INPUT type=radio value=0 name=mac_clone_enable <% nvram_match("mac_clone_enable", "0", "checked"); %> OnClick=SelMac(0,this.form)><B><script>Capture(share.disable)</script></B></TD>
          <TD width=15 background=image/UI_05.gif height=24>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=24>&nbsp;</TD>
          <TD width=14 height=24>&nbsp;</TD>
          <TD width=440 height=24>

&nbsp;&nbsp;<script>Capture(macclone.usrdef)</script>:&nbsp;
                                  <input type=hidden name="def_hwaddr"  value="6">
                                  <input name="def_hwaddr_0" value='<% get_clone_mac("0"); %>' size=2 maxlength=2 onBlur=valid_mac(this,0) class=num> :
                                  <input name="def_hwaddr_1" value='<% get_clone_mac("1"); %>' size=2 maxlength=2 onBlur=valid_mac(this,1) class=num> :
                                  <input name="def_hwaddr_2" value='<% get_clone_mac("2"); %>' size=2 maxlength=2 onBlur=valid_mac(this,1) class=num> :
                                  <input name="def_hwaddr_3" value='<% get_clone_mac("3"); %>' size=2 maxlength=2 onBlur=valid_mac(this,1) class=num> :
                                  <input name="def_hwaddr_4" value='<% get_clone_mac("4"); %>' size=2 maxlength=2 onBlur=valid_mac(this,1) class=num> :
                                  <input name="def_hwaddr_5" value='<% get_clone_mac("5"); %>' size=2 maxlength=2 onBlur=valid_mac(this,1) class=num>&nbsp;&nbsp;&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=24>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=14 height=24>&nbsp;</TD>
          <TD width=440 height=25>
           <TABLE>
              <TBODY>
              <TR>
                <TD width=1></TD>
                <TD width=439 align=left>
			<script>document.write("<INPUT onclick=CloneMAC(this.form) type=button name=clone_b value=\"" + macclone.clonepcmac + "\">");</script>
                </TD>
              </TR>
              </TBODY>
            </TABLE>
           </TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=14 height=24>&nbsp;</TD>
          <TD width=440 height=25>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=430>
			<HR color=#b5b5e6 SIZE=1>
                </TD>
                <TD width=10>&nbsp;</TD>
              </TR>
              </TBODY>
            </TABLE>
          </TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

	 <TR>
          <TD width=156 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=14>&nbsp;</TD>
          <TD width=440>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR>



    </TBODY></TABLE></TD>
    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25>&nbsp;</TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><span style="font-family: Arial"><br>

<script>Capture(hwmac2.right1)</script><br><b><a target="_blank" href="help/HMAC.asp"><script>Capture(share.more)</script></a></b></span></font></TD>

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
          <TD width=176 bgColor=#6666cc rowSpan=2>
          <IMG src="image/UI_Cisco.gif" border=0 width="176" height="64"></TD></TR>
        <TR>
          <TD width=156 bgColor=#000000>&nbsp;</TD>
          <TD width=8 bgColor=#000000>&nbsp;</TD>
          <TD width=454 bgColor=#6666cc align=right>

<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;

<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"WanMAC.asp\")>");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>
          </TR>
          </TBODY>
          </TABLE>
          </TD>
          </TR>
          </TBODY>
          </TABLE>
          </FORM>
          </DIV>
          </BODY>
          </HTML>
