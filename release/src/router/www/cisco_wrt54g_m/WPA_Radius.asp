
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
          <TD bgColor=#e7e7e7 height=25></TD>
          <TD background=image/UI_04.gif height=25></TD>
          <TD height=25></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.wpaalg)</script>:&nbsp;</TD>
          <TD height=25>&nbsp;<SELECT name=wl_crypto>
		<script>
		     var enc = '<% nvram_get("wl_crypto"); %>';
		     var now_security_mode2 = document.wpa.security_mode2.options[document.wpa.security_mode2.selectedIndex].value;

			if(now_security_mode2 == "wpa_enterprise") {
				if (enc == "tkip") {
					document.write("<OPTION value=tkip selected>"+wlansec.tkip+"</OPTION>");
					document.write("<OPTION value=aes>"+wlansec.aes+"</OPTION>");
				}
				else {
					document.write("<OPTION value=tkip>"+wlansec.tkip+"</OPTION>");
					document.write("<OPTION value=aes selected>"+wlansec.aes+"</OPTION>");
				}
			}else{	// wpa2_enterprise
				if (enc == "aes") {
					document.write("<OPTION value=aes selected>"+wlansec.aes+"</OPTION>");
					document.write("<OPTION value=tkip+aes>"+wlansec.tkip+"+"+wlansec.aes+"</OPTION>");
				}
				else {
					document.write("<OPTION value=aes>"+wlansec.aes+"</OPTION>");
					document.write("<OPTION value=tkip+aes selected>"+wlansec.tkip+"+"+wlansec.aes+"</OPTION>");
				}
			}
		</script>
		</SCRIPT></TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
        <TR>
          <TD bgColor=#e7e7e7 height=25></TD>
          <TD background=image/UI_04.gif height=25></TD>
          <TD ></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.radiussrv)</script>:&nbsp;</TD>
          <TD height=25><INPUT type=hidden name=wl_radius_ipaddr value=4>&nbsp;<INPUT size=3 maxlength=3 name=wl_radius_ipaddr_0 value='<% get_single_ip("wl_radius_ipaddr","0"); %>' onBlur=valid_range(this,1,223,"IP") class=num> .
                  <INPUT size=3 maxlength=3 name=wl_radius_ipaddr_1 value='<% get_single_ip("wl_radius_ipaddr","1"); %>' onBlur=valid_range(this,0,255,"IP") class=num> .
                  <INPUT size=3 maxlength=3 name=wl_radius_ipaddr_2 value='<% get_single_ip("wl_radius_ipaddr","2"); %>' onBlur=valid_range(this,0,255,"IP") class=num> .
                  <INPUT size=3 maxlength=3 name=wl_radius_ipaddr_3 value='<% get_single_ip("wl_radius_ipaddr","3"); %>' onBlur=valid_range(this,1,254,"IP") class=num></TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
        <TR>
          <TD bgColor=#e7e7e7 height=25></TD>
          <TD background=image/UI_04.gif height=25></TD>
          <TD></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.radiusport)</script>:&nbsp;</TD>
          <TD height=25>&nbsp;<INPUT size=3 name=wl_radius_port value='<% nvram_get("wl_radius_port"); %>' maxlength=5 onBlur=valid_range(this,1,65535,"Port")>
</TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
       <TR>
          <TD bgColor=#e7e7e7 height=25></TD>
          <TD background=image/UI_04.gif height=25></TD>
          <TD></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.sharekey)</script>:&nbsp;</TD>
          <TD height=25>&nbsp;<INPUT size=20 name=wl_radius_key value='<% nvram_get("wl_radius_key"); %>' maxlength=79>
</TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
        <TR>
          <TD bgColor=#e7e7e7 height=25></TD>
          <TD background=image/UI_04.gif height=25></TD>
          <TD></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.keyretimeout)</script>:&nbsp;</TD>
          <TD height=25>&nbsp;<INPUT maxLength=5 name=wl_wpa_gtk_rekey size=10 value='<% nvram_get("wl_wpa_gtk_rekey"); %>' onBlur=valid_range(this,600,7200,"rekey%20interval")>&nbsp;<script>Capture(wlansec.seconds)</script></TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
<!--TR>
          <TD bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD height=25>&nbsp;</TD>
          <TD height=25>&nbsp;WPA2 &nbsp;Preauthentication:</TD>
          <TD height=25>&nbsp;<SELECT name=wl_preauth> 
                     <OPTION value=enabled <% nvram_match("wl_preauth","1","selected"); %>>Enabled</OPTION>
		     <OPTION value=disabled <% nvram_match("wl_preauth","0","selected"); %>>Disabled</OPTION></SELECT></TD>
          <TD height=25>&nbsp;</TD>
          <TD background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD height=25>&nbsp;</TD>
          <TD height=25>&nbsp;Network Re-auth &nbsp;Interval:</TD>
          <TD height=25>&nbsp;<INPUT maxLength=10 name=wl_net_reauth size=10 value='<% nvram_get("wl_net_reauth"); %>' onBlur=valid_range(this,0,999999,"rekey%20interval")> seconds</TD>
          <TD height=25>&nbsp;</TD>
          <TD background=image/UI_05.gif height=25>&nbsp;</TD></TR-->
