
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

<HTML><HEAD><TITLE>MAC Address Filter List</TITLE>
<% no_cache(); %>
<% charset(); %>
<link rel="stylesheet" type="text/css" href="style.css">
<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capapp.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=javascript>
document.title = wlanmaclist.macaddrlist;
var active_win = null;
function ViewActive()
{
	active_win = self.open('WL_ActiveTable.asp','ActiveTable','alwaysRaised,resizable,scrollbars,width=650,height=450');
	active_win.focus();
}
function to_submit(F)
{
        F.submit_button.value = "WL_FilterTable";
        F.action.value = "Apply";
        F.submit();
}
function SelPage(num,F)
{
	F.submit_button.value = "WL_FilterTable";
	F.change_action.value = "gozila_cgi";
	F.wl_filter_page.value=F.wl_filter_page.options[num].value;
	F.submit();
}
function valid_macs_all(I)
{
	if(I.value == "")
		return true;
	else if(I.value.length == 12)
		valid_macs_12(I);
	else if(I.value.length == 17)
		valid_macs_17(I);
	else{
//              alert('The MAC Address length is not correct!!');
                alert(errmsg.err5);
		I.value = I.defaultValue;
        }
	
}
function exit()
{
	closeWin(active_win);
}
</SCRIPT>

</HEAD>
<BODY bgColor=white onload={window.focus();} onunload=exit()>
<FORM name=macfilter method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=submit_type>
<input type=hidden name=change_action>
<input type=hidden name=action>
<input type=hidden name=wl_mac_list>
<input type=hidden name=small_screen>

<CENTER>
<TABLE cellSpacing=0 cellPadding=10 width=436 border=1>
  <TBODY>
  <TR>
    <TD width=412>
      <TABLE height=320 cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <TR>
          <TD colSpan=5 height=17 align="center">
            <P align=center><B><FONT color=#0000ff><SPAN STYLE="FONT-SIZE:13pt"><script>Capture(wlanmaclist.macaddrlist)</script></SPAN></FONT></B></P></TD></TR>
        <TR>
          <TD colSpan=5 height=40 align="center"><SPAN STYLE="FONT-SIZE:10pt"><script>Capture(wlanmaclist.macformat)</script>: xxxxxxxxxxxx</SPAN></TD></TR>
        <TR>
          <TD colSpan=5 height=40 align="center" valign="top">
            <p style="margin-top: 5; margin-bottom: 5"><font face="Arial"> 

<script>document.write("<INPUT onclick=ViewActive() type=button name=button5 value=\"" + wlanbutton.climacls + "\">");</script>


</font></p>
            </TD></TR>
	<% wireless_filter_table("input"); %>
          </TBODY></TABLE></TD></TR></TBODY></TABLE>
<P>

<script>document.write("<input type=button name=save_button" + " value=\"" + sbutton.save + "\" onClick=to_submit(this.form)>");</script>&nbsp;

<script>document.write("<input type=button name=cancel" + " value=\"" + sbutton.cancel + "\" onClick=window.location.replace(\"WL_FilterTable.asp\")>");</script>

</CENTER></P>
</FORM>
</BODY>
</HTML>
