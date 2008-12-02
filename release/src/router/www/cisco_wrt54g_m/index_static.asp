
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

  <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.interipaddr)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial><input type=hidden name="wan_ipaddr" value=4>
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_ipaddr","0"); %>' name="wan_ipaddr_0" onBlur=valid_range(this,0,223,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_ipaddr","1"); %>' name="wan_ipaddr_1" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_ipaddr","2"); %>' name="wan_ipaddr_2" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_ipaddr","3"); %>' name="wan_ipaddr_3" onBlur=valid_range(this,0,254,"IP")>
	  </FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
		<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.submask)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial><input type=hidden name="wan_netmask" value=4>
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_netmask","0"); %>' name="wan_netmask_0" onBlur=valid_range(this,0,255,"netmask")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_netmask","1"); %>' name="wan_netmask_1" onBlur=valid_range(this,0,255,"netmask")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_netmask","2"); %>' name="wan_netmask_2" onBlur=valid_range(this,0,255,"netmask")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_netmask","3"); %>' name="wan_netmask_3" onBlur=valid_range(this,0,255,"netmask")>
	  </FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
		<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial>&nbsp;<script>Capture(share.gateway)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial><input type=hidden name="wan_gateway" value=4>
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_gateway","0"); %>' name="wan_gateway_0" onBlur=valid_range(this,0,223,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_gateway","1"); %>' name="wan_gateway_1" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_gateway","2"); %>' name="wan_gateway_2" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_single_ip("wan_gateway","3"); %>' name="wan_gateway_3" onBlur=valid_range(this,0,254,"IP")>
	  </FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
		<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(setupcontent.stadns1)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" 
            face=Arial><input type=hidden name="wan_dns" value="3">
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","0","0"); %>' name="wan_dns0_0" onBlur=valid_range(this,0,223,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","0","1"); %>' name="wan_dns0_1" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","0","2"); %>' name="wan_dns0_2" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","0","3"); %>' name="wan_dns0_3" onBlur=valid_range(this,0,254,"IP")>
	  </FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
		<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(setupcontent.stadns2)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","1","0"); %>' name="wan_dns1_0" onBlur=valid_range(this,0,223,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","1","1"); %>' name="wan_dns1_1" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","1","2"); %>' name="wan_dns1_2" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","1","3"); %>' name="wan_dns1_3" onBlur=valid_range(this,0,254,"IP")>
	  </FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
		<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(hindex2.dns3)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","2","0"); %>' name="wan_dns2_0" onBlur=valid_range(this,0,223,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","2","1"); %>' name="wan_dns2_1" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","2","2"); %>' name="wan_dns2_2" onBlur=valid_range(this,0,255,"IP")> .
		<INPUT class=num maxLength=3 size=3 value='<% get_dns_ip("wan_dns","2","3"); %>' name="wan_dns2_3" onBlur=valid_range(this,0,254,"IP")>
	  </FONT></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
