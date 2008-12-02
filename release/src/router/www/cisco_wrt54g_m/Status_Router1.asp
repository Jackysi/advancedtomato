
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

<% nvram_status_get("hidden1"); %>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD><FONT style="FONT-SIZE: 8pt"><script>Capture(stacontent.logsta)</script>:&nbsp;</FONT></TD>
          <TD><FONT style="FONT-SIZE: 8pt"><B>
<script language=javascript>
        var status1 = "<% nvram_status_get("status1"); %>";
        var status2 = "<% nvram_status_get("status2"); %>";
	if(status1 == "Status")         status1 = bmenu.statu;
        if(status2 == "Connecting")     status2 = hstatrouter2.connecting;
        else    if(status2 == "Disconnected")   status2 = hstatrouter2.disconnected;
        else    if(status2 == "Connected")      status2 = stacontent.conn;
	document.write(status2);
	document.write("&nbsp;&nbsp;");

	var but_arg = "<% nvram_status_get("button1"); %>";
        var wan_proto = "<% nvram_get("wan_proto"); %>";
        var but_type = "";
	if(but_arg == "Connect")        but_value = stacontent.connect;
        else if(but_arg == "Disconnect")        but_value = hstatrouter2.disconnect;
        but_type = but_arg +"_" + wan_proto;
	document.write("<INPUT type=button value='"+but_value+"' onClick=Connect(this.form,'"+but_type+"')>");
</script>
</B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
<% nvram_status_get("hidden2"); %>        
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD><FONT style="FONT-SIZE: 8pt"><script>Capture(share.ipaddr)</script>:&nbsp;</FONT></TD>
          <TD><FONT style="FONT-SIZE: 8pt"><B><% nvram_status_get("wan_ipaddr"); %></B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 height=25><IMG height=30 
            src="image/UI_04.gif" width=8 border=0></TD>
          <TD bgColor=#ffffff colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 bgColor=#ffffff height=25><FONT 
            style="FONT-SIZE: 8pt"><script>Capture(share.submask)</script>:&nbsp;</FONT></TD>
          <TD width=296 bgColor=#ffffff height=25><FONT 
            style="FONT-SIZE: 8pt"><B><% nvram_status_get("wan_netmask"); %></B></FONT></TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=15 bgColor=#ffffff height=25><IMG height=30 
            src="image/UI_05.gif" width=15 border=0></TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 height=25><IMG height=30 
            src="image/UI_04.gif" width=8 border=0></TD>
          <TD bgColor=#ffffff colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 bgColor=#ffffff height=25><FONT 
            style="FONT-SIZE: 8pt"><script>Capture(share.defgateway)</script>:&nbsp;</FONT></TD>
          <TD width=296 bgColor=#ffffff height=25><FONT 
            style="FONT-SIZE: 8pt"><B><% nvram_status_get("wan_gateway"); %></B></FONT></TD>
          <TD width=13 bgColor=#ffffff height=25>&nbsp;</TD>
          <TD width=15 bgColor=#ffffff height=25><IMG height=30 
            src="image/UI_05.gif" width=15 border=0></TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif 
          height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD><FONT style="FONT-SIZE: 8pt"><script>Capture(share.dns)</script> 1:&nbsp;</FONT></TD>
          <TD><FONT style="FONT-SIZE: 8pt"><B><% nvram_status_get("wan_dns0"); %></B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
          height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif 
          height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD height=25><FONT style="FONT-SIZE: 8pt"><script>Capture(share.dns)</script> 2:&nbsp;</FONT></TD>
          <TD height=25><FONT style="FONT-SIZE: 8pt"><B><% nvram_status_get("wan_dns1"); %></B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif 
          height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif 
          height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD height=25><FONT style="FONT-SIZE: 8pt"><script>Capture(share.dns)</script> 3:&nbsp;</FONT></TD>
          <TD height=25><FONT style="FONT-SIZE: 8pt"><B><% nvram_status_get("wan_dns2"); %></B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>        
        <TR>
          <TD width=156 bgColor=#e7e7e7 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif 
          height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD height=25><FONT style="FONT-SIZE: 8pt"><script>Capture(share.mtu)</script>:&nbsp;</FONT></TD>
          <TD height=25><FONT style="FONT-SIZE: 8pt"><B><% nvram_get("wan_run_mtu"); %></B></FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>        
