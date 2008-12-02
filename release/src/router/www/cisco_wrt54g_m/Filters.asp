
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

<html><head><title>Internet Access</title>
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

<Script Language="JavaScript">
document.title = secleftmenu.interaccess;
<% filter_init(); %>
var summary_win = null;
var ipmac_win = null;
function dayall(F){
	if(F.day_all.checked == false)
		I = 1;
	else
		I = 0;
	day_enable_disable(F,I);
}
function day_enable_disable(F,I){
        if(I == 1){
		choose_enable(F.week0);
		choose_enable(F.week1);
		choose_enable(F.week2);
		choose_enable(F.week3);
		choose_enable(F.week4);
		choose_enable(F.week5);
		choose_enable(F.week6);
	}
	else if(I == 0){
		choose_disable(F.week0);
		choose_disable(F.week1);
		choose_disable(F.week2);
		choose_disable(F.week3);
		choose_disable(F.week4);
		choose_disable(F.week5);
		choose_disable(F.week6);
	}
}
function timeall(F,I){
	time_enable_disable(F,I);
}
function time_enable_disable(F,I){
	if(I == 1){
		choose_enable(F.start_hour);
		choose_enable(F.start_min);
		choose_enable(F.start_time);
		choose_enable(F.end_hour);
		choose_enable(F.end_min);
		choose_enable(F.end_time);
	}
	else if(I == 0){
		choose_disable(F.start_hour);
		choose_disable(F.start_min);
		choose_disable(F.start_time);
		choose_disable(F.end_hour);
		choose_disable(F.end_min);
		choose_disable(F.end_time);
	}
}
function ViewSummary(){
	summary_win = self.open('FilterSummary.asp','FilterSummary','alwaysRaised,resizable,scrollbars,width=700,height=420');
	summary_win.focus();
}
function ViewFilter(){
	ipmac_win = self.open('FilterIPMAC.asp','FilterTable','alwaysRaised,resizable,scrollbars,width=670,height=500');
	ipmac_win.focus();
}
function to_submit(F)
{
	if(valid(F) == true){
		F.submit_type.value = "save";
  	     	F.submit_button.value = "Filters";
       		F.action.value = "Apply";
       		F.submit();
	}
}
function to_save(F)
{
	if(valid(F) == true){
      	 	F.submit_button.value = "Filters";
       		F.change_action.value = "gozila_cgi";
		F.submit_type.value = "save";
       		F.action.value = "Apply";
       		F.submit();
	}
}
function to_delete(F)
{
//      if(confirm("Delete the Policy?")){
        if(confirm(hfilter2.delpolicy)){
	   	F.submit_button.value = "Filters";
     		F.change_action.value = "gozila_cgi";
		F.submit_type.value = "delete";
	       	F.action.value = "Apply";
       		F.submit();
	}
}
function valid(F)
{
	if(F.day_all.checked == false && F.week0.checked == false && F.week1.checked == false && F.week2.checked == false && F.week3.checked == false && F.week4.checked == false && F.week5.checked == false && F.week6.checked == false){
//              alert("You must at least select a day.");
                alert(errmsg.err3);
		return false;
	}
	if(F.time_all[1].checked == true){
		start = (parseInt(F.start_time.value, 10)*12 + parseInt(F.start_hour.value, 10)) * 60 + parseInt(F.start_min.value, 10);
		end = (parseInt(F.end_time.value, 10)*12 + parseInt(F.end_hour.value, 10)) * 60 + parseInt(F.end_min.value, 10);
		if(end <= start){
//                      alert("The end time must be bigger than start time!");
                        alert(errmsg.err4);
			return false;
		}
	}
	if(F.f_status1[1].checked == true)	// Disable
		F.f_status.value = "0";
	else {					// Enable
		if(F.f_status2[1].checked == true)	// Allow
			F.f_status.value = "2";
		else					// deny
			F.f_status.value = "1";	
	}
	return true;
}
function isascii1(I,M)
{
	for(i=0 ; i<I.value.length; i++){
		ch = I.value.charAt(i);
		if((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_'){
			//
		}
		else {
			alert(errmsg.err14+" [A - Z , a - z , 0 - 9 , - , _ ]");
			I.value = I.defaultValue;	
			return false;
		}
	}
	return true;
}
function valid_name1(I,M,flag)
{
	isascii1(I,M);

	var bbb = I.value.replace(/^\s*/,"");
        var ccc = bbb.replace(/\s*$/,"");

        I.value = ccc;

	if(flag & SPACE_NO){
		check_space(I,M);
	}

}
function SelFilter(num,F)
{
        F.submit_button.value = "Filters";
        F.change_action.value = "gozila_cgi";
        F.f_id.value=F.f_id.options[num].value;
        F.submit();
}
function exit()
{
	//closeWin(summary_win);
	closeWin(ipmac_win);
}
function init() 
{               
	day_enable_disable(document.filters,'<% filter_tod_get("day_all_init"); %>');
	time_enable_disable(document.filters,'<% filter_tod_get("time_all_init"); %>');
	setBlockedServicesValue();
	Status(document.filters, '<% filter_policy_get("f_status","onload_status"); %>');
	choose_disable(document.filters.port0_start);
	choose_disable(document.filters.port0_end);
	choose_disable(document.filters.port1_start);
	choose_disable(document.filters.port1_end);
}

function service(id, name, port_start, port_end, protocol){
	this.id = id;
	this.name = name;
	this.start = port_start;
	this.end = port_end;
	this.protocol = protocol;
	this.deleted = false;
	this.modified = false;
}

services=new Array();
services_length=0;
/* Init. services data structure */
<% filter_port_services_get("all_list", "0"); %>

servport_name0 = "<% filter_port_services_get("service", "0"); %>";
servport_name1 = "<% filter_port_services_get("service", "1"); %>";
function search_service_index(name){
 var i;
        for(i=0 ; i<services_length ; i++){
                if(name == services[i].name){
                        return i;
                }
        }
        return -1;
}
function setBlockedServicesValue(){
var index;
	/* for service port 0 */
	index = search_service_index(servport_name0);
	if(index!=-1){
		document.filters.port0_start.value = services[index].start;
		document.filters.port0_end.value = services[index].end;
		document.filters.blocked_service0.selectedIndex = index+1; /* first will be none */
	}

	/* for service port 1 */
	index = search_service_index(servport_name1);
	if(index!=-1){
		document.filters.port1_start.value = services[index].start;
		document.filters.port1_end.value = services[index].end;
		document.filters.blocked_service1.selectedIndex = index+1; /* first will be none */
	}
}

function onChange_blockedServices(index, start, end){
	index--
	if(index == -1){
		start.value = '';
		end.value = '';
	}
	else{
		start.value = services[index].start;
		end.value = services[index].end;
	}
}
function Status(F,I){
	var start = '';
        var end = '';
        var total = F.elements.length;
        for(i=0 ; i < total ; i++){
                if(F.elements[i].name == "blocked_service0")  start = i;
                if(F.elements[i].name == "url5")  end = i;
        }
        if(start == '' || end == '')    return true;

        if(I == "deny" ) {
                for(i = start; i<=end ;i++)
                        choose_disable(F.elements[i]);
        }
        else {
                for(i = start; i<=end ;i++)
                        choose_enable(F.elements[i]);		
		choose_disable(document.filters.port0_start);
		choose_disable(document.filters.port0_end);
		choose_disable(document.filters.port1_start);
		choose_disable(document.filters.port1_end);
	}
}

</script>

</head>

<BODY onunload=exit() onload=init()>
<DIV align=center>
<FORM name=filters method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=submit_type>
<input type=hidden name=action>
<input type=hidden name=blocked_service>
<input type=hidden name=filter_web>
<input type=hidden name=filter_policy>
<input type=hidden name=f_status>

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
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#000000 align=middle width=163 height=49>
      <H3 style="MARGIN-TOP: 1px; MARGIN-BOTTOM: 1px"><FONT 
      style="FONT-SIZE: 15pt" face=Arial color=#ffffff><script>Capture(filter.accessrestric)</script></FONT></H3></TD>
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
                <TD width=113 height=1><IMG height=10 src="image/UI_06.gif" width=83 border=0></TD>
                <TD width=103 height=1><IMG height=10 src="image/UI_07.gif" width=103 border=0></TD>
                <TD width=85 height=1><IMG height=10 src="image/UI_06.gif" width=100 border=0></TD>
                <TD width=115 height=1><IMG height=10 src="image/UI_06.gif" width=115 border=0></TD>
                <TD width=74 height=1><IMG height=10 src="image/UI_06.gif" width=79 border=0></TD>
-->
                <script>document.write("<TD width=" + ui_06.w1 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w2 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w3 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w4 + " height=8 background=image/UI_07.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w5 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w6 + " height=8 background=image/UI_06.gif></TD>")</script>
                <script>document.write("<TD width=" + ui_06.w7 + " height=8 background=image/UI_06.gif></TD>")</script>

              </TR>
              <TR>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><A style="TEXT-DECORATION: none" href="index.asp"><script>Capture(bmenu.setup)</script></A></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Wireless_Basic.asp"><script>Capture(bmenu.wireless)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Firewall.asp"><script>Capture(bmenu.security)</script></a></FONT></TD>
                <TD align=middle bgColor=#6666cc height=20><FONT style="FONT-WEIGHT: 700" color=#ffffff><script>Capture(bmenu.accrestriction)</script></a></FONT></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Forward.asp"><script>Capture(bmenu.applications)</script> <BR>&amp; <script>Capture(bmenu.gaming)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Management.asp"><script>Capture(bmenu.admin)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
                <TD align=middle bgColor=#000000 height=20><P style="MARGIN-BOTTOM: 4px"><FONT style="FONT-WEIGHT: 700" color=#ffffff><a style="TEXT-DECORATION: none" href="Status_Router.asp"><script>Capture(bmenu.statu)</script></a>&nbsp;&nbsp;&nbsp;&nbsp;</FONT></P></TD>
              </TR>
            <tr>
              <td width="643" style="border-style: none; border-width: medium; font-style:normal; font-variant:normal; font-weight:normal; font-size:10pt; font-family:Arial, Helvetica, sans-serif; color:black" bgcolor="#6666CC" height="21" colspan="7">
              <table border="0" cellpadding="0" cellspacing="0" style="border-collapse: collapse" bordercolor="#111111" id="AutoNumber12" width="643" height="21">
                <TR align=left>


		      <!-- TD width=20></TD -->
                      <script>document.write("<TD width=" + pc_width.w1 + "></TD>")</script> 	

<% support_match("PARENTAL_CONTROL_SUPPORT", "1", "
                      <!-- TD class=small width=100 -->
                      <script>document.write('<TD class=small width=' + pc_width.w2 + '>')</script>  
                      <A href='Parental_Control.asp'><script>Capture(pactrl.pactrl)</script></A></TD>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>
                      <!-- TD width=25></TD -->
                      <script>document.write('<TD width=' + pc_width.w3 + '></TD>')</script>
                      "); %>

                      <!-- TD class=small width=90 -->
                      <script>document.write("<TD class=small width=" + pc_width.w4 + ">")</script>
                      <FONT style="COLOR: white"><script>Capture(secleftmenu.interaccess)</script></FONT></TD>

                      <TD>&nbsp;</TD>

                      <!-- TD width=22></TD -->
                      <script>document.write("<TD width=" + pc_width.w5 + "></TD>")</script>

                      <!-- TD class=small width=153></TD -->
                      <script>document.write("<TD class=small width=" + pc_width.w6 + "></TD>")</script>

                      <TD><P class=bar>&nbsp;</P></TD>

                      <!-- TD width=30></TD -->
                      <script>document.write("<TD width=" + pc_width.w7 + "></TD>")</script>

                      <!-- TD class=small width=140></TD -->
                      <script>document.write("<TD class=small width=" + pc_width.w8 + "></TD>")</script>


                    </TR>
              </table>
              </td>
            </tr>
          </table>
          </TD></TR></TBODY></TABLE></TD></TR>
  </TABLE>

   <TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>  
  <TR bgColor=black>
    <TD width=163 height=1 style="font-style: normal; font-variant: normal; font-weight: normal; font-size: 10pt; font-family: Arial, Helvetica, sans-serif; color: black" bgcolor="#E7E7E7" bordercolor="#E7E7E7">
			<img border="0" src="image/UI_03.gif" width="164" height="15"></TD>
    <TD width=646 height=1 style="font-style: normal; font-variant: normal; font-weight: normal; font-size: 10pt; font-family: Arial, Helvetica, sans-serif; color: black" bgcolor="#FFFFFF">
			<img border="0" src="image/UI_02.gif" width="645" height="15"></TD></TR>
  </TABLE>


  <table border="0" cellpadding="0" cellspacing="0" style="border-collapse: collapse" bordercolor="#111111" id="AutoNumber9" width="809" height="23">
    <tr><td width=633><table height=100% border="0" cellpadding="0" cellspacing="0">           
              <tr>
                <td width="156" height="25" bgcolor="#000000" colspan="3">
                <p align="right"><b>
                <font face="Arial" color="#FFFFFF" style="font-size: 9pt">
                <script>Capture(secleftmenu.interaccess)</script></font></b></td>
                <td width="8" height="25" bgcolor="#000000">&nbsp;</td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="101" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="296" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
              	<td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td> 
              	<td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="13">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101">
                <font style="font-size: 8pt"><script>Capture(share.policy)</script>:&nbsp;</font>
                </td>
                <td height="25" bgcolor="#FFFFFF" width="296">
      &nbsp;<SELECT name=f_id onChange=SelFilter(this.form.f_id.selectedIndex,this.form)> 
<% filter_policy_select(); %>
    </SELECT>&nbsp;

<script>document.write("<input type=button name=delete_button" + " value=\"" + sbutton.del + "\" onClick=to_delete(this.form)>");</script>&nbsp;
<script>document.write("<input type=button name=summary_button" + " value=\"" + share.summary + "\" onClick=ViewSummary()>");</script>&nbsp;

                </td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
                <td width="156" height="1" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>  
                <td width="8" height="1" background=image/UI_04.gif></td>
                <td colspan=6><table><tr>
                <td width="16" height="1" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="1" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="410" height="1" bgcolor="#FFFFFF" colspan="3">
                <hr color="#B5B5E6" size="1"></td>
                <td width="15" height="1" bgcolor="#FFFFFF">&nbsp;</td>
                </tr></table></td>
                <td width="15" height="1" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
              	<td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td> 
              	<td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="13">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101">
                <font style="font-size: 8pt"><script>Capture(bmenu.statu)</script>:&nbsp;</font>
                </td>
                <td height="25" bgcolor="#FFFFFF" width="296">
		<INPUT type=radio value=enable name=f_status1 <% filter_policy_get("f_status","enable"); %>><B><script>Capture(share.enable)</script></B>&nbsp;
		<INPUT type=radio value=disable name=f_status1 <% filter_policy_get("f_status","disable"); %>><B><script>Capture(share.disable)</script></B></td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
              	<td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td> 
              	<td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="13">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101"><script>Capture(filter.policyname)</script>:&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="296">&nbsp;<INPUT maxLength=30 size=22 name=f_name value='<% filter_policy_get("f_name",""); %>' onBlur=valid_name1(this,"")></td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <!--tr>
              	<td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td> 
              	<td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="13">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101">
                <font style="font-size: 8pt">Policy Type:</font>
                </td>
                <td height="25" bgcolor="#FFFFFF" width="296">
      &nbsp;<SELECT onchange=connectionType(this.value) name=Encapsulatoin style="font-family:Arial; font-size:9pt"> 
      		<OPTION value=0 >Internet Access</OPTION>
      		<OPTION value=1 >Inbound Traffic</OPTION>
    </SELECT>&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr-->
              
              
              
              <tr>
              	<td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>  
              	<td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="13">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101"><script>Capture(filter.pcs)</script>:&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="296">

<script>document.write("<input type=button name=vfilter_button" + " value=\"" + secbutton.editlist + "\" onClick=ViewFilter()>");</script>
                </td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="101" height="25" bgcolor="#FFFFFF">
                <input type=radio <% filter_policy_get("f_status","deny"); %> name=f_status2 value=deny OnClick=Status(this.form,"deny")><script>Capture(filter.deny)</script> <br>
                <input type=radio <% filter_policy_get("f_status","allow"); %> name=f_status2 value=allow OnClick=Status(this.form,"allow")><script>Capture(filter.allow)</script> </td>
                <td width="296" height="25" bgcolor="#FFFFFF"><script>Capture(filter.dayandhr)</script></td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="410" height="25" bgcolor="#FFFFFF" colspan="3" align="left"> 
                <b><font face="Arial" style="font-size: 8pt"><script>Capture(filter.days)</script></font></b></td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr> 
              
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101">
                <input type=checkbox value=1 name=day_all <% filter_tod_get("day_all"); %> onClick=dayall(this.form)>
                <span style="font-size: 8pt"><script>Capture(filter.everyday)</script></span><font style="font-size: 8pt">&nbsp;&nbsp;
                </font></td>
                <td height="25" bgcolor="#FFFFFF" width="296">
                	<table><tr>
                	<td width=70><INPUT type=checkbox value=1 name=week0 <% filter_tod_get("week0"); %>>&nbsp;&nbsp;<script>Capture(filter.sun)</script>&nbsp;</td>
                	<td width=70><INPUT type=checkbox value=1 name=week1 <% filter_tod_get("week1"); %>>&nbsp;&nbsp;<script>Capture(filter.mon)</script>&nbsp;</td>
                	<td width=70><INPUT type=checkbox value=1 name=week2 <% filter_tod_get("week2"); %>>&nbsp;&nbsp;<script>Capture(filter.tue)</script>&nbsp;</td>
                	<td width=70><INPUT type=checkbox value=1 name=week3 <% filter_tod_get("week3"); %>>&nbsp;&nbsp;<script>Capture(filter.wed)</script>&nbsp;</td>
                	</tr></table>
                </td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101">
                <span style="font-size: 8pt"></span><font style="font-size: 8pt">&nbsp;&nbsp;
                </font></td>
                <td height="25" bgcolor="#FFFFFF" width="296">
                	<table><tr>
                	<td width=70><INPUT type=checkbox value=1 name=week4 <% filter_tod_get("week4"); %>>&nbsp;&nbsp;<script>Capture(filter.thu)</script>&nbsp;</td>
                	<td width=70><INPUT type=checkbox value=1 name=week5 <% filter_tod_get("week5"); %>>&nbsp;&nbsp;<script>Capture(filter.fri)</script>&nbsp;</td>
                	<td width=70><INPUT type=checkbox value=1 name=week6 <% filter_tod_get("week6"); %>>&nbsp;&nbsp;<script>Capture(filter.sat)</script>&nbsp;</td>
                	</tr></table>
                </td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              
              
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="410" height="25" bgcolor="#FFFFFF" colspan="3" align="left"> 
                <b><font face="Arial" style="font-size: 8pt"><script>Capture(filter.times)</script></font></b></td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr> 
              
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101">
                <input type=radio value=1 name=time_all <% filter_tod_get("time_all_en"); %> onClick=timeall(this.form,'0')>
                <span style="font-size: 8pt"><script>Capture(filter.tfhrs)</script></span><font style="font-size: 8pt">&nbsp;&nbsp;
                </font></td>
                <td height="25" bgcolor="#FFFFFF" width="296" align=left>
                	<table><tr>
                	<td width="20">
                		<INPUT type=radio value=0 name=time_all <% filter_tod_get("time_all_dis"); %> onClick=timeall(this.form,'1')>
                	</td>
                	<td width="35">
                		<span style="font-size: 8pt">&nbsp;<script>Capture(filter.from)</script>:</span>&nbsp;
                	</td>
                	<td>
                <select name="start_hour" style="font-family:Arial; font-size:9pt">
<% filter_tod_get("start_hour_12"); %>
                </select>
                	</td>
                	<td style="font-family:Arial; font-size:9pt">
                :	
                	</td>
                	<td>
                <select name="start_min" style="font-family:Arial; font-size:9pt">
<% filter_tod_get("start_min_5"); %>
                </select>
                &nbsp;&nbsp;
                	</td>
                	<td>
                <select name="start_time" style="font-family:Arial; font-size:9pt">
                	<option value=0 <% filter_tod_get("start_time_am"); %>><script>Capture(filter.am)</script></option>
                	<option value=1 <% filter_tod_get("start_time_pm"); %>><script>Capture(filter.pm)</script></option>
                </select>
                	</td>
                	</tr></table>
                </td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td height="25" bgcolor="#FFFFFF" width="101">
                <span style="font-size: 8pt"></span><font style="font-size: 8pt">&nbsp;&nbsp;
                </font></td>
                <td height="25" bgcolor="#FFFFFF" width="296" align=left>
                	<table><tr>
                	<td width="20">
                		<input type=hidden name="allday">
                	</td>
                	<td width="35">
                		<span style="font-size: 8pt">&nbsp;<script>Capture(filter.to)</script>:</span>&nbsp;
                	</td>
                	<td>
                <select name="end_hour" style="font-family:Arial; font-size:9pt">
<% filter_tod_get("end_hour_12"); %>
                </select>
                	</td>
                	<td style="font-family:Arial; font-size:9pt">
                :	
                	</td>
                	<td>
                <select name="end_min" style="font-family:Arial; font-size:9pt">
<% filter_tod_get("end_min_5"); %>
                </select>
                &nbsp;&nbsp;
                	</td>
                	<td>
                <select name="end_time" style="font-family:Arial; font-size:9pt">
                	<option value=0 <% filter_tod_get("end_time_am"); %>><script>Capture(filter.am)</script></option>
                	<option value=1 <% filter_tod_get("end_time_pm"); %>><script>Capture(filter.pm)</script></option>
                </select>
                	</td>
                	</tr></table>
                </td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>
              
              <tr>
                <td width="156" height="1" bgcolor="#E7E7E7" colspan=3>&nbsp;</td>  
                <td width="8" height="1" background=image/UI_04.gif></td>
                <td colspan=6><table><tr>
                <td width="16" height="1" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="1" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="410" height="1" bgcolor="#FFFFFF" colspan="3">
                <hr color="#B5B5E6" size="1"></td>
                <td width="15" height="1" bgcolor="#FFFFFF">&nbsp;</td>
                </tr></table></td>
                <td width="15" height="1" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>

              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan="3"> <p align="right"><font style="font-weight: 700"><script>Capture(secleftmenu.blocksrv)</script></font></td>   
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="428" height="25" bgcolor="#FFFFFF" colspan=2>&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>  
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan="3">&nbsp;</td>   
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="428" height="25" bgcolor="#FFFFFF" colspan=2>
		<SELECT size=1 name=blocked_service0 onChange="onChange_blockedServices(blocked_service0.selectedIndex, port0_start, port0_end)"> 
			<option value=None selected><script>Capture(wlanadv.none)</script></option>
                                        <script>
                                                var i, index;
                                                index = search_service_index(servport_name0);
                                                for(i=0 ; i<services_length ; i++){
                                                        document.write("<option value="+services[i].name);
                                                        if(i==index)
                                                                document.write(" selected");
                                                        document.write(">"+services[i].name+"</option>");
                                                }                       
                                        </script>
		</SELECT>&nbsp;&nbsp; 
		<input maxLength=5 size=5 name=port0_start class=num readonly>&nbsp; ~ &nbsp;
		<input maxLength=5 size=5 name=port0_end class=num readonly></td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>  
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan="3">&nbsp;</td>   
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="428" height="25" bgcolor="#FFFFFF" colspan=2>
		<SELECT size=1 name=blocked_service1 onChange="onChange_blockedServices(blocked_service1.selectedIndex, port1_start, port1_end)"> 
			<option value=None selected><script>Capture(wlanadv.none)</script></option>
                                        <script>
                                                var i, index;
                                                index = search_service_index(servport_name1);
                                                for(i=0 ; i<services_length ; i++){
                                                        document.write("<option value="+services[i].name);
                                                        if(i==index)
                                                                document.write(" selected");
                                                        document.write(">"+services[i].name+"</option>");
                                                }                       
                                        </script>
		</SELECT>&nbsp;&nbsp; 
		<input maxLength=5 size=5 name=port1_start class=num readonly>&nbsp; ~ &nbsp;
		<input maxLength=5 size=5 name=port1_end class=num readonly></td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>  
              <tr>
                <td width="156" height="25" bgcolor="#E7E7E7" colspan="3">&nbsp;</td>   
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="14" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="17" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="428" height="25" bgcolor="#FFFFFF" colspan=2>

<script>document.write("<INPUT onclick=\"self.open(\'Port_Services.asp\',\'Port_Services\',\'alwaysRised,resizable,scrollbars,width=630,height=360\').focus()\" type=button name=\"add_button\" " + " value=\"" + secbutton.addedit + "\">");</script>

                <td width="13" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
              </tr>  
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 height=1 background=image/UI_04.gif></TD>
          <TD colSpan=6>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 bgColor=#ffffff height=1>&nbsp;</TD>
                <TD width=13 bgColor=#ffffff height=1>&nbsp;</TD>
                <TD width=410 bgColor=#ffffff colSpan=3 height=1>
                  <HR color=#b5b5e6 SIZE=1>
                </TD>
                <TD width=15 bgColor=#ffffff height=1>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 bgColor=#ffffff height=1 background=image/UI_05.gif></TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>
            <P align=right><FONT style="FONT-WEIGHT: 700"><script>Capture(filter.websiteblock)</script></FONT></P></TD>
          <TD width=8 height=25 background=image/UI_04.gif></TD>
          <TD width=14 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=17 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=428 bgColor=#ffffff colSpan=2 height=25>
		<INPUT class=num size=30 maxlength=79 name=host0 value='<% filter_web_get("host","0"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"URL")>&nbsp; 
		<INPUT class=num size=30 maxlength=79 name=host1 value='<% filter_web_get("host","1"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"URL")></TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=15 bgColor=#ffffff height=25 background=image/UI_05.gif></TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 height=25 background=image/UI_04.gif></TD>
          <TD width=14 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=17 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=428 bgColor=#ffffff colSpan=2 height=25>
		<INPUT class=num size=30 maxlength=79 name=host2 value='<% filter_web_get("host","2"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"URL")>&nbsp; 
		<INPUT class=num size=30 maxlength=79 name=host3 value='<% filter_web_get("host","3"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"URL")></TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=15 bgColor=#ffffff height=25 background=image/UI_05.gif></TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=1>&nbsp;</TD>
          <TD width=8 height=1 background=image/UI_04.gif></TD>
          <TD colSpan=6>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 bgColor=#ffffff height=1>&nbsp;</TD>
                <TD width=13 bgColor=#ffffff height=1>&nbsp;</TD>
                <TD width=410 bgColor=#ffffff colSpan=3 height=1>
                  <HR color=#b5b5e6 SIZE=1>
                </TD>
                <TD width=15 bgColor=#ffffff height=1>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 bgColor=#ffffff height=1 background=image/UI_05.gif></TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>
            <P align=right><FONT style="FONT-WEIGHT: 700"><script>Capture(filter.blockkeyword)</script></FONT></P></TD>
          <TD width=8 height=25 background=image/UI_04.gif></TD>
          <TD width=14 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=17 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=428 bgColor=#ffffff colSpan=2 height=25>
		<INPUT class=num size=18 maxlength=79 name=url0 value='<% filter_web_get("url","0"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"Keyword")>&nbsp; 
		<INPUT class=num size=18 maxlength=79 name=url1 value='<% filter_web_get("url","1"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"Keyword")>&nbsp; 
		<INPUT class=num size=18 maxlength=79 name=url2 value='<% filter_web_get("url","2"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"Keyword")></TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=15 bgColor=#ffffff height=25 background=image/UI_05.gif></TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 height=25 background=image/UI_04.gif></TD>
          <TD width=14 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=17 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=428 bgColor=#ffffff colSpan=2 height=25>
		<INPUT class=num size=18 maxlength=79 name=url3 value='<% filter_web_get("url","3"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"Keyword")>&nbsp; 
		<INPUT class=num size=18 maxlength=79 name=url4 value='<% filter_web_get("url","4"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"Keyword")>&nbsp; 
		<INPUT class=num size=18 maxlength=79 name=url5 value='<% filter_web_get("url","5"); %>' style="FONT-SIZE: 9pt; FONT-FAMILY: Arial" onBlur=valid_name(this,"Keyword")></TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=15 bgColor=#ffffff height=25 background=image/UI_05.gif></TD></TR>

              <tr>
                <td width="44" bgcolor="#E7E7E7"></td>
                <td width="65" bgcolor="#E7E7E7"></td>
                <td width="47" bgcolor="#E7E7E7"></td>
                <td width="8" background=image/UI_04.gif></td>
                <td width="14" bgcolor="#FFFFFF"></td>
                <td width="17" bgcolor="#FFFFFF"></td>
                <td width="13" bgcolor="#FFFFFF"></td>
                <td width="101" bgcolor="#FFFFFF"></td>
                <td width="296" bgcolor="#FFFFFF"></td>
                <td width="13" bgcolor="#FFFFFF"></td>
                <td width="15" background=image/UI_05.gif></td>
              </tr>
              </table></td>
              
              <td width="176" valign=top bgcolor=#6666CC><table border="0" cellpadding="0" cellspacing="0" width="176"><tr>
                <td width="11" height="25" bgcolor="#6666CC">&nbsp;</td>
                <td width="156" height="25" bgcolor="#6666CC"><font color="#FFFFFF"><span style="font-family: Arial"><br>
<script>Capture(hfilter2.right1)</script><br><br>
<script>Capture(hfilter2.right2)</script><br><br>
<script>Capture(hfilter2.right3)</script><br><br>
<b><a target="_blank" href="help/HFilters.asp"><script>Capture(share.more)</script></a></b><br><br>
<script>Capture(hfilter2.right5)</script><br><br>
<script>Capture(hfilter2.right6)</script><br><br>
<script>Capture(hfilter2.right7)</script><br><br>
<script>Capture(hfilter2.right8)</script><br><br>
<script>Capture(hfilter2.right9)</script><br><br>
</span></font></td>
                <td width="9" height="25" bgcolor="#6666CC">&nbsp;</td>
              </tr></table></td>
              
              </tr>

	      <tr>
                <td width="809" colspan=2><table border="0" cellpadding="0" cellspacing="0">
		<tr> 
		<td width="156" height="25" bgcolor="#E7E7E7">&nbsp;</td>
                <td width="8" height="25" background=image/UI_04.gif></td>
                <td width="454" height="25" bgcolor="#FFFFFF">&nbsp;</td>
                <td width="15" height="25" bgcolor="#FFFFFF" background=image/UI_05.gif></td>
                <td width="176" bgcolor="#6666CC" rowspan="2">
                <img border="0" src="image/UI_Cisco.gif" width="176" height="64"></td>
		</tr>
		<tr>
		<td width="156" height="33" bgcolor="#000000">&nbsp;</td>
                <td width="8" height="33" bgcolor="#000000">&nbsp;</td>
                <td width="454" height="33" bgcolor="#6666CC">
				<p align="right">
<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;
<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"Filters.asp\")>");</script>&nbsp;&nbsp;
                </td>
                <td width="15" height="33" bgcolor="#000000">&nbsp;</td>
		</tr>
		</table></td>
              </tr>
              </table>
    		</form></div>
            </body>

</html>
