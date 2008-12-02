
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

<HTML><HEAD><TITLE>Ping Test</TITLE>
<% charset(); %>
<link rel="stylesheet" type="text/css" href="style.css">
<script src="common.js"></script>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capadmin.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=javascript>
document.title = adleftmenu.ping;
document.onkeydown = onInputKeydown;
function onInputKeydown(event)
{
	if(typeof event == "undefined")
	{
		return handleKeyDown(window.event);
	}
	else 
	{
		return handleKeyDown(event);
	}
}
function handleKeyDown(event)
{
	if(event.keyCode == 13)
	{
		return to_submit(document.ping, "start");
	}
	return true;
}
function to_submit(F,I)
{
	if(valid(F,I)){
		F.submit_type.value = I;
		F.submit_button.value = "Ping";
		F.change_action.value = "gozila_cgi";
		F.submit();
		return true;
	}
	return false;
}
function check_ip(F, value)
{
	var i;
	var ip = new Array();
	var ip2 = new Array(4);
	ip = value.split(".");
	for(i = 0; i< ip.length; i++)
	{	
		if(!check_blank(ip[i]))
		{
			return false;
		}
	}
	for(i = 0; i< ip.length; i++)
	{
		if(!check_digit(ip[i]))
		{
			return true;
		}
	}
	if(ip.length != 4)
	{
		return false;
	}
		ip2[0] = Number(ip[0]);
		ip2[1] = Number(ip[1]);
		ip2[2] = Number(ip[2]);
		ip2[3] = Number(ip[3]);
		if(ip2[0] == 0 || ip2[0] > 223 || ip2[3] == 0 || ip2[3] > 254 || ip2[1] > 255 || ip2[2] > 255 )
		{
			return false;
		}
	return true;
}
function check_digit(value)
{
	var i;
	for(i=0 ; i<value.length; i++)
	{
		ch = value.charAt(i);
		if(ch < '0' || ch > '9')
		{
			return false;
		}
}
	return true;
}
function check_blank(value)
{
	var i;
	
	if(value.length == 0)
	{
		return false;
	}
	for(i=0 ; i<value.length; i++)
	{
		ch = value.charAt(i);
		if(ch == ' ' || ch == ',' || ch =='#' || ch =='@'|| ch =='$'|| ch =='%'|| ch =='*'|| ch =='&'|| ch =='?')
		{
			return false;
		}
	}
	return true;
}
function valid(F,I)
{
	if(I == "start" && F.ping_ip.value == ""){
//              alert("You must input an IP Address or Domain Name!");
                alert(errmsg.err8);
		F.ping_ip.focus();
		return false;
	}
	else if(I == "start" && !check_ip(F, F.ping_ip.value))
	{
		alert(errmsg.err61);
		F.ping_ip.focus();
		return false;
	}
	return true;
}
var value=0;
function Refresh()
{
	refresh_time = 5000;
	if (value>=1)
	{
		window.location.replace("Ping.asp");
	}
	value++;
	timerID=setTimeout("Refresh()",refresh_time);
}
function init()
{
	window.location.href = "#";
	<% onload("Ping", "Refresh();"); %>
}
function exit()
{
	//if(!confirm("Do you want to stop ping?"))
	//	self.close();
	//else{
		to_submit(document.ping,"stop");
		self.close();
	//}
}
</SCRIPT>
</HEAD>
<BODY text=#000000 bgColor=#ffffff onload="init()">
<FORM name=ping method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button value="Ping">
<input type=hidden name=submit_type value="start">
<input type=hidden name=action value="Apply">
<input type=hidden name=change_action value="gozila_cgi">
<div align="center">
  <center>
  <table border="0" cellpadding="0" cellspacing="0" style="border-collapse: collapse; border-width: 0" bordercolor="#111111" id="AutoNumber1" width="458" height="322">

    <tr>
      <td style="border-style: none; border-width: medium" width="459" height="19" colSpan=3>
      <b><font face="Arial" color="#0000FF"><SPAN STYLE="FONT-SIZE: 14pt"><script>Capture(adleftmenu.ping)</script></SPAN></font></b></td>
      <!-- td style="border-style: none; border-width: medium" width="224" height="19">&nbsp;</td -->
      <!-- td style="border-style: none; border-width: medium" height="19" align="right" width="41">&nbsp;</td -->
    </tr>

    <tr>
      <td style="border-style: none; border-width: medium" width="194" height="39">
      <b><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(ping.ipdomain)</script>:&nbsp;</SPAN></FONT></b></td>
      <td style="border-style: none; border-width: medium" width="224" height="39" align="left"> 
    <INPUT maxLength=31 name=ping_ip size="31" value="<% nvram_selget("ping_ip"); %>" onBlur=valid_name(this,"IP")></td>
      <td style="border-style: none; border-width: medium" height="39" align="right" width="41">

