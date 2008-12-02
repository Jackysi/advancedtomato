
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

<HTML><HEAD><TITLE>Firmware Upgrade</TITLE>
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

<SCRIPT language=javascript>
document.title = adtopmenu.upgarde;
var showchar = '|';
var maxchars = 60;
var delay_time = 1000;
var counter=0;
var num=0;
function progress(){
	var F = document.forms[0];
	if(num == 2){
		clearTimeout(timerID);
//              alert("Upgrade are failed!");
                alert(errmsg.err13);
		return false;
	}
	if (counter < maxchars)	{
		counter++;
		var tmp = '';
		for (var i=0; i < counter; i++)
			tmp = tmp + showchar;
		F.process.value = tmp;
		timerID = setTimeout('progress()',delay_time);
	}
	else{
		counter = 0;
		num ++;
		progress();	
        }
}

function stop(){
	if(ie4)
  		document.all['style0'].style.visibility = 'hidden';
}

function upgrade(F){
	var len = F.file.value.length;
	var ext = new Array('.','b','i','n');
	if (F.file.value == '')	{
//              alert('Please select a file to upgrade !');
                alert(fwupgrade.upgradefile);
		return false;
	}
	var IMAGE = F.file.value.toLowerCase();
	for (i=0; i < 4; i++)	{
		if (ext[i] != IMAGE.charAt(len-4+i)){
//                      alert('Incorrect image file!');
                        alert(hupgrade.wrimage);
			return false;
		}
	}

	if(ns4)
		delay_time = 1500;
   		
	choose_disable(F.Upgrade_b);

	F.submit_button.value = "Upgrade";
   	F.submit();
   	//document.onstop = stop;
   	progress();
}
</script>
</HEAD>
<BODY>
<DIV align=center>
<FORM name="firmware" method=post action=upgrade.cgi encType=multipart/form-data>
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
      style="FONT-SIZE: 15pt" face=Arial 
    color=#ffffff><script>Capture(bmenu.admin)</script></FONT></H3></TD>
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
              style="BORDER-RIGHT: medium none; BORDER-TOP: medium none; FONT-WEIGHT: normal; FONT-SIZE: 1pt; BORDER-LEFT: medium none; COLOR: black; BORDER-BOTTOM: medium none; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
              align=middle bgColor=#6666cc>

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
                <TD align=middle bgColor=#6666cc height=20>
                  <P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" 
                  color=#ffffff><script>Capture(bmenu.admin)</script>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
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

                      <!-- TD width=25></TD -->
                      <script>document.write("<TD width=" + man_width.w1 + "></TD>")</script>

                      <!-- TD class=small width=80 -->
                      <script>document.write("<TD class=small width=" + man_width.w2 + ">")</script>
                      <A href="Management.asp"><script>Capture(adtopmenu.manage)</script></A></TD>

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

                      <!-- TD width=105 -->
                      <script>document.write("<TD width=" + man_width.w10 + ">")</script>
                      <FONT style="COLOR: white"><script>Capture(adtopmenu.upgarde)</script></FONT></TD>
<% support_match("BACKUP_RESTORE_SUPPORT","1","
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>

                      <!-- TD width=15></TD -->
                      <script>document.write("<TD width=" + man_width.w11 + "></TD>")</script>

                      <TD class=small><A href='Backup_Restore.asp'><script>Capture(bakres2.conman)</script></A></TD>"); %>
                    </TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=809 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1><IMG height=15 
      src="image/UI_03.gif" width=163 border=0></TD>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    width=646 height=1><IMG height=15 src="image/UI_02.gif" width=646 
      border=0></TD></TR></TBODY></TABLE>
<TABLE height=23 cellSpacing=0 cellPadding=0 width=809 border=0>
  <TBODY>
  <TR>
    <TD width=633>
      <TABLE height=100% cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD align=right width=156 bgColor=#000000 
            height=25><B><FONT style="FONT-SIZE: 9pt" face=Arial 
            color=#ffffff><script>Capture(adleftmenu.upgradefw)</script></FONT></B></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=402 height=25>&nbsp;</TD>
          <TD width=39 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=402 height=25>
          <p align="center"><FONT face=Arial color=#0000ff><B><SPAN style="FONT-SIZE: 16pt"><script>Capture(adtopmenu.upgarde)</script></SPAN></B></FONT></TD>
          <TD width=39 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=402 align=right height=25>&nbsp;<script>Capture(fwupgrade.upgradefile)</script>:&nbsp;&nbsp;<INPUT type=file name=file size="20"></TD>
          <TD width=39 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=402 align=right height=25>
          <p align="center"><FONT face=Arial color=#ff0000><script>Capture(fwupgrade.warning)</script></FONT></TD>
          <TD width=39 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=402>&nbsp;</TD>
          <TD width=39 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <CENTER>
          <TD width=13 height=25>&nbsp;</TD>
          <TD align=middle height=25 width=402>&nbsp;<INPUT type=text name=process size=40 maxlength=120 value='' onFocus=blur()></UBR></TD>
          <TD height=25 width=39>&nbsp;</TD></CENTER>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=402 align=middle height=25><B><FONT color=red>
                 <script>Capture(fwupgrade.notinterrupted)</script></FONT></B>
          </TD>
          <TD width=39 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
	
	<TR>
          <TD width=156 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=13>&nbsp;</TD>
          <TD width=402></TD>
          <TD width=39>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR>


	</TBODY></TABLE></TD>

    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25>&nbsp;</TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF"><span style="font-family: Arial"><br>

<script>Capture(hupgrade.right1)</script><br><br>
<script>Capture(hupgrade.right2)</script><br>
<b><a target="_blank" href="help/HUpgrade.asp"><script>Capture(share.more)</script></a></b></span></font></TD>
          <TD width=9 bgColor=#6666cc 
  height=25>&nbsp;</TD></TR></TBODY></TABLE></TD></TR>
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
          <TD width=454 bgColor=#6666cc>
            <p align="right">

<script>document.write("<INPUT onclick=upgrade(this.form) type=button name=Upgrade_b value=\"" + adbutton.upgrade + "\">");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>
</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
