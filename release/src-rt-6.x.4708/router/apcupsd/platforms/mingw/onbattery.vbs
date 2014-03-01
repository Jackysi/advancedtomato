'///Wriiten by Ed Dondlinger 1/23/2009 - edondlinger@thepylegroup.com ///

'/// MODIFY THE VARIABLES LISTED BELOW IN THE "USER VARIABLES" SECTION.
'/// THEN RENAME FILE WITHOUT THE ".example" SUFFIX.

'///Comment out the next line when testing, then change back when done. ///
On Error Resume Next

Dim Get_Status
Dim oShell
set oShell = CreateObject("WScript.Shell") 

'/// Get APCUPS Status Report ///

set Get_Status = oShell.exec("%comspec% /c c:\Progra~1\apcupsd\bin\apcaccess.exe status")
UPS_Status = Get_Status.StdOut.readall

'/// Get Time Zone info from local windows registry ///

atb = "HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\TimeZoneInformation\StandardName" 
TZInfo = oShell.RegRead(atb)



'XXXXXXXXXXXXXXXXXXXX  USER VARIABLES  XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

MyLocation = "Home"

MyEmailSubject = "POWER FAILURE at: " & MyLocation

MyEmailFromAddress = "xxx@zzz.com"  'you could also use this format to display the common name: """Me"" <user@mydomain.com>" 

MyEmailToAddress = "xxx@zzz.com"

MyTextBody = "The utility power has failed at " & MyLocation & " and the UPS is running on batteries." & _
		vbCRLF & "Reported by the APCUPS Server at: " & dateAdd("H",0,Now) & " " & _
		TZInfo & vbcrlf & vbcrlf & UPS_Status

MySMTPServer = "smtp.gmail.com"  'or use a IP address i.e. "10.x.x.x"

MyMailServerUserName = "UName"

MyMailServerPW = "password"

MySMTPServerPort = 465

MySMTPServerSSL = True

'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXxXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX




'////////// Email Section  //////////

'*** NOTE: MSDN CDO Library can be referenced at: http://msdn.microsoft.com/en-us/library/ms872853(EXCHG.65).aspx

Const cdoSendUsingPickup = 1 'Send message using the local SMTP service pickup directory. 
Const cdoSendUsingPort = 2 'Send the message using the network (SMTP over the network).
Const cdoSendUsingExchange = 3  'Send the message using MS Exchange Mail Server. 

Const cdoAnonymous = 0 'Do not authenticate
Const cdoBasic = 1 'basic (clear-text) authentication
Const cdoNTLM = 2 'NTLM

Const cdoURL = "http://schemas.microsoft.com/cdo/configuration/"

Set objMessage = CreateObject("CDO.Message") 
	
	'/// Set the message properties. ///
	
	With objMessage

		.Subject = MyEmailSubject 
		
		.From = MyEmailFromAddress 
		
		.To = MyEmailToAddress 
		
		.TextBody = MyTextBody
		
		'Set Mail Importance level, you can set [high,normal,low]
		.fields.Item("urn:schemas:mailheader:importance").Value = "high"
		
		'Set Mail priority level, you can set [1=urgent, 0=normal, -1=nonurgent]
       		.fields.Item("urn:schemas:mailheader:priority").Value = 1

		.fields.Update()
 
	End with

	'/// This section provides the configuration information for the remote SMTP server. ///

	with objMessage.Configuration.Fields

		'Send Using method (1 = local smtp, 2 = remote network smtp, 3 = MS Exchange)
		.Item(cdoURL & "sendusing") = 2 

		'Name or IP of Remote SMTP Server
		.Item(cdoURL & "smtpserver") = MySMTPServer

		'Type of authentication, NONE, Basic (Base64 encoded), NTLM
		.Item(cdoURL & "smtpauthenticate") = cdoBasic

		'Your UserID on the SMTP server
		.Item(cdoURL & "sendusername") = MyMailServerUserName

		'Your password on the SMTP server
		.Item(cdoURL & "sendpassword") = MyMailServerPW

		'Server port (typically 25)
		.Item(cdoURL & "smtpserverport") = MySMTPServerPort 

		'Use SSL for the connection (False or True)
		.Item(cdoURL & "smtpusessl") = MySMTPServerSSL

		'Connection Timeout in seconds (the maximum time CDO will try to establish a connection to the SMTP server)
		.Item(cdoURL & "smtpconnectiontimeout") = 60

		'Update the config.
		.Update

	end with


'/// Send the message ///

objMessage.Send 


'/// Clean-up ///

Set objMessage = nothing
set oShell = nothing
set Get_Status = nothing

'/// Exit with a 0 error level to ensure the apccontrol.bat continues ///

Wscript.Quit 0