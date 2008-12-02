
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
          <TD></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.wpaalg)</script>:&nbsp;</TD>
          <TD height=25>&nbsp;<SELECT name=wl_crypto>
		<script>
		     var enc = '<% nvram_get("wl_crypto"); %>';
		     var now_security_mode2 = document.wpa.security_mode2.options[document.wpa.security_mode2.selectedIndex].value;

			if(now_security_mode2 == "wpa_personal") {
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
                  </SELECT></TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
        <TR>
          <TD bgColor=#e7e7e7 height=25></TD>
          <TD background=image/UI_04.gif height=25></TD>
          <TD ></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.wpaskey)</script>:&nbsp;</TD>
          <TD height=25>&nbsp;<INPUT size=32 name=wl_wpa_psk value='<% nvram_get("wl_wpa_psk"); %>' maxlength=64></TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
        <TR>
          <TD bgColor=#e7e7e7 height=25></TD>
          <TD background=image/UI_04.gif height=25></TD>
          <TD></TD>
          <TD height=25>&nbsp;<script>Capture(wlansec.groupkey)</script>:&nbsp;</TD>
          <TD height=25>&nbsp;<INPUT maxLength=5 name=wl_wpa_gtk_rekey size=10 value='<% nvram_get("wl_wpa_gtk_rekey"); %>' onBlur=valid_range(this,600,7200,"rekey%20interval")> <script>Capture(wlansec.seconds)</script></TD>
          <TD height=25></TD>
          <TD background=image/UI_05.gif height=25></TD></TR>
