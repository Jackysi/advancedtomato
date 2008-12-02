
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

<HTML><HEAD><TITLE>DDNS</TITLE>
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capapp.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capsetup.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/ddns.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
document.title = share.ddns;
function check_email(email_url)
{
	var email_parttern = /^\w+(\.\w+)?@([A-Za-z0-9]+\.)+[A-Za-z0-9]{2,3}$/;
	if(email_url.value != "" && !email_parttern.test(email_url.value))
	{
		alert(errmsg.erremail);
		email_url.value = email_url.defaultValue;
		return false;
	}
	return true;
}
function ddns_check(F,T)
{
	if(F.ddns_enable.value == 0)
		return true;
	else if(F.ddns_enable.value == 1){
		username = eval("F.ddns_username");
		passwd = eval("F.ddns_passwd");
		hostname = eval("F.ddns_hostname");
	}
	else {
		username = eval("F.ddns_username_"+F.ddns_enable.value);
		passwd = eval("F.ddns_passwd_"+F.ddns_enable.value);
		hostname = eval("F.ddns_hostname_"+F.ddns_enable.value);
	}

	if(username.value == ""){
//              alert("You must input a username!");
		if(F.ddns_enable.value != 2)
		{
                alert(errmsg.err0);
		}
		else
		{
			alert(errmsg.err60);
		}
		username.focus();
		return false;
	}
	else 
	{
		if(F.ddns_enable.value == 2)
		{
			var email_parttern = /^\w+(\.\w+)?@([A-Za-z0-9]+\.)+[A-Za-z0-9]{2,3}$/;
			if(!email_parttern.test(username.value))
			{
				alert(errmsg.erremail);
		username.focus();
		return false;
	}
		}
	}
	if(passwd.value == ""){
//              alert("You must input a passwd!");
		if(F.ddns_enable.value != 2)
		{
			alert(errmsg.err6);
		}
		else
		{
			alert(errmsg.errkey);
		}
		passwd.focus();
		return false;
	}
	else
	{
		if(F.ddns_enable.value == 2 && passwd.value.length < 16)
		{
			alert(errmsg.errkeylen);
			passwd.focus();
			return false;
		}
	}
	if(F.ddns_enable.value != 4){
		if(hostname.value == ""){
//              	alert("You must input a hostname!");
			if(F.ddns_enable.value == 1)
			{
				alert(errmsg.err7);
			}
			else
			{
				alert(errmsg.errdomain);
			}
			hostname.focus();
			return false;
		}
		else
		{
			if(F.ddns_enable.value == 2)
            		{
				var parttern = /^[\w]+\.[\w]+(\.[\w]+(\.[\w]+)?)?$/;
				if(!parttern.test(hostname.value))
				{
					alert(errmsg.errdomainformat);
					hostname.focus();
					return false;
				}
			}
		}	
	}
	return true;
}
function to_save(F)
{
	if(ddns_check(F,"update") == true){
		F.change_action.value = "gozila_cgi";
		F.submit_button.value = "DDNS";
		F.submit_type.value = "save";
       		F.action.value = "Apply";
       		F.submit();
	}
}
function to_submit(F)
{
	if(ddns_check(F,"save") == true){
		F.submit_button.value = "DDNS";
      		F.action.value = "Apply";
		F.submit();
	}
}
function SelDDNS(num,F)
{
        F.submit_button.value = "DDNS";
        F.change_action.value = "gozila_cgi";
        F.ddns_enable.value=F.ddns_enable.options[num].value;
        F.submit();
}
function show_status()
{
        var RetMsg="<% show_ddns_status(); %>";
        if( RetMsg=="  " || RetMsg.length < 2)
                return;
        else
                Capture(<% show_ddns_status(); %>);
}
</SCRIPT>
</HEAD>
<BODY>
<DIV align=center>
<FORM name=ddns method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=action>
<input type=hidden name=change_action>
<input type=hidden name=submit_type>
<input type=hidden name=wait_time value=6>
<TABLE cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=95>
    <IMG src="image/UI_Linksys.gif" 
      border=0 width="165" height="57"></TD>
    <TD vAlign=bottom align=right width=714 bgColor=#6666cc><FONT 
      style="FONT-SIZE: 7pt" color=#ffffff><FONT face=Arial><script>Capture(share.firmwarever)</script>:&nbsp;<% get_firmware_version(); %>&nbsp;&nbsp;&nbsp;</FONT></FONT></TD></TR>
  <TR>
    <TD width=808 bgColor=#6666cc colSpan=2>
    <IMG height=11 src="image/UI_10.gif" width=809 border=0></TD></TR></TBODY></TABLE>
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
            rowSpan=2><FONT color=#ffffff><SPAN style="FONT-SIZE: 8pt"><B><% get_model_name(); %></B></SPAN></FONT></TD></TR>
        <TR>
          <TD 
          style="FONT-WEIGHT: normal; FONT-SIZE: 1pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=537 bgColor=#000000 height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=646 bgColor=#000000 colSpan=2 height=32>
            <TABLE id=AutoNumber1 style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; BORDER-COLLAPSE: collapse; FONT-VARIANT: normal" height=6 cellSpacing=0 cellPadding=0 width=646 border=0>
              <TBODY>
              <TR style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" align=middle bgColor=#6666cc>

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
                <TD align=middle bgColor=#6666cc height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <A style="TEXT-DECORATION: none" href="index.asp"><script>Capture(bmenu.setup)</script></A></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="Wireless_Basic.asp"><script>Capture(bmenu.wireless)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff>
                <a style="TEXT-DECORATION: none" href="Firewall.asp"><script>Capture(bmenu.security)</script></a></FONT></TD>
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

                      <!-- TD width=100 -->
                      <script>document.write("<TD width=" + set_width.w4 + ">")</script>  
                      <FONT style="COLOR: white"><script>Capture(share.ddns)</script></FONT></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=22></TD -->
                      <script>document.write("<TD width=" + set_width.w5 + "></TD>")</script>  

                      <!-- TD class=small width=153 --> 
                      <script>document.write("<TD class=small width=" + set_width.w6 + ">")</script>  
                      <A href="WanMAC.asp"><script>Capture(topmenu.macaddrclone)</script></A></TD>

                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

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
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1>
    <IMG height=15 src="image/UI_03.gif" width=164 border=0></TD>
    <TD style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" width=646 height=1><IMG height=15 src="image/UI_02.gif" width=645 border=0></TD></TR></TBODY></TABLE>
