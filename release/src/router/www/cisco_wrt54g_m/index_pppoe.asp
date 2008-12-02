
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
          <TD width=101 height=25>&nbsp;<script>Capture(share.usrname)</script>:&nbsp;</TD>
          <TD width=296 height=25><INPUT maxLength=63 size=24 name="ppp_username" value='<% nvram_get("ppp_username"); %>' onBlur=valid_name(this,"User%20Name")></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(share.passwd)</script>:&nbsp;</TD>
	  <TD width=296 height=25><INPUT maxLength=63 size=24 name="ppp_passwd" value='<% nvram_invmatch("ppp_passwd","","d6nw5v1x2pc7st9m"); %>' type=password onBlur=valid_name(this,"Password")></TD>
	  <TD height=25>&nbsp;</TD>
	  <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(share.srvname)</script>:&nbsp;</TD>
	  <TD width=296 height=25><INPUT maxLength=63 size=24 name="ppp_service" value='<% nvram_get("ppp_service"); %>' onBlur=valid_name(this,"")></TD>
	  <TD height=25>&nbsp;</TD>
	  <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
	<TR>
	  <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
	  <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
	  <TD colSpan=3 height=25>&nbsp;</TD>
	  <TD width=397 height=25 colspan="2"><INPUT type=radio value=1 name="ppp_demand" <% nvram_match("ppp_demand","1","checked"); %> onclick=ppp_enable_disable(this.form,1) ><font color=<% nvram_else_match("aol_block_traffic1","1","gray","black"); %>><script>Capture(setupcontent.conndemand)</script>&nbsp;</font>
	<INPUT class=num maxLength=4 size=4 value='<% nvram_get("ppp_idletime"); %>' name="ppp_idletime" onBlur=valid_range(this,1,9999,"Idle%20time")>&nbsp;<font color=<% nvram_else_match("aol_block_traffic1","1","gray","black"); %>><script>Capture(setupcontent.min)</script></font></TD>
          </TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
       <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=397 height=25 colspan="2"><INPUT type=radio value=0 name="ppp_demand" <% nvram_match("ppp_demand","0","checked"); %> onclick=ppp_enable_disable(this.form,0)><font color=<% nvram_else_match("aol_block_traffic1","1","gray","black"); %>><script>Capture(setupcontent.keepalive)</script>&nbsp;</font>
	<INPUT class=num maxLength=4 size=4 value='<% nvram_get("ppp_redialperiod"); %>' name="ppp_redialperiod" onBlur=valid_range(this,20,180,"Redial%20period")>&nbsp;<font color=<% nvram_else_match("aol_block_traffic1","1","gray","black"); %>><script>Capture(setupcontent.sec)</script></font></TD>
          <TD height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
