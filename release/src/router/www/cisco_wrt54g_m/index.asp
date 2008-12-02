
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

<HTML><HEAD><TITLE>Basic Setup</TITLE>
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
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/timezone.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=JavaScript>
document.title = topmenu.basicsetup;
var EN_DIS2 = '<% nvram_get("mtu_enable"); %>';	
var wan_proto = '<% nvram_get("wan_proto"); %>';
var dhcp_win = null;

function valid_mtu(I)
{
	var min = '<% get_mtu("min"); %>';
	var max = '<% get_mtu("max"); %>';
	valid_range(I,min,max,"MTU");
	d = parseInt(I.value, 10);
	if(d > max) {
		I.value = max ;
	}
	if(d < min) {
		I.value = max ;
	}
}
function SelMTU(num,F)
{
	mtu_enable_disable(F,num);
}
function mtu_enable_disable(F,I)
{
	EN_DIS1 = I;
	if ( I == "0" ){
		choose_disable(F.wan_mtu);
	}
	else{
		choose_enable(F.wan_mtu);
	}
}


function SelWAN(num,F)
{
        F.submit_button.value = "index";
        F.change_action.value = "gozila_cgi";
        F.wan_proto.value=F.wan_proto.options[num].value;
        F.submit();
}

function SelPPP(num,F)
{
        F.submit_button.value = "index";
        F.change_action.value = "gozila_cgi";
        F.mpppoe_enable.value = F.mpppoe_enable.options[num].value;
        F.submit();
}

function to_submit(F)
{
	if(valid_value(F) && valid_lan_ip(F) ){
		if(F._daylight_time.checked == false)
			F.daylight_time.value = 0;
		else
			F.daylight_time.value = 1;

		F.submit_button.value = "index";
		F.action.value = "Apply";
        	F.submit();
	}
}
function valid_value(F)
{
	if(F.now_proto.value == "pptp" || F.now_proto.value == "static"){
		if(!valid_ip(F,"F.wan_ipaddr","IP",ZERO_NO|MASK_NO))
                        return false;

		if(!valid_mask(F,"F.wan_netmask",ZERO_NO|BCST_NO))
			return false;

		if(F.now_proto.value == "static"){
			if(!valid_ip(F,"F.wan_gateway","Gateway",ZERO_NO|MASK_NO))
              		  	return false;
			//if(!valid_ip(F,"F.wan_dns0","DNS",MASK_NO))
			//	return false;
			//if(!valid_ip(F,"F.wan_dns1","DNS",MASK_NO))
			//	return false;
			//if(!valid_ip(F,"F.wan_dns2","DNS",MASK_NO))
			//	return false;
			if(!valid_ip_gw(F,"F.wan_ipaddr","F.wan_netmask","F.wan_gateway"))
				return false;
		}

		if(F.now_proto.value == "pptp"){
			if(!valid_ip(F,"F.pptp_server_ip","Gateway",ZERO_NO|MASK_NO))
                		return false;
			if(!valid_ip_gw(F,"F.wan_ipaddr","F.wan_netmask","F.pptp_server_ip"))
				return false;
		}

	}
	if(F.now_proto.value == "pppoe" || F.now_proto.value == "pptp" || F.now_proto.value == "l2tp" || F.now_proto.value == "heartbeat"){
		if(F.ppp_username.value == ""){
//                      alert("You must input a username!");
                        alert(errmsg.err0);
			F.ppp_username.focus();
			return false;
		}
		if(F.ppp_passwd.value == ""){
//                      alert("You must input a passwd!");
                        alert(errmsg.err6);
			F.ppp_passwd.focus();
			return false;
		}
	}
	
	if(!valid_dhcp_server(F))
		return false;

	if(F.router_name.value == ""){
//              alert("You must input a Router Name!");
                alert(errmsg.err1);
                F.router_name.focus();
                return false;
	}	

	return true;
}
function valid_hb(I,M)
{
	if(I.value == "0.0.0.0" || I.value == "255.255.255.255") {
//		alert("The Telstra Cable Server IP Address is invalid!");
//              alert("The HeartBeat Server IP Address is invalid!");
                alert(errmsg2.err0);
		I.value = I.defaultValue;
		return false;
	}
	return valid_name(I,M);
}

