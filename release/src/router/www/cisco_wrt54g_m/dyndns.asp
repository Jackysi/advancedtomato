        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(ddns.system)</script>:&nbsp;</TD>
          <TD width=296 height=25><SELECT name="ddns_service"> 
		<OPTION value=dyndns <% nvram_selmatch("ddns_service", "dyndns", "selected"); %> selected><b><script>Capture(ddns.dynamic)</script></b></OPTION> 
      		<OPTION value=dyndns-static <% nvram_selmatch("ddns_service", "dyndns-static", "selected"); %>><b><script>Capture(ddns.static)</script></b></OPTION>
      		<OPTION value=dyndns-custom <% nvram_selmatch("ddns_service", "dyndns-custom", "selected"); %>><b><script>Capture(ddns.custom)</script></b></OPTION>
	</SELECT>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
	<TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(share.usrname)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>

<input name="ddns_username" size=30 maxlength=32 value='<% nvram_get("ddns_username"); %>' onFocus="check_action(this,0)" onBlur=valid_name(this,"User%20Name")>
	  </FONT></TD>

          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(share.passwd)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>

<input type=password name="ddns_passwd" size=30 maxlength=32 value='<% nvram_invmatch("ddns_passwd","","d6nw5v1x2pc7st9m"); %>' onFocus="check_action(this,0)" onBlur=valid_name(this,"Password")>
	  </FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(share.hostname)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>

<input name="ddns_hostname" size=35 maxlength=128 value='<% nvram_get("ddns_hostname"); %>' onFocus="check_action(this,0)" onBlur=valid_name(this,"Host%20Name")></b>

          </FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25></TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>&nbsp;<script>Capture(ddns.mailexchange)</script>:&nbsp;</FONT></TD>
          <TD width=296 height=25><FONT style="FONT-SIZE: 8pt" face=Arial>

<input name="ddns_mx" size=35 maxlength=63 value='<% nvram_get("ddns_mx"); %>' onFocus="check_action(this,0)" onBlur=valid_name(this,"Mail%20Exchange")></b>

          </FONT></TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(ddns.backupmx)</script>:&nbsp;</TD>
          <TD width=296 height=25><SELECT name="ddns_backmx"> 
		<OPTION value=YES <% nvram_selmatch("ddns_backmx", "YES", "selected"); %>><b><script>Capture(share.yes)</script></b></OPTION> 
      		<OPTION value=NO <% nvram_selmatch("ddns_backmx", "NO", "selected"); %>><b><script>Capture(share.no)</script></b></OPTION>
	</SELECT>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
        <TR>
          <TD width=156 bgColor=#e7e7e7 colSpan=3 height=25>&nbsp;</TD>
          <TD width=8 background=image/UI_04.gif height=25>&nbsp;</TD>
          <TD colSpan=3 height=25>&nbsp;</TD>
          <TD width=101 height=25>&nbsp;<script>Capture(ddns.wildcard)</script>:&nbsp;</TD>
          <TD width=296 height=25><SELECT name="ddns_wildcard">
		<OPTION value=ON <% nvram_selmatch("ddns_wildcard", "ON", "selected"); %>><b><script>Capture(hwlad2.on)</script></b></OPTION> 
      		<OPTION value=OFF <% nvram_selmatch("ddns_wildcard", "OFF", "selected"); %>><b><script>Capture(hwlad2.off)</script></b></OPTION>
	</SELECT>&nbsp;</TD>
          <TD width=13 height=25>&nbsp;</TD>
          <TD width=15 background=image/UI_05.gif height=25>&nbsp;</TD></TR>