<script>document.write("<INPUT onclick=to_submit(this.form,\'start\'); type=button name=ping_button value=\"" + adbutton.ping + "\">");</script>

      </td>
    </tr>
    <tr>
      <td style="border-style: none; border-width: medium" width="194" height="22">
      <b>
    <font face="Arial"><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(ping.times)</script>:&nbsp;</SPAN></font> <FONT face=Arial size=1>
    &nbsp;&nbsp; </FONT> </b></td>
      <td style="border-style: none; border-width: medium" width="224" height="22"> 
    <B> <FONT face=Arial size=1>
    <SELECT name=ping_times> 
        <OPTION value=5 <% nvram_selmatch("ping_times", "5", "selected"); %>>5</OPTION> 
        <OPTION value=10 <% nvram_selmatch("ping_times", "10", "selected"); %>>10</OPTION>
        <option value=0 <% nvram_selmatch("ping_times", "0", "selected"); %>><script>Capture(ping.unlimited)</script></option>
    </SELECT></FONT></B></td>
      <td style="border-style: none; border-width: medium" height="22" align="right" width="41"></td>
    </tr>
    <tr>
      <td style="border-style: none; border-width: medium" width="194" height="18">
      </td>
      <td style="border-style: none; border-width: medium" width="224" height="18"> 
      </td>
      <td style="border-style: none; border-width: medium" height="18" align="right" width="41">
      </td>
    </tr>
    <tr>
      <td style="border-style: double; border-width: 3" width="453" colspan="3" height="293" bordercolor="#808080" valign=top>
<script language=javascript>

var germany_ping_string = new Array("Network is unreachable", "Netzwerk ist unerreichbar", "packets transmitted", "Pakete gesendet", "packets received", "Pakete empfangen", "packet loss", "Pakete verloren", "data bytes", "Datenbytes", "ping statistics", "Ping-Statistik", "round-trip min/avg/max", "Durchlauf, Minimum/Mittelwert/Maximum", "bytes from", "Bytes von", "Time", "Zeit");
						
var table = new Array(
<% dump_ping_log(""); %>
);

   var i = 0;
   var a = new String("");
   var b = new String("");
   var language =  '<% nvram_get("language"); %>'

   if (language == "DE") {
       for (i = 0; i < table.length; i++) {
           for (j = 0; j < germany_ping_string.length; j += 2) {
   	       RE = new RegExp(germany_ping_string[j], "i");
	       a = table[i];
	       b = a.replace(RE, germany_ping_string[j+1]);
	       table[i] = b;
  	   }
        }
    }
   
   i = 0;
   for(;;){
	if(!table[i])	break;
	document.write(table[i]+"<br>");
	i = i + 1;
   }
</script>
</td>
    </tr>
    <tr>
      <td style="border-style: none; border-width: medium" height="45" colspan="3" align="right" width="459">
      <p align="right">&nbsp; 

<script>document.write("<INPUT onclick=to_submit(this.form,\'stop\') type=button name=stop_button value=\"" + adbutton.stop + "\">");</script>

         &nbsp;

<script>document.write("<INPUT onclick=to_submit(this.form,\'clear\') type=button name=clog_button value=\"" + adbutton.clearlog + "\">");</script>

         &nbsp;

<script>document.write("<INPUT onclick=exit() type=button name=close_button value=\"" + sbutton.close + "\">");</script>

    </td>
<a name="#"></a>
    </tr>
  </table>
  </center>
</div>
<CENTER>
<p><WINDWEB_URL>
</p>
</CENTER></BODY></HTML>
