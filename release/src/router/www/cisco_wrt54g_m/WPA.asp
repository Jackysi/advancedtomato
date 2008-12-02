
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

<HTML><HEAD><TITLE>WEP Keys</TITLE>
<% no_cache(); %>
<% charset(); %>
<link rel="stylesheet" type="text/css" href="style.css">
<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=javascript>
function to_submit(F)
{
	if(valid_value(F)){
        	F.submit_button.value = "WPA";
    		F.action.value = "Apply";
       		F.submit();
	}
}               
function valid_value(F)
{
	if(!valid_wpa_psk(F.wpa_psk))	return false;
	else	return true;
}
function init(){
	SelMode("<% nvram_get("auth_mode"); %>",document.wpa);
}
function SelMode(num,F)
{
	if(num == 0 || num == 'disabled'){
		choose_disable(F.wpa_psk);
		choose_disable(F.wpa_gtk_rekey);
		choose_disable(F.radius_ipaddr_0);
		choose_disable(F.radius_ipaddr_1);
		choose_disable(F.radius_ipaddr_2);
		choose_disable(F.radius_ipaddr_3);
		choose_disable(F.radius_port);
		choose_disable(F.radius_key);
	}else if (num == 1 || num == 'radius'){
		choose_disable(F.wpa_psk);
                choose_disable(F.wpa_gtk_rekey);
                choose_enable(F.radius_ipaddr_0);
                choose_enable(F.radius_ipaddr_1);
                choose_enable(F.radius_ipaddr_2);
                choose_enable(F.radius_ipaddr_3);
                choose_enable(F.radius_port);
                choose_enable(F.radius_key);
	}else if (num == 2 || num == 'wpa'){
		choose_disable(F.wpa_psk);
                choose_enable(F.wpa_gtk_rekey);
                choose_enable(F.radius_ipaddr_0);
                choose_enable(F.radius_ipaddr_1);
                choose_enable(F.radius_ipaddr_2);
                choose_enable(F.radius_ipaddr_3);
                choose_enable(F.radius_port);
                choose_enable(F.radius_key);
	}else if (num == 3 || num == 'psk'){
		choose_enable(F.wpa_psk);
                choose_enable(F.wpa_gtk_rekey);
                choose_disable(F.radius_ipaddr_0);
                choose_disable(F.radius_ipaddr_1);
                choose_disable(F.radius_ipaddr_2);
                choose_disable(F.radius_ipaddr_3);
                choose_disable(F.radius_port);
                choose_disable(F.radius_key);
	}

	if(num == 2 || num == 'wpa' || num == 3 || num == 'psk'){
		document.forms[0].wl_wep.length = 2;
		document.forms[0].wl_wep[0] = new Option("TKIP","tkip");
		document.forms[0].wl_wep[0].value = "tkip";
		document.forms[0].wl_wep[1] = new Option("AES","aes");
		document.forms[0].wl_wep[1].value = "aes";
	}
	else{
		document.forms[0].wl_wep.length = 2;
		document.forms[0].wl_wep[0] = new Option("OFF","off");
		document.forms[0].wl_wep[0].value = "off";
		document.forms[0].wl_wep[1] = new Option("WEP","restricted");
		document.forms[0].wl_wep[1].value = "restricted";
	}
	
	for(i=0 ; i < document.forms[0].wl_wep.length; i++){
		if(document.forms[0].wl_wep[i].value == "<% nvram_get("wl_wep"); %>"){
			document.forms[0].wl_wep[i].selected = true;
			break;
		}
	}
}
function valid_wpa_psk(I)
{
	if(I.value.length == 64){
		if(!isxdigit(I,I.value)) return false;
	}	
	else if(I.value.length >=8 && I.value.length <= 63 ){
		if(!isascii(I,I.value)) return false;
	}
	else{
		alert("Invalid Key, must be between 8 and 63 ASCII characters or 64 hexadecimal digits");
		return false;
	}
	return true;	
}

</SCRIPT>

</HEAD>
<BODY bgColor=#ffffff onload=init()>
<CENTER>
<FORM name=wpa method=<% get_http_method(); %> action=Apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=change_action>
<input type=hidden name=submit_type>
<input type=hidden name=action>
<input type=hidden name=small_screen>

