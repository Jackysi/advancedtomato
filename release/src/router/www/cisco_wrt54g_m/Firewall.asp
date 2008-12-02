
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

<HTML><HEAD><TITLE>Firewall</TITLE>
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
document.title = share.firewall;
function wan_enable_disable(F)
{
	if(F._block_wan.checked == true) 
		choose_enable(F._ident_pass);	
	else {
		choose_disable(F._ident_pass);	
	}
}

function to_submit(F)
{
	F.submit_button.value = "Firewall";
	if(F._block_wan.checked == true) 
		F.block_wan.value = 1;
	else {
		F.block_wan.value = 0;
	}
	if(F._block_loopback){
		if(F._block_loopback.checked == true) F.block_loopback.value = 1;
		else 				 F.block_loopback.value = 0;
	}
	if(F._block_cookie){
		if(F._block_cookie.checked == true) F.block_cookie.value = 1;
		else 				 F.block_cookie.value = 0;
	}
	if(F._block_java){
		if(F._block_java.checked == true) F.block_java.value = 1;
		else 				 F.block_java.value = 0;
	}
	if(F._block_proxy){
		if(F._block_proxy.checked == true) F.block_proxy.value = 1;
		else 				 F.block_proxy.value = 0;
	}
	if(F._block_activex){
		if(F._block_activex.checked == true) F.block_activex.value = 1;
		else 				 F.block_activex.value = 0;
	}
	if(F._ident_pass){
		if(F._ident_pass.checked == true) F.ident_pass.value = 0;
		else 				 F.ident_pass.value = 1;
	}

	if(F._block_multicast){
		if(F._block_multicast.checked == true) F.multicast_pass.value = 0;
		else				F.multicast_pass.value = 1;
	}

	F.action.value = "Apply";
        F.submit();
}
function init()
{
	if(document.firewall._block_wan.checked == true) 
                choose_enable(document.firewall._ident_pass);   
        else {
                choose_disable(document.firewall._ident_pass);  
        }
}

</SCRIPT>

</HEAD>
<BODY onload=init()>
<DIV align=center>
<FORM name=firewall method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=action>
<input type=hidden name=block_wan>
<input type=hidden name=block_loopback>
<input type=hidden name=multicast_pass value=0>
<input type=hidden name=ident_pass>
<input type=hidden name=block_cookie value=0>
<input type=hidden name=block_java value=0>
<input type=hidden name=block_proxy value=0>
<input type=hidden name=block_activex value=0>

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
      <H3 style="MARGIN-TOP: 1px; MARGIN-BOTTOM: 1px">
      <FONT 
      style="FONT-SIZE: 15pt" face=Arial color=#ffffff><script>Capture(bmenu.security)</script></FONT></H3></TD>
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
                <TD width=73 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=113 height=1><IMG height=10 src="image/UI_07.gif" width=83 border=0></TD>
                <TD width=103 height=1><IMG height=10 src="image/UI_06.gif" width=103 border=0></TD>
                <TD width=85 height=1><IMG height=10 src="image/UI_06.gif" width=100 border=0></TD>
                <TD width=115 height=1><IMG height=10 src="image/UI_06.gif" width=115 border=0></TD>
                <TD width=74 height=1><IMG height=10 src="image/UI_06.gif" width=79 border=0></TD>
