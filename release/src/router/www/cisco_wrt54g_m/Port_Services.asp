
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

<HTML><HEAD><TITLE>Port Services</TITLE>
<% no_cache(); %>
<% charset(); %>
<link rel="stylesheet" type="text/css" href="style.css">
<style fprolloverstyle>
A:hover {color: #00FFFF}
.small A:hover {color: #00FFFF}
</style>

<SCRIPT src="common.js"></SCRIPT>
<SCRIPT language="Javascript" type="text/javascript" src="lang_pack/capsec.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/share.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/help.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capapp.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capasg.js"></SCRIPT>
<SCRIPT language="javascript" type="text/javascript" src="lang_pack/capwrt54g.js"></SCRIPT>

<SCRIPT language=javascript>
document.title = secbutton.addedit;
function service(id, name, port_start, port_end, protocol){
	this.id = id;
	this.name = name;
	this.start = port_start;
	this.end = port_end;
	this.protocol = protocol;
	this.deleted = false;
	this.modified = false;
}
services=new Array();
services_length=0;
/* Init. services data structure */
<% filter_port_services_get("all_list", "0"); %>


function setValue(){
      	document.PortServices.ServiceTable.selectedIndex = 0;
	document.PortServices.Add_Service_Name.value =
		services[document.PortServices.ServiceTable.options[document.PortServices.ServiceTable.selectedIndex].value].name;
	switch(services[document.PortServices.ServiceTable.options[
		document.PortServices.ServiceTable.selectedIndex].value].protocol){
	 case "icmp":
	 case 1:
		document.PortServices.Add_Service_Protocol.selectedIndex = 0;
                break;
	 default:
	 case "tcp":
	 case 6:
		document.PortServices.Add_Service_Protocol.selectedIndex = 1;
		break;
	 case "udp":
	 case 17:
		document.PortServices.Add_Service_Protocol.selectedIndex = 2;
		break;
	 case "both":
	 case 23:
	 	document.PortServices.Add_Service_Protocol.selectedIndex = 3;
		break;
	}
	document.PortServices.Add_Service_Port_S.value =
		services[document.PortServices.ServiceTable.options[document.PortServices.ServiceTable.selectedIndex].value].start;
	document.PortServices.Add_Service_Port_E.value =
		services[document.PortServices.ServiceTable.options[document.PortServices.ServiceTable.selectedIndex].value].end;
}

function onChange_ServiceTable(index, name, protocol, start, end){ 
	name.value = services[index].name;
	switch(services[index].protocol){
	 case "icmp":
	 case 1:
	 case "1":
		protocol.selectedIndex = 0;
                break;
	 case "tcp":
	 case 6:
	 case "6":
		protocol.selectedIndex = 1;
		break;
	 case "udp":
	 case 17:
	 case "17":
		protocol.selectedIndex = 2;
		break;
	 case "both":
	 case 23:
	 case "23":
		protocol.selectedIndex = 3;
		break;
	}
	if(services[index].protocol == 1){
		choose_disable(start);
		choose_disable(end);
	}
	else{
		choose_enable(start);
                choose_enable(end);
	}

	start.value = services[index].start;
	end.value = services[index].end;
}

function onClick_Delete(){
//      if(!confirm("After finished all actions, click the Apply button to save the settings."))
        if(!confirm(errmsg2.err11))
		return false;
  var index = document.PortServices.ServiceTable.options[document.PortServices.ServiceTable.selectedIndex].value;
	services[index].deleted = true;
	document.PortServices.ServiceTable.options[document.PortServices.ServiceTable.selectedIndex] = null;
	document.PortServices.ServiceTable.selectedIndex = 0;
	setValue();
}

function onClick_Modify(){
//      if(!confirm("After finished all actions, click the Apply button to save the settings."))
        if(!confirm(errmsg2.err11))
		return false;
 var index1 = document.PortServices.ServiceTable.options[document.PortServices.ServiceTable.selectedIndex].value;
 var index2 = document.PortServices.ServiceTable.selectedIndex;
	services[index1].modified = true;
	services[index1].name = document.PortServices.Add_Service_Name.value;
	services[index1].protocol = document.PortServices.Add_Service_Protocol.options[
							document.PortServices.Add_Service_Protocol.selectedIndex].value.valueOf();
        if(Number(document.PortServices.Add_Service_Port_S.value) > Number(document.PortServices.Add_Service_Port_E.value)){
                var start = document.PortServices.Add_Service_Port_S.value;
                var end = document.PortServices.Add_Service_Port_E.value;
                document.PortServices.Add_Service_Port_S.value = end;
                document.PortServices.Add_Service_Port_E.value = start;
        }
	services[index1].start = document.PortServices.Add_Service_Port_S.value;
	services[index1].end = document.PortServices.Add_Service_Port_E.value;

	document.PortServices.ServiceTable.options[index2].text = services[index1].name + "[ "+
				services[index1].start+" ~ "+services[index1].end+" ]";
	
}

function onClick_Add(){
	if(document.PortServices.Add_Service_Name.value == ""){
//                alert("You must input a Service Name!");
                alert(errmsg2.err12);
                document.PortServices.Add_Service_Name.focus();
                return false;
        }
 	for(i=0 ; i<services.length ; i++){
		if(services[i].deleted == true)	continue;
		if(document.PortServices.Add_Service_Name.value == services[i].name){
//                      alert("The Service Name have exist!");
                        alert(errmsg2.err13);
			document.PortServices.Add_Service_Name.focus();
			return false;
		}
 	}

//      if(!confirm("After finished all actions, click the Apply button to save the settings."))
        if(!confirm(errmsg2.err11))
		return false;
	if(Number(document.PortServices.Add_Service_Port_S.value) > Number(document.PortServices.Add_Service_Port_E.value)){
		var start = document.PortServices.Add_Service_Port_S.value;
		var end = document.PortServices.Add_Service_Port_E.value;
		document.PortServices.Add_Service_Port_S.value = end;
		document.PortServices.Add_Service_Port_E.value = start;
	}
	if(document.PortServices.Add_Service_Protocol[
                                        document.PortServices.Add_Service_Protocol.selectedIndex].value == "1"){	// icmp
		document.PortServices.Add_Service_Port_S.value = "0";
		document.PortServices.Add_Service_Port_E.value = "0";
	}

	services[services_length] = new service(-1, 
			document.PortServices.Add_Service_Name.value, document.PortServices.Add_Service_Port_S.value, 
			document.PortServices.Add_Service_Port_E.value, 
			document.PortServices.Add_Service_Protocol.options[
					document.PortServices.Add_Service_Protocol.selectedIndex].value);
	services[services_length].modified = true;
	document.PortServices.ServiceTable.options[document.PortServices.ServiceTable.length] = 
							new Option(services[services_length].name + 
							" [ "+services[services_length].start+" ~ "+services[services_length].end+" ]",
							services_length, 0, 1);
	services_length ++;
}

function onClick_Apply(F){
 var i;
 	for(i=0 ; i<services.length ; i++){
		if(services[i].deleted == true)	continue;
 		//F.services_array .value += services[i].id+":";
 		F.services_array.value += services[i].start+":";
 		F.services_array.value += services[i].end+":";
 		F.services_array.value += services[i].protocol+":";
 		F.services_array.value += services[i].name+"(&nbsp;)";
 		//F.services_array.value += services[i].deleted+":";
 		//F.services_array.value += services[i].modified +"|";
 	}
 	F.services_length.value = services.length;
	F.submit();
}
/*
function onClick_close(){
	top.opener.location.href="Filters.asp";
	self.close();
}
*/
function onChange_Prot(F,I)
{
	if(I == "1"){
		choose_disable(F.Add_Service_Port_S);
		choose_disable(F.Add_Service_Port_E);
	}
	else{
		choose_enable(F.Add_Service_Port_S);
		choose_enable(F.Add_Service_Port_E);
	}
}

</SCRIPT>

<BODY bgColor=#808080 onLoad="setValue()" onUnload="top.opener.window.location.href='Filters.asp'">
<CENTER>

<FORM action="apply.cgi" method="get" name="PortServices">
<input name="submit_button" type="HIDDEN" value="Port_Services">
<input name="change_action" type="HIDDEN" value="gozila_cgi">
<input name="submit_type" type="HIDDEN" value="save_services">
<input type="HIDDEN" name="services_array">
<input type="HIDDEN" name="services_length">
<TABLE borderColor=#111111 height=282 cellSpacing=0 cellPadding=0 width=449 bgColor=#ffffff border=0>
  <TBODY>
  <TR>
    <TH width=862 colSpan=2 height=282 bordercolor="#FFFFFF" >
      <TABLE height=296 cellSpacing=3 width=582 bgColor=#ffffff border=0>
        <TBODY>
        <TR>
          <TH width=742 height=232>
            <TABLE height=280 cellSpacing=0 width=355 bgColor=#ffffff
              border=0 style="border-collapse: collapse" bordercolor="#111111" cellpadding="0"><TBODY>
              <TR>
                <TH vAlign=bottom align=right width=759 bgColor=#FFFFFF
                height=16 >
                <div align="center">
                  <table border="0" cellpadding="0" cellspacing="0" style="border-collapse: collapse" bordercolor="#111111" id="AutoNumber1" width="570" height="24">

                    <!--tr>
                      <td height="16" colspan="2" bgcolor="#6666CC" width="153"> &nbsp;</td>
                      <td height="16" colspan="4" bgcolor="#6666CC" width="417"> &nbsp;</td>
                    </tr>

                    <tr>
                      <td height="1" colspan="6" bgcolor="#FFFFFF" width="570"> </td>
                    </tr>

                    <tr>
                      <td height="91" colspan="2" bgcolor="#000000" width="153">
                      <p align="center"><b>
                      <font color="#FFFFFF" style="font-size: 15pt">Port Services</font></b></td>
                      <td height="91" colspan="4" bgcolor="#6666CC" width="417">
                      <table border="0" cellpadding="0" cellspacing="0" style="border-collapse: collapse" bordercolor="#111111" id="AutoNumber2" width="387" height="76">
                        <tr>
                          <td width="13" height="76">&nbsp;</td>
                          <td width="374" height="76">
                  <p style="margin-top: 1px; margin-bottom: 1px" align="justify">
                  <font color="#FFFFFF" size="1"><span style="FONT-FAMILY: Arial"> 
                  Enter a passphrase to
                  automatically generate 64 or 128-bit WEP keys. The Passphrase is case-sensitive, and should have
                  16 characters or fewer. If you are not using a Passphrase, then manually enter the WEP keys in hexadecimal characters,
                  ?0?-?9? and A?-F?.</span></font></p>
                          </td>
                        </tr>
                      </table>
                      </td>
                    </tr>
                    
                    <tr>
                      
                      <td height="16" width="570" colspan="6">
			<img border="0" src="image/UI_02.gif" width="571" height="34"></td>
                    </tr-->

                    <tr>
                      <td height="16" bgcolor="#FFFFFF" width="570" colspan="6">
                      <table width="570">
                      <tr>
                      	<td width=30></td>
                      	<td width=510>
                      	<table cellSpacing=1 cellPadding=0 border=0 align=center>

                      		<TABLE cellSpacing=0 cellPadding=10 width=500 border=1 height="185">
  <TBODY>
  <TR>
    <TD width=300 height="163">
      <b><font face="Arial" size="2">&nbsp;</font></b><b><font face=Arial color=black>&nbsp; <script>Capture(share.srvname)</script><br>&nbsp; &nbsp;
      <input name=Add_Service_Name size=12 maxlength=12 onBlur=valid_name(this,"Service%20Name")><br><br>
      &nbsp; &nbsp;<script>Capture(share.protocol)</script><br>&nbsp; &nbsp;
      <select name=Add_Service_Protocol onChange=onChange_Prot(this.form,Add_Service_Protocol.options[Add_Service_Protocol.selectedIndex].value)>
	<option value=1>ICMP</option>
	<option value=6>TCP</option>
	<option value=17>UDP</option>
        <option value=23>TCP & UDP</option>
      </select>
      <br><br>
      &nbsp; &nbsp;<script>Capture(portsrv.portrange)</script><br>&nbsp; &nbsp;
      <input name=Add_Service_Port_S size=5 maxlength=5 onBlur=valid_range(this,0,65535,"Port") class=num value=0> ~
      <input name=Add_Service_Port_E size=5 maxlength=5 onBlur=valid_range(this,0,65535,"Port") class=num value=0><br><br>&nbsp;
      &nbsp;

<script>document.write("<input type=button name=add_button onClick=onClick_Add() value=\"" + portsrv.add + "\">");</script>

        &nbsp;

<script>document.write("<input type=button name=modify_button onClick=onClick_Modify() value=\"" + portsrv.modify + "\">");</script>

        &nbsp;

<script>document.write("<input type=button name=delete_button onClick=onClick_Delete() value=\"" + sbutton.del + "\">");</script>

      </font></b></TD>
    <TD width=140 height="163">
      <font face="Arial"><b>
      <select name="ServiceTable" size=12 onchange="onChange_ServiceTable(
ServiceTable.options[ServiceTable.selectedIndex].value, 
Add_Service_Name, Add_Service_Protocol, Add_Service_Port_S, Add_Service_Port_E)">
      <SCRIPT language="javascript">
      var i=0;
	for(;i<services_length;i++)
      		document.write("<option value="+i+">"+services[i].name+ " [ "+
			services[i].start+" ~ "+
			services[i].end + " ]" + "</option>");
      </SCRIPT>
      </select>

      </b></font></TD></TR></TBODY></TABLE>
<P>

<script>document.write("<INPUT type=button name=Submit onClick=onClick_Apply(this.form) value=\"" + hportser2.submit + "\">");</script>

&nbsp;

<script>document.write("<INPUT type=button name=cancel onclick=window.location.reload() value=\"" + portsrv.cancel + "\">");</script>

&nbsp;


<script>document.write("<INPUT onclick=self.close() type=button name=close_button value=\"" + sbutton.close + "\">");</script>

                </div>
                </TH>
                </TR>
              </TBODY></TABLE></TH></TR></TBODY></TABLE>
  </TR>
  </TBODY>
</TABLE>
</FORM>


</CENTER>
</BODY>
</HTML>