<TABLE id=AutoNumber9 style="BORDER-COLLAPSE: collapse" borderColor=#111111 height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE height=100% cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD width=156 bgColor=#000000 colSpan=3 height=25>
            <P align=right><B><FONT style="FONT-SIZE: 9pt" face=Arial color=#ffffff><script>Capture(share.ddns)</script></FONT></B></P></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD width=17 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(ddns.srv)</script>:&nbsp;</TD>
          <TD width=296 height=25><SELECT onChange=SelDDNS(this.form.ddns_enable.selectedIndex,this.form) name="ddns_enable"> 
	<script>
		var flag;
		var lang = '<% nvram_get("language"); %>';
		var ddns_enable = '<% nvram_selget("ddns_enable"); %>';
		var ddns3322 = '0';
		var peanuthull = '0';
<% support_invmatch("DDNS3322_SUPPORT", "1", "//"); %> ddns3322 = '1';
<% support_invmatch("PEANUTHULL_SUPPORT", "1", "//"); %> peanuthull = '1';

		if(ddns_enable == '0')
			flag = '0';
		if (ddns3322 == '1' && peanuthull == '1') {
			if(lang == "EN" || lang == "SC") {
				flag = "34";
			}
			else {
				flag = "12";
			}
		}
		else
			flag = "12";

		
		if(ddns_enable == "0") {
			document.write("<OPTION value=0 selected>"+share.disable+"<b></b></OPTION>");
		}
		else
			document.write("<OPTION value=0>"+share.disable+"<b></b></OPTION>");

		if(flag == "12") {
			if(ddns_enable == '1')
				document.write("<OPTION value=1 selected>"+ddns.dyndns+"<b></b></OPTION>");
			else
				document.write("<OPTION value=1>"+ddns.dyndns+"<b></b></OPTION>");
			if(ddns_enable == '2')
				document.write("<OPTION value=2 selected>"+ddns.tzo+"<b></b></OPTION>");
			else
				document.write("<OPTION value=2>"+ddns.tzo+"<b></b></OPTION>");
		}
		else if(flag == "34") {
			if(ddns_enable == '3')
				document.write("<OPTION value=3 selected>"+ddns.ddns3322+"<b></b></OPTION>");
			else
				document.write("<OPTION value=3>"+ddns.ddns3322+"<b></b></OPTION>");
			if(ddns_enable == '4')
				document.write("<OPTION value=4 selected>"+ddns.peanuthull+"<b></b></OPTION>");
			else
				document.write("<OPTION value=4>"+ddns.peanuthull+"<b></b></OPTION>");
		}
	</script>

	</SELECT>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD colSpan=4><HR color=#b5b5e6 SIZE=1></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

        <% show_ddns_setting(); %>               

<% nvram_else_selmatch("ddns_enable","0","","<!--"); %>

        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
		<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
    	<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
  
<% nvram_else_selmatch("ddns_enable","0","","-->"); %>
          
        <% nvram_selmatch("ddns_enable","0","<!--"); %>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.interipaddr)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial><% show_ddns_ip(); %></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(bmenu.statu)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT color=red><script>show_status();</script></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <% nvram_selmatch("ddns_enable","0","-->"); %>

        <TR>
          <TD width=44 bgColor=#e7e7e7>&nbsp;</TD>
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
<script>Capture(hddns2.right1)</script><br>
<b><a target="_blank" href="help/HDDNS.asp"><script>Capture(share.more)</script></a></b>
</span></font>
</TD>
          <TD width=9 bgColor=#6666cc height=25>&nbsp;</TD></TR>
<% nvram_else_selmatch("ddns_enable","2","","<!--"); %>
        <TR>
          <TD width=167 colspan=2 bgColor=#6666cc height=25><font color="#FFFFFF"><span ><br>
<li><script>Capture(hddns2.right2)</script></li><br><br>
<li><script>Capture(hddns2.right3)</script></li><br><br>
<li><script>Capture(hddns2.right4)</script></li><br><br>
<li><script>Capture(hddns2.right5)</script></li><br><br>
</span></font>
</TD>
          <TD width=9 bgColor=#6666cc height=25>&nbsp;</TD></TR>
<% nvram_else_selmatch("ddns_enable","2","","-->"); %>


</TBODY></TABLE></TD></TR>
  <TR>
    <TD width=809 colSpan=2>
      <TABLE cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD width=156 bgColor=#e7e7e height=30>&nbsp;</TD>
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

<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"DDNS.asp\")>");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>

</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