function valid_dhcp_server(F)
{
	if(F.lan_proto.selectedIndex == 0)
                return true;

        a1 = parseInt(F.dhcp_start.value,10);
        a2 = parseInt(F.dhcp_num.value,10);
        if(a1 + a2 > 255){
//                alert("Out of range, please adjust start IP address or user's numbers.");
                alert(errmsg.err2);
                return false;
        }       
	  if(F.lan_ipaddr_3.value == F.dhcp_start.value)
	  {
		alert(errmsg.err75);
                return false;
	  }

        if(!valid_ip(F,"F.wan_dns0","DNS",MASK_NO))
                return false;
        if(!valid_ip(F,"F.wan_dns1","DNS",MASK_NO))
                return false;
        if(!valid_ip(F,"F.wan_dns2","DNS",MASK_NO))
                return false;
        if(!valid_ip(F,"F.wan_wins","WINS",MASK_NO))
                return false;

	return true;
}
function SelDHCP(T,F)
{
	dhcp_enable_disable(F,T);
}

function dhcp_enable_disable(F,T)
{
	var start = '';
	var end = '';
 	var total = F.elements.length;
	for(i=0 ; i < total ; i++){
		if(F.elements[i].name == "dhcp_start")	start = i;
		if(F.elements[i].name == "wan_wins_3")	end = i;
	}
	if(start == '' || end == '')	return true;

	if( T == "static" ) {
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
function SelTime(num,f)
{
	var str = f.time_zone.options[num].value;
	var Arr = new Array();
	Arr = str.split(' ');
	aaa = Arr[2];
	daylight_enable_disable(f,aaa);
}

function ppp_enable_disable(F,I)
{
	if( I == "0"){
		choose_disable(F.ppp_idletime);
		choose_enable(F.ppp_redialperiod);
	}
	else{
		choose_enable(F.ppp_idletime);
		choose_disable(F.ppp_redialperiod);
	}
}
function daylight_enable_disable(F,aaa)
{
	if(aaa == 0){
                F._daylight_time.checked = false;
                choose_disable(F._daylight_time);
                F.daylight_time.value = 0;
        }
        else{
                choose_enable(F._daylight_time);                
                F._daylight_time.checked = true;
                F.daylight_time.value = 1;
        }

}
function init()
{
	mtu_enable_disable(document.setup,'<% nvram_get("mtu_enable"); %>');
	var str = document.setup.time_zone.options[document.setup.time_zone.selectedIndex].value;
	var Arr = new Array();
	Arr = str.split(' ');
	aaa = Arr[2];
	if(aaa == 0){
                document.setup._daylight_time.checked = false;
                choose_disable(document.setup._daylight_time);
                document.setup.daylight_time.value = 0;
        }

	if(document.setup.now_proto.value == "pppoe" || document.setup.now_proto.value == "pptp" || document.setup.now_proto.value == "l2tp" || document.setup.now_proto.value == "heartbeat")
		ppp_enable_disable(document.setup,'<% nvram_get("ppp_demand"); %>');

	dhcp_enable_disable(document.setup,'<% nvram_get("lan_proto"); %>');

	var max_mtu = <% get_mtu("max"); %>;
	if(document.setup.wan_mtu.value > max_mtu || document.setup.mtu_enable.value == '0') 
	{
		document.setup.wan_mtu.value = max_mtu;
	}

}

function valid_lan_ip(F)
{
		M1 = "value is out of range";
		var mask = new Array(4);
		var ip = new Array(4);
		var netid = new Array(4);
		var brcastip = new Array(4);
		for(i=0,j=0;i<4;i++,j=j+4)
		{
			ip[i]=eval("F.lan_ipaddr_"+i).value;
			mask[i]=F.lan_netmask.value.substring(j,j+3);
			netid[i]=eval(ip[i]&mask[i]);
			if(i<3)
				brcastip[i]=netid[i];
			else
				brcastip[i]=eval(netid[i]+255-mask[i]);	
		}
		startip = eval(netid[3]+1);
		endip = eval(brcastip[3]-1);
		if( ip[0] == netid[0] && ip[1] == netid[1] && ip[2] == netid[2] && ip[3] == netid[3])
		{
			alert(M1+" ["+startip+"-"+endip+"]");
			F.lan_ipaddr_0.value = F.lan_ipaddr_0.defaultValue;
			F.lan_ipaddr_1.value = F.lan_ipaddr_1.defaultValue;
			F.lan_ipaddr_2.value = F.lan_ipaddr_2.defaultValue;
			F.lan_ipaddr_3.value = F.lan_ipaddr_3.defaultValue;
			return false;
		}	
		
		if( ip[0] == brcastip[0] && ip[1] == brcastip[1] && ip[2] == brcastip[2] && ip[3] == brcastip[3])
		{
			alert(M1+" ["+startip+"-"+endip+"]");
			F.lan_ipaddr_0.value = F.lan_ipaddr_0.defaultValue;
			F.lan_ipaddr_1.value = F.lan_ipaddr_1.defaultValue;
			F.lan_ipaddr_2.value = F.lan_ipaddr_2.defaultValue;
			F.lan_ipaddr_3.value = F.lan_ipaddr_3.defaultValue;
			return false;
		}	
		if( (F.lan_ipaddr_0.value != F.lan_ipaddr_0.defaultValue) || (F.lan_ipaddr_1.value != F.lan_ipaddr_1.defaultValue) || (F.lan_ipaddr_2.value != F.lan_ipaddr_2.defaultValue) || (F.lan_ipaddr_3.value != F.lan_ipaddr_3.defaultValue) )
		{
			//F.router2gateway.value="1";
			F.wait_time.value="21";
			F.need_reboot.value="1";
		}
		return true;
}
</SCRIPT>
</HEAD>
<BODY onload=init()>
<DIV align=center>
<FORM name=setup method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=submit_type>
<input type=hidden name=action>
<input type=hidden name=now_proto value='<% nvram_gozila_get("wan_proto"); %>'>
<input type=hidden name=daylight_time value=0>
<input type=hidden name="lan_ipaddr" value="4">
<input type=hidden name="wait_time" value="0">
<input type=hidden name="need_reboot" value="0">

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
    <IMG height=11 
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
                      <FONT style="COLOR: white"><script>Capture(topmenu.basicsetup)</script></FONT></TD>

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

                      <!-- TD class=small width=140 -->
                      <script>document.write("<TD class=small width=" + set_width.w8 + ">")</script>  
                      <A href="Routing.asp"><script>Capture(topmenu.advrouting)</script></TD>

<% support_invmatch("HSIAB_SUPPORT", "1", "<!--"); %>
                      <TD width=1 align=center><P class=bar><font color='white'><b>|</b></font></P></TD>
                      <TD width=30></TD>
                      <TD class=small width=140><A href="HotSpot_Admin.asp">Hot Spot</TD>
<% support_invmatch("HSIAB_SUPPORT", "1", "-->"); %>

		      <TD>&nbsp;</TD>		
	
                    </TR>
                    </TBODY>
                  </TABLE>
                </TD>
              </TR></TBODY></TABLE></TD></TR></TBODY></TABLE></TD></TR></TBODY></TABLE>
<TABLE height=5 cellSpacing=0 cellPadding=0 width=806 bgColor=black border=0>
  <TBODY>
  <TR bgColor=black>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    borderColor=#e7e7e7 width=163 bgColor=#e7e7e7 height=1>
    <IMG height=15 src="image/UI_03.gif" width=164 border=0></TD>
    <TD 
    style="FONT-WEIGHT: normal; FONT-SIZE: 10pt; COLOR: black; FONT-STYLE: normal; FONT-FAMILY: Arial, Helvetica, sans-serif; FONT-VARIANT: normal" 
    width=646 height=1>
    <IMG height=15 src="image/UI_02.gif" width=645 border=0></TD></TR></TBODY></TABLE>
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
            color=#ffffff><script>Capture(lefemenu.intersetup)</script></FONT></B></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD width=17 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=1><B><script>Capture(lefemenu.conntype)</script></B></TD>
          <TD width=8 background=image/UI_04.gif height=1>&nbsp;</TD>
          <TD colSpan=6>
            <TABLE>
              <TBODY>
              <TR>
                <TD width=16 height=1>&nbsp;</TD>
                <TD width=28 height=1>&nbsp;</TD>               
                <TD width=396 colSpan=3 height=1><SELECT name="wan_proto" onChange=SelWAN(this.form.wan_proto.selectedIndex,this.form)>
	                 <OPTION value="dhcp" <% nvram_selmatch("wan_proto", "dhcp", "selected"); %>><script>Capture(setupcontent.dhcp)</script></OPTION> 
                     <OPTION value="static" <% nvram_selmatch("wan_proto", "static", "selected"); %>><script>Capture(share.staticip)</script></OPTION>
	                 <OPTION value="pppoe" <% nvram_selmatch("wan_proto", "pppoe", "selected"); %>><script>Capture(share.pppoe)</script></OPTION> 
                     <OPTION value="pptp" <% nvram_selmatch("wan_proto", "pptp", "selected"); %>><script>Capture(share.pptp)</script></OPTION>
<% support_invmatch("L2TP_SUPPORT", "1", "<!--"); %>
                     <OPTION value="l2tp" <% nvram_selmatch("wan_proto", "l2tp", "selected"); %>><script>Capture(hstatrouter2.l2tp)</script></OPTION>
<% support_invmatch("L2TP_SUPPORT", "1", "-->"); %>
<% support_invmatch("HEARTBEAT_SUPPORT", "1", "<!--"); %>
                     <OPTION value="heartbeat" <% nvram_selmatch("wan_proto", "heartbeat", "selected"); %>><script>Capture(hindex2.telstra)</script></OPTION>
<% support_invmatch("HEARTBEAT_SUPPORT", "1", "-->"); %>
                     </SELECT></TD>
                <TD>&nbsp;</TD></TR></TBODY></TABLE></TD>
                <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>                        
        
     
        <% show_index_setting(); %>
        
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
                <TD height=1>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>
            <P align=right><FONT style="FONT-WEIGHT: 700"><B><script>Capture(lefemenu.optset)</script></B><BR><B><script>Capture(lefemenu.requireisp)</script></B></FONT></P></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.routename)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial><INPUT maxLength=39 name="router_name" size="20" value='<% nvram_get("router_name"); %>' onBlur=valid_name(this,"Router%20Name")></FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.hostname)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial><INPUT maxLength=39 name="wan_hostname" size="20" value='<% nvram_get("wan_hostname"); %>' onBlur=valid_name(this,"Host%20Name")></FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.domainname)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial><INPUT maxLength=63 name="wan_domain" size="20" value='<% nvram_get("wan_domain"); %>' onBlur=valid_name(this,"Domain%20name",SPACE_NO)></FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.mtu)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><B><select name="mtu_enable" onChange=SelMTU(this.form.mtu_enable.selectedIndex,this.form)>
			<option value="0" <% nvram_match("mtu_enable", "0", "selected"); %>><b><script>Capture(share.auto)</script></b></option>
			<option value="1" <% nvram_match("mtu_enable", "1", "selected"); %>><b><script>Capture(share.mtumanual)</script>&nbsp;</b></option>
		</select></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;</FONT><script>Capture(share.mtusize)</script>:&nbsp;</B></TD>
          <TD width=296 height=25><INPUT class=num maxLength=4 onBlur=valid_mtu(this) size=5 value='<% nvram_get("wan_mtu"); %>' name="wan_mtu"></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <!--TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;Speed &amp; Duplex:</FONT></TD>
          <TD width=296 height=25><B><select name="wan_speed")>
			<option value="0" <% nvram_match("wan_speed", "0", "selected"); %>><b>10 Mb Full</b></option>
			<option value="1" <% nvram_match("wan_speed", "1", "selected"); %>><b>10 Mb Half</b></option>
			<option value="2" <% nvram_match("wan_speed", "2", "selected"); %>><b>100 Mb Full</b></option>
			<option value="3" <% nvram_match("wan_speed", "3", "selected"); %>><b>100 Mb Half</b></option>
			<option value="4" <% nvram_match("wan_speed", "4", "selected"); %>><b>Auto</b></option>
		</select></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR-->
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
                <TD height=1>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#000000 colSpan=3 
            height=25><B><FONT style="FONT-SIZE: 9pt" face=Arial 
            color=#ffffff><script>Capture(lefemenu.netsetup)</script></FONT></B></TD>
          <TD width=8 bgColor=#000000 height=25>&nbsp;</TD>
          <TD width=14 height=25>&nbsp;</TD>
          <TD width=17 height=25>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=25><B><script>Capture(lefemenu.routerip)</script></B></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(setupcontent.localipaddr)</script>:&nbsp;</TD>
          <TD width=296 height=25><INPUT class=num maxLength=3 onBlur=valid_range(this,1,223,"IP") size=3 value='<% get_single_ip("lan_ipaddr","0"); %>' name="lan_ipaddr_0"> . 
            <INPUT class=num maxLength=3 onBlur=valid_range(this,0,255,"IP") size=3 value='<% get_single_ip("lan_ipaddr","1"); %>' name="lan_ipaddr_1"> . 
            <INPUT class=num maxLength=3 onBlur=valid_range(this,0,255,"IP") size=3 value='<% get_single_ip("lan_ipaddr","2"); %>' name="lan_ipaddr_2"> . 
            <INPUT class=num maxLength=3 onBlur=valid_range(this,1,254,"IP") size=3 value='<% get_single_ip("lan_ipaddr","3"); %>' name="lan_ipaddr_3"></TD>  
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(share.submask)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><SELECT class=num size=1 name="lan_netmask"><OPTION 
              value=255.255.255.0 <% nvram_match("lan_netmask", "255.255.255.0", "selected"); %>>255.255.255.0</OPTION><OPTION 
              value=255.255.255.128 <% nvram_match("lan_netmask", "255.255.255.128", "selected"); %>>255.255.255.128</OPTION><OPTION 
              value=255.255.255.192 <% nvram_match("lan_netmask", "255.255.255.192", "selected"); %>>255.255.255.192</OPTION><OPTION 
              value=255.255.255.224 <% nvram_match("lan_netmask", "255.255.255.224", "selected"); %>>255.255.255.224</OPTION><OPTION 
              value=255.255.255.240 <% nvram_match("lan_netmask", "255.255.255.240", "selected"); %>>255.255.255.240</OPTION><OPTION 
              value=255.255.255.248 <% nvram_match("lan_netmask", "255.255.255.248", "selected"); %>>255.255.255.248</OPTION><OPTION 
              value=255.255.255.252 <% nvram_match("lan_netmask", "255.255.255.252", "selected"); %>>255.255.255.252</OPTION></SELECT></TD>
          <TD height=25>&nbsp;</TD>
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
                <TD width=410 colSpan=3 height=1><HR color=#b5b5e6 SIZE=1></TD>
                <TD height=1>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=25><B><script>Capture(lefemenu.netaddr)</script></B><BR><B><script>Capture(lefemenu.dhcpserverset)</script></B></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(share.dhcpsrv)</script>:&nbsp;</TD>
          <TD width=296 height=25><input type="radio" name="lan_proto" value="dhcp" <% nvram_selmatch("lan_proto", "dhcp", "checked"); %> onClick="SelDHCP('dhcp',this.form)"><B><span ><script>Capture(share.enable)</script></span></B>
          <input type="radio" name="lan_proto" value="static" <% nvram_selmatch("lan_proto", "static", "checked"); %> onClick="SelDHCP('static',this.form)"><B><span ><script>Capture(share.disable)</script></span></B></TD>  
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<input type=hidden name=dhcp_check>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(share.startipaddr)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;<b><% prefix_ip_get("lan_ipaddr",1); %></b><B><INPUT maxLength=3 onBlur=valid_range(this,1,254,"DHCP%20starting%20IP") size=3 value='<% nvram_get("dhcp_start"); %>' name="dhcp_start" class=num></B></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(setupcontent.maxdhcpusr)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;<INPUT maxLength=3 onBlur=valid_range(this,1,253,"Number%20of%20DHCP%20users") size=3 value='<% nvram_get("dhcp_num"); %>' name="dhcp_num" class=num></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.clileasetime)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;<INPUT maxLength=4 onBlur=valid_range(this,0,9999,"DHCP%20Lease%20Time") size=4 value='<% nvram_get("dhcp_lease"); %>' name="dhcp_lease" class=num>&nbsp;<script>Capture(setupcontent.clileasetimemin)</script></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% nvram_selmatch("wan_proto","static","<!--"); %>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(setupcontent.stadns1)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;<input type=hidden name=wan_dns value=4><INPUT maxLength=3 onBlur=valid_range(this,0,223,"DNS") size=3 value='<% get_dns_ip("wan_dns","0","0"); %>' name="wan_dns0_0" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"DNS") size=3 value='<% get_dns_ip("wan_dns","0","1"); %>' name="wan_dns0_1" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"DNS") size=3 value='<% get_dns_ip("wan_dns","0","2"); %>' name="wan_dns0_2" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,254,"DNS") size=3 value='<% get_dns_ip("wan_dns","0","3"); %>' name="wan_dns0_3" class=num></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(setupcontent.stadns2)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;<INPUT maxLength=3 onBlur=valid_range(this,0,223,"DNS") size=3 value='<% get_dns_ip("wan_dns","1","0"); %>' name="wan_dns1_0" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"DNS") size=3 value='<% get_dns_ip("wan_dns","1","1"); %>' name="wan_dns1_1" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"DNS") size=3 value='<% get_dns_ip("wan_dns","1","2"); %>' name="wan_dns1_2" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,254,"DNS") size=3 value='<% get_dns_ip("wan_dns","1","3"); %>' name="wan_dns1_3" class=num></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(hindex2.dns3)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;<INPUT maxLength=3 onBlur=valid_range(this,0,223,"DNS") size=3 value='<% get_dns_ip("wan_dns","2","0"); %>' name="wan_dns2_0" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"DNS") size=3 value='<% get_dns_ip("wan_dns","2","1"); %>' name="wan_dns2_1" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"DNS") size=3 value='<% get_dns_ip("wan_dns","2","2"); %>' name="wan_dns2_2" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,254,"DNS") size=3 value='<% get_dns_ip("wan_dns","2","3"); %>' name="wan_dns2_3" class=num></B></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% nvram_selmatch("wan_proto","static","-->"); %>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.wins)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25>&nbsp;<input type=hidden name=wan_wins value=4><INPUT maxLength=3 onBlur=valid_range(this,0,223,"WINS") size=3 value='<% get_single_ip("wan_wins","0"); %>' name="wan_wins_0" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"WINS") size=3 value='<% get_single_ip("wan_wins","1"); %>' name="wan_wins_1" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,255,"WINS") size=3 value='<% get_single_ip("wan_wins","2"); %>' name="wan_wins_2" class=num> . 
            <INPUT maxLength=3 onBlur=valid_range(this,0,254,"WINS") size=3 value='<% get_single_ip("wan_wins","3"); %>' name="wan_wins_3" class=num></TD>
          <TD height=25>&nbsp;</TD>
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
                <TD height=1>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>
        <!--TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=25><B>Time Setting</B></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><input type=radio name=time_mode>Manually</TD>
          <TD width=296 height=25>&nbsp;</TD>  
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;Date:</FONT></TD>
          <TD width=296 height=25>&nbsp;<font face="Arial" style="font-size: 8pt">
                	<INPUT class=num maxLength=4 size=4 name="year" value="<% show_time_setting("year","0"); %>" style="font-family:Courier; font-size:10pt">&nbsp;-&nbsp;
                	<INPUT class=num maxLength=3 size=3 name="mon" value="<% show_time_setting("mon","0"); %>" style="font-family:Courier; font-size:10pt">&nbsp;-&nbsp;
                	<INPUT class=num maxLength=3 size=3 name="mday" value="<% show_time_setting("mday","0"); %>" style="font-family:Courier; font-size:10pt">&nbsp;
                	(yyyy-mm-dd)</font></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;Time:</FONT></TD>
          <TD width=296 height=25>&nbsp;<font face="Arial" style="font-size: 8pt">
                	<INPUT class=num maxLength=3 size=3 name="hour" value="<% show_time_setting("hour","0"); %>" style="font-family:Courier; font-size:10pt">:&nbsp;
                	<INPUT class=num maxLength=3 size=3 name="min" value="<% show_time_setting("min","0"); %>" style="font-family:Courier; font-size:10pt">:&nbsp;
                	<INPUT class=num maxLength=3 size=3 name="sec" value="<% show_time_setting("sec","0"); %>" style="font-family:Courier; font-size:10pt">&nbsp;(hh-mm-ss)	</font></TD>
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
                <TD width=15 height=1>&nbsp;</TD></TR></TBODY></TABLE></TD>
          <TD width=15 background=image/UI_05.gif height=1>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=25><B>Time Setting</B></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><input type=radio name=time_mode checked value="1">Automatically</TD>
          <TD width=296 height=25>&nbsp;</TD>  
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR-->
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=25><b><script>Capture(lefemenu.timeset)</script></b></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(share.timezone)</script>:&nbsp;</TD>
          <TD width=296 height=25>&nbsp;</TD>  
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=397 height=25 colspan="2">&nbsp;<select name="time_zone" onChange=SelTime(this.form.time_zone.selectedIndex,this.form)>
	  <option value="-12 1 0" <% nvram_match("time_zone", "-12 1 0", "selected"); %>><script>Capture(timezone.Kwajalein)</script></option>
	  <option value="-11 1 0" <% nvram_match("time_zone", "-11 1 0", "selected"); %>><script>Capture(timezone.Midway)</script></option>
	  <option value="-10 1 0" <% nvram_match("time_zone", "-10 1 0", "selected"); %>><script>Capture(timezone.Hawaii)</script></option>
	  <option value="-09 1 1" <% nvram_match("time_zone", "-09 1 1", "selected"); %>><script>Capture(timezone.Alaska)</script></option>
	  <option value="-08 1 1" <% nvram_match("time_zone", "-08 1 1", "selected"); %>><script>Capture(timezone.Pacific)</script></option>
	  <option value="-07 1 0" <% nvram_match("time_zone", "-07 1 0", "selected"); %>><script>Capture(timezone.Arizona)</script></option>
	  <option value="-07 2 1" <% nvram_match("time_zone", "-07 2 1", "selected"); %>><script>Capture(timezone.Mountain)</script></option>
	  <option value="-06 1 0" <% nvram_match("time_zone", "-06 1 0", "selected"); %>><script>Capture(timezone.Mexico)</script></option>
	  <option value="-06 2 1" <% nvram_match("time_zone", "-06 2 1", "selected"); %>><script>Capture(timezone.Central)</script></option>
	  <option value="-05 1 0" <% nvram_match("time_zone", "-05 1 0", "selected"); %>><script>Capture(timezone.Indiana)</script></option>
	  <option value="-05 2 1" <% nvram_match("time_zone", "-05 2 1", "selected"); %>><script>Capture(timezone.Eastern)</script></option>
	  <option value="-04 1 0" <% nvram_match("time_zone", "-04 1 0", "selected"); %>><script>Capture(timezone.Bolivia)</script></option>
	  <option value="-04 2 1" <% nvram_match("time_zone", "-04 2 1", "selected"); %>><script>Capture(timezone.Atlantic)</script></option>
	  <option value="-03.5 1 1" <% nvram_match("time_zone", "-03.5 1 1", "selected"); %>><script>Capture(timezone.Newfoundland)</script></option>
	  <option value="-03 1 0" <% nvram_match("time_zone", "-03 1 0", "selected"); %>><script>Capture(timezone.Guyana)</script></option>
	  <option value="-03 2 1" <% nvram_match("time_zone", "-03 2 1", "selected"); %>><script>Capture(timezone.Brazil)</script></option>
	  <option value="-02 1 0" <% nvram_match("time_zone", "-02 1 0", "selected"); %>><script>Capture(timezone.Mid)</script></option>
	  <option value="-01 1 2" <% nvram_match("time_zone", "-01 1 2", "selected"); %>><script>Capture(timezone.Azores)</script></option>
	  <option value="+00 1 0" <% nvram_match("time_zone", "+00 1 0", "selected"); %>><script>Capture(timezone.Gambia)</script></option>
	  <option value="+00 2 2" <% nvram_match("time_zone", "+00 2 2", "selected"); %>><script>Capture(timezone.England)</script></option>
	  <option value="+01 1 0" <% nvram_match("time_zone", "+01 1 0", "selected"); %>><script>Capture(timezone.Tunisia)</script></option>
	  <option value="+01 2 2" <% nvram_match("time_zone", "+01 2 2", "selected"); %>><script>Capture(timezone.France)</script></option>
	  <option value="+02 1 0" <% nvram_match("time_zone", "+02 1 0", "selected"); %>><script>Capture(timezone.South)</script></option>
	  <option value="+02 2 2" <% nvram_match("time_zone", "+02 2 2", "selected"); %>><script>Capture(timezone.Greece)</script></option>
	  <option value="+03 1 0" <% nvram_match("time_zone", "+03 1 0", "selected"); %>><script>Capture(timezone.Iraq)</script></option>
	  <option value="+04 1 0" <% nvram_match("time_zone", "+04 1 0", "selected"); %>><script>Capture(timezone.Armenia)</script></option>
	  <option value="+05 1 0" <% nvram_match("time_zone", "+05 1 0", "selected"); %>><script>Capture(timezone.Pakistan)</script></option>
	  <option value="+05.5 1 0" <% nvram_match("time_zone", "+05.5 1 0", "selected"); %>><script>Capture(timezone.india)</script></option>
	  <option value="+06 1 0" <% nvram_match("time_zone", "+06 1 0", "selected"); %>><script>Capture(timezone.Bangladesh)</script></option>
	  <option value="+07 1 0" <% nvram_match("time_zone", "+07 1 0", "selected"); %>><script>Capture(timezone.Thailand)</script></option>
	  <option value="+08 1 0" <% nvram_match("time_zone", "+08 1 0", "selected"); %>><script>Capture(timezone.China)</script></option>
	  <option value="+08 2 0" <% nvram_match("time_zone", "+08 2 0", "selected"); %>><script>Capture(timezone.Singapore)</script></option>
	  <option value="+09 1 0" <% nvram_match("time_zone", "+09 1 0", "selected"); %>><script>Capture(timezone.Japan)</script></option>
	  <option value="+10 1 0" <% nvram_match("time_zone", "+10 1 0", "selected"); %>><script>Capture(timezone.Guam)</script></option>
	  <option value="+10 2 4" <% nvram_match("time_zone", "+10 2 4", "selected"); %>><script>Capture(timezone.Australia)</script></option>
	  <option value="+11 1 0" <% nvram_match("time_zone", "+11 1 0", "selected"); %>><script>Capture(timezone.Solomon)</script></option>
	  <option value="+12 1 0" <% nvram_match("time_zone", "+12 1 0", "selected"); %>><script>Capture(timezone.Fiji)</script></option>
	  <option value="+12 2 4" <% nvram_match("time_zone", "+12 2 4", "selected"); %>><script>Capture(timezone.New_Zealand)</script></option>
	</select></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD align=right width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><INPUT type=checkbox value="1" name="_daylight_time" <% nvram_match("daylight_time","1","checked"); %>><font face="Arial" style="font-size: 8pt"><script>Capture(setupcontent.autoadjtime)</script></font></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>

        <TR>
          <TD width=44 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=65 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=47 bgColor=#e7e7e7>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif>&nbsp;</TD>
          <TD width=454 colSpan=6></TD>
          <TD width=15 background=image/UI_05.gif>&nbsp;</TD></TR>


	</TBODY></TABLE></TD>

    <TD vAlign=top width=176 bgColor=#6666cc>
      <TABLE cellSpacing=0 cellPadding=0 width=176 border=0>
        <TBODY>
        <TR>
          <TD width=11 bgColor=#6666cc height=25>&nbsp;</TD>
          <TD width=156 bgColor=#6666cc height=25><font color="#FFFFFF">
          <span ></span></font><br><font color="#FFFFFF"><span style="font-family: Arial"><br>
