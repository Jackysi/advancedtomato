
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

<HTML><HEAD><TITLE>Port Triggering</TITLE>
<% no_cache(); %>
<% charset(); %>
<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/layout.js"></SCRIPT>

<SCRIPT language=javascript>
function to_submit(F)
{
	F.submit_button.value = "PortTriggerTable";
	F.action.value = "Apply";
        F.submit();
}
</SCRIPT>

</HEAD>
<BODY bgColor=white>
<FORM name=macfilter method=<% get_http_method(); %> action=apply.cgi>
<input type=hidden name=submit_button>
<input type=hidden name=action>
<input type=hidden name=port_trigger>
<input type=hidden name=small_screen>

<CENTER>
<TABLE cellSpacing=0 cellPadding=10 width=515 border=1>
  <TBODY>
  <TR>
    <TD width=521>
      <TABLE height=320 cellSpacing=0 cellPadding=0 border=0>
        <TBODY>
        <tr>
	<td align="center" colspan="4" height="40">
            <P align=center><B><FONT face=Arial color=#0000ff size="4">Port Trigger List</FONT></B></P></td>
	    </tr>
        <tr>
	<td align="center" height="30">&nbsp;</td>
	<td width="130" align="center" height="30">
    <p style="margin-top: 1; margin-bottom: 1"><font face="Arial" size="2"><b>Application Name</font></td>
	<td width="170" align="center" height="30">
    <p style="margin-top: 1; margin-bottom: 1"><font face="Arial" size="2"><b>Outgoing Port Range</font></td>
	<td width="170" align="center" height="30">
    <p style="margin-top: 1; margin-bottom: 1"><font face="Arial" size="2"><b>Incoming Port Range</font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>01:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name0 value='<% port_trigger_table("name","0"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from0 size=5 maxlength=5 value='<% port_trigger_table("i_from","0"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to0 size=5 maxlength=5 value='<% port_trigger_table("i_to","0"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from0 size=5 maxlength=5 value='<% port_trigger_table("o_from","0"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to0 size=5 maxlength=5 value='<% port_trigger_table("o_to","0"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>02:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name1 value='<% port_trigger_table("name","1"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from1 size=5 maxlength=5 value='<% port_trigger_table("i_from","1"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to1 size=5 maxlength=5 value='<% port_trigger_table("i_to","1"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from1 size=5 maxlength=5 value='<% port_trigger_table("o_from","1"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to1 size=5 maxlength=5 value='<% port_trigger_table("o_to","1"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>03:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name2 value='<% port_trigger_table("name","2"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from2 size=5 maxlength=5 value='<% port_trigger_table("i_from","2"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to2 size=5 maxlength=5 value='<% port_trigger_table("i_to","2"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from2 size=5 maxlength=5 value='<% port_trigger_table("o_from","2"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to2 size=5 maxlength=5 value='<% port_trigger_table("o_to","2"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>04:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name3 value='<% port_trigger_table("name","3"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from3 size=5 maxlength=5 value='<% port_trigger_table("i_from","3"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to3 size=5 maxlength=5 value='<% port_trigger_table("i_to","3"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from3 size=5 maxlength=5 value='<% port_trigger_table("o_from","3"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to3 size=5 maxlength=5 value='<% port_trigger_table("o_to","3"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>05:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name4 value='<% port_trigger_table("name","4"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from4 size=5 maxlength=5 value='<% port_trigger_table("i_from","4"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to4 size=5 maxlength=5 value='<% port_trigger_table("i_to","4"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from4 size=5 maxlength=5 value='<% port_trigger_table("o_from","4"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to4 size=5 maxlength=5 value='<% port_trigger_table("o_to","4"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>06:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name5 value='<% port_trigger_table("name","5"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from5 size=5 maxlength=5 value='<% port_trigger_table("i_from","5"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to5 size=5 maxlength=5 value='<% port_trigger_table("i_to","5"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from5 size=5 maxlength=5 value='<% port_trigger_table("o_from","5"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to5 size=5 maxlength=5 value='<% port_trigger_table("o_to","5"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>07:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name6 value='<% port_trigger_table("name","6"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from6 size=5 maxlength=5 value='<% port_trigger_table("i_from","6"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to6 size=5 maxlength=5 value='<% port_trigger_table("i_to","6"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from6 size=5 maxlength=5 value='<% port_trigger_table("o_from","6"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to6 size=5 maxlength=5 value='<% port_trigger_table("o_to","6"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>08:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name7 value='<% port_trigger_table("name","7"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from7 size=5 maxlength=5 value='<% port_trigger_table("i_from","7"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to7 size=5 maxlength=5 value='<% port_trigger_table("i_to","7"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from7 size=5 maxlength=5 value='<% port_trigger_table("o_from","7"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to7 size=5 maxlength=5 value='<% port_trigger_table("o_to","7"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>09:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name8 value='<% port_trigger_table("name","8"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from8 size=5 maxlength=5 value='<% port_trigger_table("i_from","8"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to8 size=5 maxlength=5 value='<% port_trigger_table("i_to","8"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from8 size=5 maxlength=5 value='<% port_trigger_table("o_from","8"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to8 size=5 maxlength=5 value='<% port_trigger_table("o_to","8"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        <tr>
	    <td align="center" height="27"><font face="Arial" size="2"><b>10:</b></font></td>
	    <td align="center" height="27"><FONT size=2 face="Arial">
		<INPUT class=num maxLength=12 size=12 name=name9 value='<% port_trigger_table("name","9"); %>' onBlur=valid_name(this,"Name")></FONT></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=i_from9 size=5 maxlength=5 value='<% port_trigger_table("i_from","9"); %>' onBlur=valid_range(this,0,65535,"Port")><font size="2">&nbsp;~&nbsp;</font>
		<input name=i_to9 size=5 maxlength=5 value='<% port_trigger_table("i_to","9"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
	    <td align="center" height="27"><font face="Arial">
		<input name=o_from9 size=5 maxlength=5 value='<% port_trigger_table("o_from","9"); %>' onBlur=valid_range(this,0,65535,"Port") ><font size="2">&nbsp;~&nbsp;</font>
		<input name=o_to9 size=5 maxlength=5 value='<% port_trigger_table("o_to","9"); %>' onBlur=valid_range(this,0,65535,"Port")></font></td>
        </tr>

        </TBODY></TABLE></TD></TR></TBODY></TABLE>
<P><INPUT type=button value="  Apply  " onClick=to_submit(this.form)>&nbsp; <INPUT type=reset value=" Cancel ">&nbsp; 
    <INPUT onclick=self.close() type=reset value=" Close "></CENTER></P></FORM></BODY></HTML>