-->
                <script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_07.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_06.gif></TD>")</script>

              </TR>
              <TR>
                <TD align=middle bgColor=#000000 height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff><A style="TEXT-DECORATION: none" 
                  href="index.asp"><script>Capture(bmenu.setup)</script></A></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="Wireless_Basic.asp"><script>Capture(bmenu.wireless)</script></a></FONT></TD>
                <TD align=middle bgColor=#6666cc height=20><FONT 
                  style="FONT-WEIGHT: 700" color=#ffffff><script>Capture(bmenu.security)</script></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="<% support_elsematch("PARENTAL_CONTROL_SUPPORT", "1", "Parental_Control.asp", "Filters.asp"); %>"><script>Capture(bmenu.accrestriction)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Forward.asp"><script>Capture(bmenu.applications)</script> <BR>&amp; <script>Capture(bmenu.gaming)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Management.asp"><script>Capture(bmenu.admin)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                  <a style="TEXT-DECORATION: none" href="Status_Router.asp"><script>Capture(bmenu.statu)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
              </TR>
              <TR>
                <TD width=643 bgColor=#6666cc colSpan=7 height=21>
                  <TABLE height=21 cellSpacing=0 cellPadding=0 width=643>
                    <TBODY>
                    <TR align=left>

		      <!-- TD width=20></TD -->
                      <script>document.write("<TD width=" + fi_width.w1 + "></TD>")</script>  

                      <!-- TD width=61 -->
                      <script>document.write("<TD width=" + fi_width.w2 + ">")</script>  
                      <FONT style="COLOR: white"><script>Capture(share.firewall)</script></FONT></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=33></TD -->
                      <script>document.write("<TD width=" + fi_width.w3 + "></TD>")</script>  

                      <!-- TD class=small width=49 -->
                      <script>document.write("<TD class=small width=" + fi_width.w4 + ">")</script>  
                      <a href="VPN.asp"><script>Capture(share.vpn)</script></a></TD>

                      <TD width=1><!--P class=bar><font color='white'><b>|</b></font></P--></TD>

                      <!-- TD width=30></TD -->
                      <script>document.write("<TD width=" + fi_width.w5 + "></TD>")</script>  

                      <!-- TD class=small width=292 -->
                      <script>document.write("<TD class=small width=" + fi_width.w6 + ">")</script>  
                      <!--span >&nbsp;</span><a href="WL_WPATable.asp">Wireless</a--></TD>          

		      <TD>&nbsp;</TD>

		   </TR>
</TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1><IMG height=15 src="image/UI_03.gif" width=164 border=0></TD>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=646 height=1><IMG height=15 src="image/UI_02.gif" width=645 
      border=0></TD></TR></TBODY></TABLE>
<TABLE id=AutoNumber9 style="BORDER-COLLAPSE: collapse" borderColor=#111111 height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE height=100% cellSpacing=0 cellPadding=0 border=0 width="633">
        <TBODY>
        <TR>
          <TD width=156 bgColor=#000000 height=25>
            <p align="right"><b><font face="Arial" color="#FFFFFF" style="font-size: 9pt">
                <script>Capture(share.firewall)</script></font></b></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=11 height=25>&nbsp;</TD>
          <TD width=17 height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=15 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        
        <TR>
          <TD width=156 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD colSpan=2 height=1 width="28">&nbsp;</TD>
          <TD width=411 colSpan=3 height=1>&nbsp;&nbsp;&nbsp;&nbsp;<script>Capture(firewall.firewallpro)</script>:&nbsp;&nbsp;<INPUT type=radio value=on name=filter <% nvram_match("filter","on","checked"); %>><B><script>Capture(share.enable)</script></B>&nbsp;<INPUT type=radio value=off name=filter <% nvram_match("filter","off","checked"); %>><B><script>Capture(share.disable)</script></B></TD>
          <TD width=15>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR>
         <TR>
          <TD width=156 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD colSpan=2 height=1 width="28">&nbsp;</TD>
          <TD width=411 colSpan=3 height=1>
            <HR color=#b5b5e6 SIZE=1>
          </TD>
          <TD width=15>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR>

        <!--TR>
          <TD width=156 bgColor=#e7e7e7 height=25>
          <p align="right">
                <font face="Arial" style="font-size: 8pt; font-weight: 700"> Additional Filters</font></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><table border="0" cellpadding="0" cellspacing="0" id="AutoNumber12" width="300">
                  <tr>
                    <td height="25" width="27"><INPUT type=checkbox value=1 name="_block_proxy" <% nvram_match("block_proxy","1","checked"); %>><b>
				
		    </td>
                    <td height="25" width="110"> Filter Proxy&nbsp;
		    </td>
                    <td height="25" width="27">
    <INPUT type=checkbox value=1 name="_block_cookie" <% nvram_match("block_cookie","1","checked"); %>><b>
		    </td>
                    <td height="25" width="90"> Filter Cookies&nbsp;
		    </td>
                  </tr>
                </table>
