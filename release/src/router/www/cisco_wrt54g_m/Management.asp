
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

<HTML><HEAD><TITLE>Management</TITLE>
<% no_cache(); %>
<% charset(); %>
<link rel="stylesheet" type="text/css" href="style.css">
<style fprolloverstyle>
A:hover {color: #00FFFF}
.small A:hover {color: #00FFFF}
</style>
<script src="common.js"></script>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capadmin.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
document.title = adtopmenu.manage;
var EN_DIS1 = '<% nvram_get("remote_management"); %>'	
var wan_proto = '<% nvram_get("wan_proto"); %>'
var snmp_confirm = 0
function PasswdLen(F)
{
	var sLen = F.snmpv3_passwd.value;
	if( sLen.length < 8 )
	{
		F.snmpv3_passwd.value = F.snmpv3_passwd.defaultValue;
		alert("SNMPv3 Password must be at least 8 characters long");
	}
	if( F.snmpv3_passwd.value != F.SnmpPasswdConfirm.value )
	{
		snmp_confirm = 1;
	}
	else 
	{
		snmp_confirm = 0;
	}
}
function ConfirmPasswd(F)
{
	if( F.snmpv3_passwd.value != F.SnmpPasswdConfirm.value )
	{
		snmp_confirm = 1;
	}
}

function SelPort(num,F)
{	
	if(num == 1 && F.PasswdModify.value == 1){
		 if(ChangePasswd(F) == true)
			port_enable_disable(F,num);	
	}
	else
		port_enable_disable(F,num);
}
function port_enable_disable(F,I)
{
	EN_DIS2 = I;
	if ( I == "0" ){
		choose_disable(F.http_wanport);
		choose_disable(F._remote_mgt_https);
	}
	else{
		choose_enable(F.http_wanport);
		choose_enable(F._remote_mgt_https);
	}

	if(F._http_enable.checked == false && F._https_enable.checked == true) {
		choose_disable(F._remote_mgt_https);
		F._remote_mgt_https.checked = true;
	}

	if(F._http_enable.checked == true && F._https_enable.checked == false)
		choose_disable(F._remote_mgt_https);

}
function ChangePasswd(F)
{
	if((F.PasswdModify.value==1 && F.http_passwd.value == "d6nw5v1x2pc7st9m") || F.http_passwd.value == "admin")
	{
//              if(confirm('The Router is currently set to its default password. As a security measure, you must change the password before the Remote Management feature can be enabled. Click the OK button to change your password.  Click the Cancel button to leave the Remote Management feature disabled.'))
                if(confirm(manage2.changpass))
		{
			//window.location.replace('Management.asp');
			F.remote_management[1].checked = true;
			return false;
		}
		else
		{
			F.remote_management[1].checked = true;
			return false;
		}
	}
	else 
		return true;
}
function valid_password(F)
{
	if (F.http_passwd.value != F.http_passwdConfirm.value)
	{	
//              alert("Confirmed password did not match Entered Password.  Please re-enter password");
                alert(manage2.vapass);
		F.http_passwdConfirm.focus();
		F.http_passwdConfirm.select();
		return false;
	}
	return true;
}
function to_submit(F)
{
	if( F.http_passwd.value != F.http_passwdConfirm.value )
		{
//              alert('Password confirmation is not matched.');
                alert(manage2.passnot);
		return false;
		}
	else if( snmp_confirm == 1 )
	{	
		alert('SNMPv3 Password confirmation is not matched.');
//		alert(snmp_confirm);
		snmp_confirm = 0;
		return false;
	}
	else
		F.action.value='Apply';

	valid_password(F);

	if(F.remote_management[0].checked == true){
		if(!ChangePasswd(F))
			return false;
	}

	if(F._remote_mgt_https){
		if(F.http_enable.checked == true && F.https_enable.checked == false)
			F._remote_mgt_https.checked == false;
		if(F.http_enable.checked == false && F.https_enable.checked == true)
			F._remote_mgt_https.checked == true;
		if(F._remote_mgt_https.checked == true) F.remote_mgt_https.value = 1;
		else 	 F.remote_mgt_https.value = 0;
	}
	if(F._http_enable){
		if(F._http_enable.checked == true) F.http_enable.value = 1;
		else 	 F.http_enable.value = 0;
	}
	if(F._https_enable){
		if(F._https_enable.checked == true) F.https_enable.value = 1;
		else 	 F.https_enable.value = 0;
	}

	if(F._http_enable.checked == false && F._https_enable.checked == false) {
//              alert("You must at least select a web server!");
                alert(manage2.selweb);
		return false;
	}

	if(F.upnp_enable[0].checked == true && '<% nvram_get("upnp_enable"); %>' == '0') {
		F.need_reboot.value = "1";
		F.wait_time.value = "10";
	}

	F.submit_button.value = "Management";
	F.submit();
	return true;
}
function handle_https(F)
{
	if(F._https_enable.checked == true && F.remote_management[0].checked == true) {
		choose_enable(F._remote_mgt_https);
	}
	else {
		choose_disable(F._remote_mgt_https);
	}
}
function init() 
{    
	port_enable_disable(document.password, '<% nvram_get("remote_management"); %>');
}        
	
</SCRIPT>

</HEAD>
<BODY vLink=#b5b5e6 aLink=#ffffff link=#b5b5e6 onload=init()>
<DIV align=center>
<FORM name=password method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=action>
<INPUT type=hidden name=PasswdModify value='<% nvram_else_match("http_passwd", "admin", "1", "0"); %>'> 
<input type=hidden name=remote_mgt_https>
<input type=hidden name=http_enable>
<input type=hidden name=https_enable>
<input type=hidden name=wait_time value=4>
<input type=hidden name=need_reboot value=0>
<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95><IMG src="image/UI_Linksys.gif" border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT style="FONT-SIZE: 7pt" color=#ffffff><FONT face=Arial><script>Capture(share.firmwarever)</script>:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></FONT></TD></TR>
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
      style="FONT-SIZE: 15pt" face=Arial 
    color=#ffffff><script>Capture(bmenu.admin)</script></FONT></H3></TD>
    <TD  
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" vAlign=center borderColor=#000000 width=646 bgColor=#000000 height=49><TABLE style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" height=33 cellSpacing=0 cellPadding=0 bgColor=#6666cc border=0>
    <TBODY>
        <TR>
          <TD style="FONT-WEIGHT: bolder; FONT-SIZE: 10pt" align=right bgColor=#6666cc height=33><FONT color=#ffffff><script>productname()</script>&nbsp;&nbsp;</FONT></TD>
          <TD borderColor=#000000 borderColorLight=#000000 align=middle width=109 bgColor=#000000 borderColorDark=#000000 height=12 rowSpan=2><FONT color=#ffffff><SPAN style="FONT-SIZE: 8pt"><B><% get_model_name(); %></B></SPAN></FONT></TD></TR>
        <TR>
          <TD style="FONT-WEIGHT: normal; FONT-SIZE: 1pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=537 bgColor=#000000 height=1></TD></TR>
        <TR>
          <TD width=646 bgColor=#000000 colSpan=2 height=32>
            <TABLE id=AutoNumber1 style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" height=6 cellSpacing=0 cellPadding=0 width=646 border=0>
            <TBODY>
              <TR style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" align=middle bgColor=#6666cc>

<!--
                <TD width=83 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=73 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=113 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=103 height=1><IMG height=10 src="image/UI_06.gif" width=103 border=0></TD>
                <TD width=85 height=1><IMG height=10 src="image/UI_06.gif" width=100 border=0></TD>
                <TD width=115 height=1><IMG height=10 src="image/UI_07.gif" width=115 border=0></TD>
                <TD width=74 height=1><IMG height=10 src="image/UI_06.gif" width=79 border=0></TD>
-->
                <script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_07.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_06.gif></TD>")</script>

              </TR>
              <TR>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><A style="TEXT-DECORATION: none" href="index.asp"><script>Capture(bmenu.setup)</script></A></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Wireless_Basic.asp"><script>Capture(bmenu.wireless)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Firewall.asp"><script>Capture(bmenu.security)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><A style="TEXT-DECORATION: none" href="<% support_elsematch("PARENTAL_CONTROL_SUPPORT", "1", "Parental_Control.asp", "Filters.asp"); %>"><script>Capture(bmenu.accrestriction)</script></A></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Forward.asp"><script>Capture(bmenu.applications)</script> <BR>&amp; <script>Capture(bmenu.gaming)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#6666cc height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><script>Capture(bmenu.admin)</script>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Status_Router.asp"><script>Capture(bmenu.statu)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD></TR>             
              <TR>
                <TD width=643 bgColor=#6666cc colSpan=7 height=21>
                  <TABLE borderColor=black height=21 cellSpacing=0 cellPadding=0 width=643>
                    <TBODY>
                    <TR align=left>

                      <!-- TD width=25></TD -->
                      <script>document.write("<TD width=" + man_width.w1 + "></TD>")</script>  

                      <!-- TD width=80 -->
                      <script>document.write("<TD width=" + man_width.w2 + ">")</script>  
                      <FONT style="COLOR: white"><script>Capture(adtopmenu.manage)</script></FONT></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=20></TD -->
                      <script>document.write("<TD width=" + man_width.w3 + "></TD>")</script>  

                      <!-- TD class=small width=40 -->
                      <script>document.write("<TD class=small width=" + man_width.w4 + ">")</script>  
                      <A href="Log.asp"><script>Capture(adtopmenu.log)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=15></TD -->
                      <script>document.write("<TD width=" + man_width.w5 + "></TD>")</script>  

                      <!-- TD class=small width=75 -->
                      <script>document.write("<TD class=small width=" + man_width.w6 + ">")</script>  
                      <A href="Diagnostics.asp"><script>Capture(adtopmenu.diag)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=15></TD -->
                      <script>document.write("<TD width=" + man_width.w7 + "></TD>")</script>  

                      <!-- TD class=small width=95 -->
                      <script>document.write("<TD class=small width=" + man_width.w8 + ">")</script>  
                      <A href="Factory_Defaults.asp"><script>Capture(adtopmenu.facdef)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=15></TD -->
                      <script>document.write("<TD width=" + man_width.w9 + "></TD>")</script>  

                      <!-- TD class=small width=105 -->
                      <script>document.write("<TD class=small width=" + man_width.w10 + ">")</script>  
                      <A href="Upgrade.asp"><script>Capture(adtopmenu.upgarde)</script></A></TD>
<% support_match("BACKUP_RESTORE_SUPPORT","1","
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=15></TD -->
                      <script>document.write("<TD width=" + man_width.w11 + "></TD>")</script>  

                      <TD class=small>
                      <A href='Backup_Restore.asp'><script>Capture(bakres2.conman)</script></A></TD>"); %>



</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1><IMG height=15 src="image/UI_03.gif" width=164 border=0></TD>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=646 height=1><IMG height=15 src="image/UI_02.gif" width=645 border=0></TD></TR></TBODY></TABLE>
<TABLE id=AutoNumber9 style="BORDER-COLLAPSE: collapse" borderColor=#111111 height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE height=100% cellSpacing=0 cellPadding=0 border=0 width="633">
        <TBODY>        
        <TR>
          <TD width=156 bgColor=#000000 colSpan=3 height=25><P align=right><B><FONT style="FONT-SIZE: 9pt" face=Arial color=#ffffff><script>Capture(adleftmenu.routerpsw)</script></FONT></B></P></TD>
          <TD width=5 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=454 colSpan=6 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <!--TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25><P align=right><FONT style="FONT-WEIGHT: 700">Local Router Access</FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25></TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt"><span >User Name</span>:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><FONT style="FONT-SIZE: 8pt" face=Arial><% nvram_get("http_username"); %></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR-->
        
<% support_invmatch("DDM_SUPPORT", "1", "<!--"); %>
	<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25><P align=right><FONT style="FONT-WEIGHT: 700">Local Router Access</FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;Router Account:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("http_username"); %>' name=http_username onBlur=valid_name(this,"Account",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% support_invmatch("DDM_SUPPORT", "1", "-->"); %>

	<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25><P align=right><FONT style="FONT-WEIGHT: 700">

<!-- % support_invmatch("DDM_SUPPORT", "1", "Local Router Access"); %  -->
<% support_match("DDM_SUPPORT", "1", "<!--"); %>
<script>Capture(adleftmenu.localaccess)</script>
<% support_match("DDM_SUPPORT", "1", "-->"); %>

          </FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;<script>Capture(adleftmenu.routerpsw)</script>:&nbsp;</FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=password maxLength=63 size=20 value="d6nw5v1x2pc7st9m" name="http_passwd" onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;<script>Capture(mgt.reconfirm)</script>:&nbsp;</FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=password maxLength=63 size=20 value="d6nw5v1x2pc7st9m" name=http_passwdConfirm onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=30>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=30>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=30>&nbsp;</TD>
                <TD width=13 height=30>&nbsp;</TD>
                <TD vAlign=top width=410 colSpan=3 height=30><HR color=#b5b5e6 SIZE=1></TD>
                <TD width=15 height=30>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=30>&nbsp;</TD></TR>        
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25><P align=right><FONT style="FONT-WEIGHT: 700"><script>Capture(manage2.webacc)</script></FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(manage2.accser)</script>:&nbsp;</TD>
          <TD width=298 height=25><INPUT type=checkbox value=1 name=_http_enable <% nvram_match("http_enable","1","checked"); %>><b>HTTP</b>&nbsp;&nbsp;&nbsp;<INPUT type=checkbox value=1 name=_https_enable <% nvram_match("https_enable","1","checked"); %> onClick=handle_https(this.form)><b>HTTPS</b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(manage2.wlacc)</script>:&nbsp;</TD>
          <TD width=298 height=25><INPUT type=radio value=0 name=web_wl_filter <% nvram_match("web_wl_filter","0","checked"); %>><b><script>Capture(share.enable)</script></b>&nbsp;&nbsp;&nbsp;<INPUT type=radio value=1 name=web_wl_filter <% nvram_match("web_wl_filter","1","checked"); %>><b><script>Capture(share.disable)</script></b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=30>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=30>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=30>&nbsp;</TD>
                <TD width=13 height=30>&nbsp;</TD>
                <TD vAlign=top width=410 colSpan=3 height=30><HR color=#b5b5e6 SIZE=1></TD>
                <TD width=15 height=30>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=30>&nbsp;</TD></TR>        
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25><p align="right"><FONT style="FONT-WEIGHT: 700"><span ><script>Capture(adleftmenu.remoteaccess)</script></span></FONT></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;<script>Capture(mgt.remotemgt)</script>:</SPAN></TD>
          <TD width=298 height=25><INPUT type=radio value=1 name=remote_management <% nvram_match("remote_management","1","checked"); %> OnClick=SelPort(1,this.form)><b><script>Capture(share.enable)</script></b>&nbsp;&nbsp;&nbsp;<INPUT type=radio value=0 name=remote_management <% nvram_match("remote_management","0","checked"); %> OnClick=SelPort(0,this.form)><b><script>Capture(share.disable)</script></b></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;<script>Capture(mgt.mgtport)</script>:&nbsp;</SPAN></TD>
          <TD width=298 height=25>&nbsp;&nbsp;<INPUT class=num maxLength=5 size=5 value='<% nvram_get("http_wanport"); %>' onBlur='valid_range(this,1,65535,"Port%20number")' name="http_wanport"></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% support_invmatch("HTTPS_SUPPORT", "1", "<!--"); %>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(mgt.https)</script>:&nbsp;</TD>
          <TD width=298 height=25>&nbsp;<INPUT type=checkbox value=1 name=_remote_mgt_https <% nvram_match("remote_mgt_https","1","checked"); %>></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% support_invmatch("HTTPS_SUPPORT", "1", "-->"); %>
<% support_invmatch("SNMP_SUPPORT", "1", "<!--"); %>
        
        <TR>
          <TD width=156 bgColor=#000000 colSpan=3 height=25><P align=right><B><FONT style="FONT-SIZE: 9pt" color=#ffffff>SNMP</FONT></B></P></TD>
          <TD width=5 bgColor=#000000 height=25>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=25>&nbsp;</TD>
                <TD width=12 height=25>&nbsp;</TD>
                <TD style="BORDER-TOP: 1px solid; BORDER-LEFT-WIDTH: 1px; BORDER-BOTTOM-WIDTH: 1px; BORDER-RIGHT-WIDTH: 1px" borderColor=#b5b5e6 width=411 height=25>&nbsp;</TD>
                <TD width=15 height=25>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
         <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25><P align=right><FONT style="FONT-WEIGHT: 700">Identification</FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;Contact:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("snmp_contact"); %>' name="snmp_contact" onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;Device Name:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("router_name"); %>' name=router_name onBlur=valid_name(this,"router_name")></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR> 
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;Location:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("snmp_location"); %>' name=snmp_location onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=30>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=30>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=30>&nbsp;</TD>
                <TD width=13 height=30>&nbsp;</TD>
                <TD vAlign=top width=410 colSpan=3 height=30><HR color=#b5b5e6 SIZE=1></TD>
                <TD width=15 height=30>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=30>&nbsp;</TD></TR>       
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;Get Community:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("snmp_getcom"); %>' name="snmp_getcom" onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;Set Community:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("snmp_setcom"); %>' name=snmp_setcom onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>   
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;SNMP Trusted Host:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("snmp_trust"); %>' name=snmp_trust onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;SNMP Trap - Community:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("trap_com"); %>' name=trap_com onBlur=valid_name(this,"Password",SPACE_NO)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR> 
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;SNMP Trap - Destination:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;<b><% prefix_ip_get("lan_ipaddr",1); %></b></SPAN><INPUT type=num maxLength=3 size=3 value='<% nvram_get("trap_dst"); %>' name=trap_dst onBlur=valid_range(this,1,254,"IP")></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>


        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;SNMPv3 UserName:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=text maxLength=63 size=20 value='<% nvram_get("snmpv3_username"); %>' name=snmpv3_username ></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR> 


        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;SNMPv3 Password:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=password maxLength=63 size=20 value="d6nw5v1x2pc7st9m" name=snmpv3_passwd onBlur=PasswdLen(this.form)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR> 


        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt">&nbsp;Re-enter to confirm:&nbsp;&nbsp; </FONT></TD>
          <TD width=298 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;</SPAN><INPUT type=password maxLength=63 size=20 value="d6nw5v1x2pc7st9m" name=SnmpPasswdConfirm onBlur=PasswdLen(this.form)></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR> 



<% support_invmatch("SNMP_SUPPORT", "1", "-->"); %>


        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=30>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=30>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=30>&nbsp;</TD>
                <TD width=13 height=30>&nbsp;</TD>
                <TD vAlign=top width=410 colSpan=3 height=30><HR color=#b5b5e6 SIZE=1></TD>
                <TD width=15 height=30>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=30>&nbsp;</TD></TR>        


        <TR>
          <TD width=156 bgColor=#000000 colSpan=3 height=25><P align=right><B><FONT style="FONT-SIZE: 9pt" color=#ffffff>UPnP</FONT></B></P></TD>
          <TD width=5 bgColor=#000000 height=25>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=25>&nbsp;</TD>
                <TD width=12 height=25>&nbsp;</TD>
                <TD width=411 height=25>&nbsp;</TD>
                <TD width=15 height=25>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25 width="42">&nbsp;</TD>
          <TD width=101 height=25><SPAN  style="FONT-SIZE: 8pt">&nbsp;UPnP:&nbsp;</SPAN></TD>
          <TD width=298 height=25>
            <TABLE id=AutoNumber12 cellSpacing=0 cellPadding=0 width=242 border=0>
              <TBODY>
              <TR>
                <TD width=242 height=25><INPUT type=radio value=1 name=upnp_enable <% nvram_match("upnp_enable","1","checked"); %>><b><script>Capture(share.enable)</script></b>&nbsp;&nbsp;&nbsp;<INPUT type=radio value=0 name=upnp_enable <% nvram_match("upnp_enable","0","checked"); %>><b><script>Capture(share.disable)</script></b></TD>
                </TR></TBODY></TABLE></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>               
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=1>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=13 height=1>&nbsp;</TD>
                <TD width=410 colSpan=3 height=1></TD>
                <TD width=15>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=1>&nbsp;</TD>
          <TD colSpan=6 width="454">
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=13 height=1>&nbsp;</TD>
                <TD width=410 colSpan=3 height=1></TD>
                <TD width=15>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=42 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=62 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=52 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=12></TD>
          <TD width=17></TD>
          <TD width=13></TD>
          <TD width=101></TD>
          <TD width=298></TD>
          <TD width=13></TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR></TBODY></TABLE></TD>
    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25></TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><span style="font-family: Arial"><br>
<script>Capture(hmanage2.right1)</script><br><br>
<script>Capture(hmanage2.right2)</script><br>
<b><a target="_blank" href="help/HManagement.asp"><script>Capture(share.more)</script></a></b><br><br>
<script>Capture(hmanage2.right3)</script><br><br>
<script>Capture(hmanage2.right4)</script><br>
<b><a target="_blank" href="help/HManagement.asp"><script>Capture(share.more)</script></a></b></span></font></TD>
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
          <TD width=176 bgColor=#6666cc height=58 rowSpan=2>
          <IMG src="image/UI_Cisco.gif" border=0 width="176" height="64"></TD></TR>
        <TR>
          <TD width=156 bgColor=#000000>&nbsp;</TD>
          <TD width=8 bgColor=#000000>&nbsp;</TD>
          <TD width=454 bgColor=#6666cc>
		<p align="right">
<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;
<script>document.write("<input type=button name=cancel_button" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"Management.asp\")>");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>

</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