<TABLE 
style="BORDER-RIGHT: 3px double; BORDER-TOP: 3px double; BORDER-LEFT: 3px double; BORDER-BOTTOM: 3px double; BORDER-COLLAPSE: collapse" 
borderColor=#111111 height=282 cellSpacing=0 cellPadding=0 width=367 
bgColor=#ffffff border=0>
  <TBODY>
  <TR>
    <TH width=768 colSpan=2 height=282>
      <TABLE height=296 cellSpacing=3 width=602 bgColor=#ffffff border=0>
        <TBODY>
        <TR>
          <TH width=747 height=232>
            <TABLE height=297 cellSpacing=0 width=593 bgColor=#ffffff 
              border=0><TBODY>
              <TR>
                <TH align=right width=170 bgColor=#6666cc height=79>
        <img src="image/11g.gif" tppabs="image/11g.gif" width="129" height="73"></TH>
                <TD vAlign=bottom width=435 height=106>&nbsp;</TD></TR>

              <TR>
                <TH vAlign=bottom align=right width=150 bgColor=#6666cc height=7>&nbsp;</TH>
                <TD vAlign=bottom width=435 height=7>
                  <HR color="#CCCCCC">
                </TD></TR>
              <TR>
                <TH align=right width=170 bgColor=#6666cc height=25>Network Authencation:&nbsp;&nbsp;</TH>
                <TD width=435 height=25>&nbsp;
			<SELECT name=auth_mode size=1 onChange=SelMode(this.form.auth_mode.selectedIndex,this.form)><OPTION value=disabled <% nvram_selmatch("auth_mode","disabled","selected"); %>>Disabled</OPTION>
                        <OPTION value=radius <% nvram_selmatch("auth_mode","radius","selected"); %>>802.1X</OPTION>
                        <OPTION value=wpa <% nvram_selmatch("auth_mode","wpa","selected"); %>>WPA</OPTION>
                        <OPTION value=psk <% nvram_selmatch("auth_mode","psk","selected"); %>>WPA-PSK</OPTION></SELECT></TD></TR>
              <TR>
                <TH align=right width=170 bgColor=#6666cc height=25>&nbsp;&nbsp;&nbsp;WPA Pre-Shared Key:&nbsp;&nbsp;</TH>
                <TD align=left width=435 height=25>&nbsp;
			<INPUT size=32 name=wpa_psk value='<% nvram_get("wpa_psk"); %>' maxlength=64></TD></TR>
              <TR>
                <TH align=right width=170 bgColor=#6666cc height=25>&nbsp;WPA Group Rekey&nbsp;&nbsp;<br>Interval:&nbsp;&nbsp;</TH>
                <TD align=left width=435 height=25>&nbsp;
                	<INPUT maxLength=79 name=wpa_gtk_rekey size=10 value='<% nvram_get("wpa_gtk_rekey"); %>' onBlur=valid_range(this,600,7200,"rekey%20interval")>&nbsp;Sec.</TD></TR>
              <TR>
                <TH vAlign=center align=right width=170 bgColor=#6666cc height=25>RADIUS Server:&nbsp;&nbsp;</TH>
                <TD vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;<INPUT type=hidden name=radius_ipaddr value=4>
			<INPUT size=3 maxlength=3 name=radius_ipaddr_0 value='<% get_single_ip("radius_ipaddr","0"); %>' onBlur=valid_range(this,0,255,"IP")> . 
			<INPUT size=3 maxlength=3 name=radius_ipaddr_1 value='<% get_single_ip("radius_ipaddr","1"); %>' onBlur=valid_range(this,0,255,"IP")> . 
			<INPUT size=3 maxlength=3 name=radius_ipaddr_2 value='<% get_single_ip("radius_ipaddr","2"); %>' onBlur=valid_range(this,0,255,"IP")> . 
			<INPUT size=3 maxlength=3 name=radius_ipaddr_3 value='<% get_single_ip("radius_ipaddr","3"); %>' onBlur=valid_range(this,1,254,"IP")>
</TD></TR>
              <TR>
                <TH vAlign=center align=right width=170 bgColor=#6666cc height=25>RADIUS Port:&nbsp;&nbsp;</TH>
                <TD vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;
			<INPUT size=5 name=radius_port value='<% nvram_get("radius_port"); %>' maxlength=5 onBlur=valid_range(this,0,65535,"Port")></TD></TR>
              <TR>
                <TH vAlign=center align=right width=170 bgColor=#6666cc height=25>RADIUS Key:&nbsp;&nbsp;</TH>
                <TD vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;
			<INPUT size=20 name=radius_key value='<% nvram_get("radius_key"); %>' maxlength=79></TD></TR>
              <TR>
                <TH vAlign=center align=right width=170 bgColor=#6666cc height=25>&nbsp;&nbsp;</TH>
                <TD vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;</TD></TR>
              <TR>
                <TH align=right width=170 bgColor=#6666cc height=25>Data Encryption:&nbsp;&nbsp;</TH>
                <TD width=435 height=25>&nbsp;
			<SELECT name=wl_wep size=1>
			<OPTION value=off <% nvram_selmatch("wl_wep","off","selected"); %>>Disabled</OPTION>
                        <OPTION value=restricted <% nvram_selmatch("wl_wep","restricted","selected"); %>>WEP</OPTION>
                        <OPTION value=tkip <% nvram_selmatch("wl_wep","tkip","selected"); %>>TKIP</OPTION>
                        <OPTION value=aes <% nvram_selmatch("wl_wep","aes","selected"); %>>AES</OPTION></SELECT></TD></TR>
              <TR>
                <TD vAlign=center align=right width=170 bgColor=#6666cc height=40></TD>
                <TD align=left width=435 height=50>
                  <DIV align=left>&nbsp;&nbsp;<INPUT type=button value="  Apply  " onClick=to_submit(this.form)>&nbsp; <INPUT class=b1 type=reset value=" Cancel " name=cancel>&nbsp; <INPUT onclick=showHELP(this.form) type=button value="   Help  " name=Submit3> 
                  </DIV></TD></TR></TBODY></TABLE></TH></TR></TBODY></TABLE></TR></TBODY></TABLE></FORM></CENTER></BODY></HTML>