</TD>
          <TD width=15 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><table border="0" cellpadding="0" cellspacing="0" id="AutoNumber12" width="300" >
                  <tr>
                    <td height="25" width="27">
    <INPUT type=checkbox value=1 name="_block_java" <% nvram_match("block_java","1","checked"); %>><b>
				
		    </td>
                    <td height="25" width="110"> Filter Java Applets&nbsp;
		    </td>
                    <td height="25" width="27">
    <INPUT type=checkbox value=1 name="_block_activex" <% nvram_match("block_activex","1","checked"); %>><b>
				
		    </td>
                    <td height="25" width="90"> Filter ActiveX&nbsp;
		    </td>
                  </tr>
                </table>
</TD>
          <TD width=15 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
       
        <TR>
          <TD width=156 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD colSpan=2 height=1 width="28">&nbsp;</TD>
          <TD width=411 colSpan=3 height=1>
            <HR color=#b5b5e6 SIZE=1>
          </TD>
          <TD width=15>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR-->

        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>
          <p align="right"><font face="Arial" style="font-size: 8pt; font-weight: 700">
                <script>Capture(secleftmenu.blockwan)</script></font></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><table border="0" cellpadding="0" cellspacing="0" id="AutoNumber12" width="300">
                  <tr>
                    <td height="25" width="27"><input type=checkbox value=1 name=_block_wan <% nvram_match("block_wan","1","checked"); %> onclick=wan_enable_disable(this.form)></td>
                    <td height="25" width="240"> <script>Capture(firewall.blockinterreq)</script>&nbsp; </td>
                  </tr>
                </table>
</TD>
          <TD width=15 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>               
<% support_invmatch("MULTICAST_SUPPORT", "1", "<!--"); %>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>
          <p align="right">
                <font face="Arial" style="font-size: 8pt; font-weight: 700"></font></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><table border="0" cellpadding="0" cellspacing="0" id="AutoNumber12" width="300">
                  <tr>
                    <td height="25" width="27"><input type=checkbox value=0 name=_block_multicast <% nvram_match("multicast_pass","0","checked"); %>></td>
                    <td height="25" width="240"> <script>Capture(firewall2.multi)</script>&nbsp; </td>
                  </tr>
                </table>
</TD>
          <TD width=15 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>               
<% support_invmatch("MULTICAST_SUPPORT", "1", "-->"); %>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>
          <p align="right">
                <font face="Arial" style="font-size: 8pt; font-weight: 700"></font></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><table border="0" cellpadding="0" cellspacing="0" id="AutoNumber12" width="300">
                  <tr>
                    <td height="25" width="27"><input type=checkbox value=0 name=_block_loopback <% nvram_match("block_loopback","1","checked"); %>></td>
                    <td height="25" width="240"> <script>Capture(firewall2.natredir)</script>&nbsp; </td>
                  </tr>
                </table>
</TD>
          <TD width=15 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>               
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>
          <p align="right">
                <font face="Arial" style="font-size: 8pt; font-weight: 700">
                </font></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><table border="0" cellpadding="0" cellspacing="0" id="AutoNumber12" width="300">
                  <tr>
                    <td height="25" width="27"><input type=checkbox value=1 name=_ident_pass <% nvram_match("ident_pass","0","checked"); %>></td>
                    <td height="25" width="240"> <script>Capture(firewall2.ident)</script>&nbsp; </td>
                  </tr>
                </table>
</TD>
          <TD width=15 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>               


	<TR>
          <TD bgColor=#e7e7e7></TD>
          <TD background=image/UI_04.gif></TD>
          <TD colSpan=3></TD>
          <TD colspan=2></TD>
          <TD></TD>
          <TD background=image/UI_05.gif></TD>
        </TR>



        </TBODY></TABLE></TD>
    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25>&nbsp;</TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><br>

<b><script>Capture(firewall.firewallpro)</script>: </b><script>Capture(hfirewall.right)</script><br>
          <span ><a target="_blank" href="help/HFirewall.asp"><b><script>Capture(share.more)</script></b></a></span></font></TD>

          <TD width=9 bgColor=#6666cc height=25>&nbsp;
	  </TD></TR></TBODY></TABLE></TD></TR>
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
<script>document.write("<input type=button name=cancel_button" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"Firewall.asp\")>");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>

</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
