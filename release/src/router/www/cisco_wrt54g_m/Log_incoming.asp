
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

<HTML><HEAD><TITLE>Incoming Log Table</TITLE>
<% no_cache(); %>
<% charset(); %>
<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capadmin.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>
<script>
document.title = log.inlogtbl;
</script>
</HEAD>
<BODY bgColor=white onload={window.focus();}>
<FORM>
<CENTER>
<TABLE cellSpacing=1 width=537 border=0>
  <TBODY>
  <TR>
    <TH width=400 height=30 bgColor="#FFFFFF" align=left><FONT face=Arial color=blue><B><SPAN STYLE="FONT-SIZE: 14pt"><script>Capture(log.inlogtbl)</script></SPAN></B></FONT></TH>
 	<TH width=137 height=30 bgColor="#FFFFFF" align=right><script>document.write("<INPUT onclick=window.location.reload() type=button value=\"" + sbutton.refresh + "\">");</script></TH>
  </TR>
 </TABLE>
 <TABLE cellSpacing=1 width=537 border=0> 
  <TR align=middle bgColor=#b3b3b3>
    <TH width=280 height="16" bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(log.srcip)</script></SPAN></FONT></TH>
    <TH width=280 height="16" bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(log.desportnum)</script></SPAN></FONT></TH></TR>
<% dumplog("incoming"); %>
  <TR align=middle bgColor=#b3b3b3>
    <TH width=280 height="35" bgcolor="#FFFFFF"></TH>
    <TH width=280 height="35" bgcolor="#FFFFFF" align="right"> 

<script>document.write("<INPUT onclick=self.close() type=reset name=close_button value=\"" + sbutton.close + "\">");</script>

    </TH></TR>
    </TBODY></TABLE></CENTER>&nbsp; </FORM></BODY></HTML>
