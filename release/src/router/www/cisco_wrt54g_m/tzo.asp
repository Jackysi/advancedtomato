        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(ddns.emailaddr)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>

<input name="ddns_username_2" size=30 maxlength=32 value='<% nvram_get("ddns_username_2"); %>' onFocus="check_action(this,0)" onBlur="check_email(this);">

          </FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>    
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(share.tzo_passwd)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>

<input type=password name="ddns_passwd_2" size=30 maxlength=32 value='<% nvram_invmatch("ddns_passwd_2","","d6nw5v1x2pc7st9m"); %>' onFocus="check_action(this,0)" onBlur=valid_name(this,"Password")>

          </FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(share.domainname)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>

<input name="ddns_hostname_2" size=35 maxlength=48 value='<% nvram_get("ddns_hostname_2"); %>' onFocus="check_action(this,0)" onBlur=valid_name(this,"Domain%20Name")></b>

          </FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
