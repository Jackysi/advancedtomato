
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

<HTML><HEAD><TITLE>DMZ</TITLE>
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
document.title = share.dmz;
var EN_DIS = '<% nvram_get("dmz_enable"); %>'
function to_submit(F)
{
	if(F.dmz_enable[0].checked == true){
		if(F.dmz_ipaddr.value == "0"){
//                      alert("Illegal DMZ IP Address!");
                        alert(errmsg.err9);
			F.dmz_ipaddr.focus();
			return false;
		}	
	}
	
	F.submit_button.value = "DMZ";
	F.action.value = "Apply";
        F.submit();
}
function dmz_enable_disable(F,I)
{
	EN_DIS1 = I;
	if ( I == "0" ){
		choose_disable(F.dmz_ipaddr);
	}
	else{
		choose_enable(F.dmz_ipaddr);
	}
}
function SelDMZ(F,num)
{
	dmz_enable_disable(F,num);
}
function init() 
{               
	dmz_enable_disable(document.dmz,'<% nvram_get("dmz_enable"); %>');
}
</SCRIPT>
</HEAD>
<BODY onload=init()>
<DIV align=center>
<FORM name=dmz method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button value="DMZ">
<input type=hidden name=change_action>
<input type=hidden name=action value="Apply">
<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95><IMG src="image/UI_Linksys.gif" border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT 
      style="FONT-SIZE: 7pt" color=#ffffff><script>Capture(share.firmwarever)</script>&nbsp;:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></TD>
  </TR>
  <TR>
    <TD width=808 bgColor=#6666cc colSpan=2><IMG height=11 src="image/UI_10.gif" width=809 border=0></TD>
  </TR>
  </TBODY>
</TABLE>
<TABLE height=77 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR>
    <TD width=163 height=49>
      <H3><script>Capture(bmenu.applications)</script><BR>&amp; <script>Capture(bmenu.gaming)</script></H3></TD>
    <TD width=646 bgColor=#000000 height=49>
      <TABLE 
      style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black;" 
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
          style="FONT-WEIGHT: normal; FONT-SIZE: 1pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=537 bgColor=#000000 height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=646 bgColor=#000000 colSpan=2 height=32>
            <TABLE id=AutoNumber1 
            style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" height=6 cellSpacing=0 cellPadding=0 width=646 border=0>
              <TBODY>
              <TR style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" align=middle bgColor=#6666cc>

<!--
                <TD width=83 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=73 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=113 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=103 height=1><IMG height=10 src="image/UI_06.gif" width=103 border=0></TD>
                <TD width=85 height=1><IMG height=10 src="image/UI_07.gif" width=100 border=0></TD>
                <TD width=115 height=1><IMG height=10 src="image/UI_06.gif" width=115 border=0></TD>
                <TD width=74 height=1><IMG height=10 src="image/UI_06.gif" width=79 border=0></TD>
-->
                <script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_07.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_06.gif></TD>")</script>

              </TR>
              <TR>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="index.asp"><script>Capture(bmenu.setup)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="Wireless_Basic.asp"><script>Capture(bmenu.wireless)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="Firewall.asp"><script>Capture(bmenu.security)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <A style="TEXT-DECORATION: none" href="<% support_elsematch("PARENTAL_CONTROL_SUPPORT", "1", "Parental_Control.asp", "Filters.asp"); %>"><script>Capture(bmenu.accrestriction)</script></A></FONT></TD>
                <TD align=middle bgColor=#6666cc height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" 
                  color=#ffffff><script>Capture(bmenu.applications)</script> <BR>&amp; <script>Capture(bmenu.gaming)</script>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Management.asp"><script>Capture(bmenu.admin)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Status_Router.asp"><script>Capture(bmenu.statu)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
              </TR>              
	      <TR>
                <TD width=643 bgColor=#6666cc colSpan=7 height=21>
                  <TABLE borderColor=black height=21 cellSpacing=0 cellPadding=0 width=643>
                    <TBODY>
                    <TR align=left>

                      <script>document.write("<TD width=" + for_width.w1 + "></TD>")</script> 

                      <script>document.write("<TD class=small width=" + for_width.w2 + ">")</script> 
                      <A href="Forward.asp"><script>Capture(apptopmenu.portrange)</script></A></TD>
<% support_invmatch("PORT_TRIGGER_SUPPORT", "1", "<!--"); %>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <script>document.write("<TD width=" + for_width.w3 + "></TD>")</script>

                      <script>document.write("<TD class=small width=" + for_width.w4 + ">")</script>
                      <A href="Triggering.asp"><script>Capture(trigger2.ptrigger)</script></A></TD>
<% support_invmatch("PORT_TRIGGER_SUPPORT", "1", "-->"); %>
<% support_invmatch("UPNP_FORWARD_SUPPORT", "1", "<!--"); %>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <script>document.write("<TD width=" + for_width.w5 + "></TD>")</script>

                      <script>document.write("<TD class=small width=" + for_width.w6 + ">")</script> 
                      <A href="Forward_UPnP.asp">UPnP Forward</A></TD>
<% support_invmatch("UPNP_FORWARD_SUPPORT", "1", "-->"); %>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <script>document.write("<TD width=" + for_width.w7 + "></TD>")</script>

                      <script>document.write("<TD class=small width=" + for_width.w8 + ">")</script> 
                      <FONT style="COLOR: white"><script>Capture(share.dmz)</script></FONT></TD>