<script language=javascript>
if(document.setup.now_proto.value == "dhcp"){
Capture(hindex2.hdhcp);
}
else if(document.setup.now_proto.value == "pppoe"){
Capture(hindex2.hpppoe1);
document.write("<br>");
Capture(hindex2.hpppoe2);
document.write("<br>");
Capture(hindex2.hpppoe3);
document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");

}
else if(document.setup.now_proto.value == "static"){
Capture(hindex2.hstatic1);
document.write("<br>");
Capture(hindex2.hstatic2);
document.write("<br>");
Capture(hindex2.hstatic3);

document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");
}
else if(document.setup.now_proto.value == "pptp"){
Capture(hindex2.hpptp1);
document.write("<br>");
Capture(hindex2.hpptp2);
document.write("<br>");
Capture(hindex2.hpptp3);
document.write("<br>");
Capture(hindex2.hpptp4);

document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");

//Capture(hindex2.hpptp5);
}
else if(document.setup.now_proto.value == "l2tp"){
Capture(hindex2.hl2tp1);
document.write("<br>");
Capture(hindex2.hl2tp2);
document.write("<br>");
Capture(hindex2.hl2tp3);

document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");

//Capture(hindex2.hl2tp4);
}
else if(document.setup.now_proto.value == "heartbeat"){
Capture(hindex2.hhb1);
document.write("<br>");
Capture(hindex2.hhb2);
document.write("<br>");
Capture(hindex2.hhb3);

document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");

//Capture(hindex2.hhb4);
}
</script>
<script>Capture(hindex2.right1)</script>
<script>document.write("<br>")</script>
<script>Capture(hindex2.right2)</script>
<b><a target="_blank" href="help/HSetup.asp"><script>Capture(share.more)</script></a></b><br><br><br><br>
<script>Capture(hindex2.right3)</script>
<script>document.write("<br>")</script>
<script>Capture(hindex2.right4)</script>
<script>document.write("<br>")</script>
<script>Capture(hindex2.right5)</script>
<script>document.write("<br>")</script>
<script>Capture(hindex2.right6)</script>
<script>document.write("<br>")</script>
<script language=javascript>
if(document.setup.now_proto.value == "dhcp"){
Capture(hindex2.hdhcps1);
document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br>");
//document.write("<b>Maximum number of DHCP Users: </b>You may limit the number of addresses your router hands out.<br>");
//document.write("<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br>");
}
else if(document.setup.now_proto.value == "static"){
document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br>");
}
else if(document.setup.now_proto.value == "pppoe"){
//document.write("<b>Maximum number of DHCP Users: </b>You may limit the number of addresses your router hands out.<br>");
//document.write("<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br>");
Capture(hindex2.hdhcps1);
document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");
}
else if(document.setup.now_proto.value == "pptp"){
//document.write("<b>Maximum number of DHCP Users: </b>You may limit the number of addresses your router hands out.<br>");
//document.write("<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br>");
Capture(hindex2.hdhcps1);
document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");
}
else if(document.setup.now_proto.value == "l2tp"){
//document.write("<b>Maximum number of DHCP Users: </b>You may limit the number of addresses your router hands out.<br>");
//document.write("<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br>");
Capture(hindex2.hdhcps1);
document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");
}
else if(document.setup.now_proto.value == "heartbeat"){
//document.write("<b>Maximum number of DHCP Users: </b>You may limit the number of addresses your router hands out.<br>");
//document.write("<b><a target=_blank href=help/HSetup.asp>More...</a></b><br><br><br><br>");
Capture(hindex2.hdhcps1);
document.write("<b><a target=_blank href=help/HSetup.asp>");
Capture(share.more);
document.write("</a></b><br><br><br><br>");
}
</script>
<script>document.write("<br>")</script>
<script>Capture(hindex2.right7)</script>
</span></font>
</TD>
          <TD width=9 bgColor=#6666cc height=25>&nbsp;</TD></TR></TBODY></TABLE></TD></TR>
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
          <TD width=454 align=right bgColor=#6666cc>

<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;


<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"index.asp\")>");</script>&nbsp;&nbsp;

          </TD>
          <TD width=15 bgColor=#000000 height=33>&nbsp;</TD>
</TR></TBODY></TABLE></TD></TR></TBODY></TABLE></FORM></DIV></BODY></HTML>
