
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

<HTML><HEAD><TITLE>Outgoing Log Table</TITLE>
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
document.title = log.outlogtbl;
</script>
</HEAD>
<BODY bgColor=white onload={window.focus();}>
<FORM>
<CENTER>
<TABLE cellSpacing=1 width=721 border=0>
  <TBODY>
  <TR>
    <TH width=443 height=30 bgColor="#FFFFFF" align=left colspan=2><SPAN STYLE="FONT-SIZE: 14pt"><FONT face=Arial color=blue><B><script>Capture(log.outlogtbl)</script></B></FONT></SPAN></TH>
    <TH width=307 height=30 bgColor="#FFFFFF" align=right>

<script>document.write("<INPUT onclick=window.location.reload() type=button name=refresh_button value=\"" + sbutton.refresh + "\">");</script>

    </TH></TR>
  <TR align=middle bgColor=#b3b3b3>
    <TH width=163 bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(log.lanip)</script></SPAN></FONT></TH>
    <TH width=280 bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(log.desurlip)</script></SPAN></FONT></TH>
    <TH width=307 bgcolor="#C0C0C0"><FONT face=Arial><SPAN STYLE="FONT-SIZE: 10pt"><script>Capture(log.portnum)</script></SPAN></FONT></TH></TR>
<% dumplog("outgoing"); %>
  <TR>
    <TD align=middle></TD>
    <TD align=middle></TD>
    <TD align=right height="35"> 

<script>document.write("<INPUT onclick=self.close() type=reset name=close_button value=\"" + sbutton.close + "\">");</script>

    </TD></TR>
    </TBODY></TABLE></CENTER>
<P> </P></FORM></BODY></HTML>