<% support_invmatch("HW_QOS_SUPPORT", "1", "<!--"); %>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <script>document.write("<TD width=" + for_width.w9 + "></TD>")</script>

                      <script>document.write("<TD class=small width=" + for_width.w10 + ">")</script>
                      <a href="QoS.asp"><script>Capture(trigger2.qos)</script></a></TD>
<% support_invmatch("HW_QOS_SUPPORT", "1", "-->"); %>

		      <TD>&nbsp;</TD>

                    </TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD>
    <IMG height=15 src="image/UI_03.gif" width=164 border=0></TD>
    <TD>
    <IMG height=15 src="image/UI_02.gif" width=645 border=0></TD></TR></TBODY></TABLE>

<TABLE height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE height=100% cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD width=156 bgColor=#000000 colSpan=3 height=25 align=right><B><FONT style="FONT-SIZE: 9pt" color=#ffffff><script>Capture(share.dmz)</script></FONT></B></TD>
          <TD width=8 bgColor=#000000 height=25></TD>
          <TD width=42 height=25>&nbsp;</TD>
          <TD width=161 height=25>&nbsp;</TD>
          <TD width=238 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&#12288;</TD></TR>
     
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&#12288;</TD>
          <td width=8 height=25 align=right background=image/UI_04.gif>&nbsp;</td>
          <TD height=25 width=42>&#12288;</TD>
          <TD width=399 height=25 colSpan=2><INPUT type=radio value=1 name=dmz_enable <% nvram_match("dmz_enable","1","checked"); %> onClick=SelDMZ(this.form,1)><B><script>Capture(share.enable)</script></B>&nbsp;<INPUT type=radio value=0 name=dmz_enable <% nvram_match("dmz_enable","0","checked"); %> onClick=SelDMZ(this.form,0)><B><script>Capture(share.disable)</script></B></TD>
          <TD width=13 height=25>&#12288;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&#12288;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&#12288;</TD>
          <td width=8 height=25 align=right background=image/UI_04.gif>&nbsp;</td>
          <TD height=25 width=42>&#12288;</TD>
          <TD width=161 height=25>&nbsp;&nbsp;<script>Capture(dmz.dmzhostip)</script>&nbsp;:&nbsp;</TD>
          <TD width=238 height=25><B>&nbsp;<% prefix_ip_get("lan_ipaddr",1); %><INPUT class=num maxLength=3 onBlur=valid_range(this,1,254,"IP") size=3 value='<% nvram_get("dmz_ipaddr"); %>' name="dmz_ipaddr"></b></TD>
          <TD width=13 height=25>&#12288;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&#12288;</TD></TR>
 <!--          <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&#12288;</TD>
          <td width="8" height="25" align=right><img border="0" src="image/UI_04.gif" width="8" height="28"></td>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=13 height=1>&nbsp;</TD>
                <TD width=410 colSpan=3 height=1>
                  <HR color=#b5b5e6 SIZE=1>
                </TD>
                <TD width=15>&#12288;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&#12288;</TD></TR>
-->          
       <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3></TD>
          <TD width=8 background=image/UI_04.gif></TD>
          <TD width=42></TD>
          <TD width=161></TD>
          <TD width=238></TD>
          <TD width=13></TD>
          <TD width=15 background=image/UI_05.gif></TD></TR>

      </TBODY>
      </TABLE></TD>
    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=10 bgColor=#6666cc height=25>&#12288;</TD>
          <TD width=156 bgColor=#6666cc height=25><br><span style="font-family: Arial"><font color="#FFFFFF">
<script>Capture(hdmz2.right1)</script><br>
<b><a target="_blank" href="help/HDMZ.asp"><script>Capture(share.more)</script></a></b>
</span></font>
          </TD>
          <TD width=9 bgColor=#6666cc height=25>&#12288;</TD>
        </TR>
        </TBODY>
      </TABLE>
    </TD>
  </TR>

 <tr>
                <td width="809" colspan=2>
                 <table border="0" cellpadding="0" cellspacing="0">
		  <tr> 
		      <td width="156" height="25" bgcolor="#E7E7E7">&nbsp;</td>
                      <td width="8" height="25"><img border="0" src="image/UI_04.gif" width="8" height="30"></td>
                      <td width="454" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                      <td width="15" height="25" bgcolor="#FFFFFF"><img border="0" src="image/UI_05.gif" height="30" width="15"></td>
                      <td width="176" bgcolor="#6666CC" rowspan="2"><img border="0" src="image/UI_Cisco.gif" width="176" height="64"></td>
		  </tr>
		  <tr>
		      <td width="156" height="33" bgcolor="#000000">&nbsp;</td>
                      <td width="8" height="33" bgcolor="#000000">&nbsp;</td>
                      <td width="454" height="33" bgcolor="#6666CC">
				<p align="right">
<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;
<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"DMZ.asp\")>");</script>&nbsp;&nbsp;
                      </td>
                      <td width="15" height="33" bgcolor="#000000">&nbsp;</td>
		  </tr>
		</table>
	       </td>
              </tr>
    </TBODY>
    </TABLE>
    </FORM>
    </DIV>
    </BODY>
    </HTML>
