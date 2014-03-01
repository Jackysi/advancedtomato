; winapcupsd.nsi
;
; Adapted by Kern Sibbald for apcupsd from Bacula code
; Further modified by Adam Kropelin
;

;
; Basics
;
Name "Apcupsd"
OutFile "winapcupsd-${VERSION}.exe"
SetCompressor lzma
InstallDir "c:\apcupsd"

;			    
; Include files
;
!include "MUI.nsh"
!include "LogicLib.nsh"
!include "StrReplace.nsh"
!include "WinVer.nsh"
!include "DrvSetup.nsh"

; Global variables
Var ExistingConfig
Var MainInstalled
Var TrayInstalled
Var OrigInstDir

; Paths
!define WINDIR ${TOPDIR}/src/win32

; Icon indexes in shell32.dll
!define START_ICON_INDEX  137
!define STOP_ICON_INDEX   131
!define CONFIG_ICON_INDEX 21
!define HELP_ICON_INDEX   23
!define MANUAL_ICON_INDEX 23

;
; Pull in pages
;
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE ${TOPDIR}/COPYING
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
Page custom EditApcupsdConfEnter EditApcupsdConfExit ""
Page custom InstallServiceEnter InstallServiceExit ""
Page custom ApctrayEnter ApctrayExit ""
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View the ReleaseNotes"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION "ShowReadme"
!define MUI_FINISHPAGE_LINK "Visit Apcupsd Website"
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.apcupsd.org"
!define MUI_PAGE_CUSTOMFUNCTION_SHOW DisableBackButton
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
 
!define      MUI_ABORTWARNING

!insertmacro MUI_LANGUAGE "English"

DirText "Setup will install Apcupsd ${VERSION} to the directory \
         specified below."

; Disable back button
!define FINISH_BUTTON_ID 3
Function DisableBackButton
	GetDlgItem $R1 $HWNDPARENT ${FINISH_BUTTON_ID}
	EnableWindow $R1 0
FunctionEnd

; Post-process apcupsd.conf.in by replacing @FOO@ tokens
; with proper values.
Function PostProcConfig
  FileOpen $0 "$INSTDIR\etc\apcupsd\apcupsd.conf.in" "r"
  FileOpen $1 "$INSTDIR\etc\apcupsd\apcupsd.conf.new" "w"

  ClearErrors
  FileRead $0 $2

  ${DoUntil} ${Errors}
    ${StrReplace} $2 "@VERSION@"    "${VERSION}"           $2
    ${StrReplace} $2 "@sysconfdir@" "$INSTDIR\etc\apcupsd" $2
    ${StrReplace} $2 "@PWRFAILDIR@" "$INSTDIR\etc\apcupsd" $2
    ${StrReplace} $2 "@LOGDIR@"     "$INSTDIR\etc\apcupsd" $2
    ${StrReplace} $2 "@nologdir@"   "$INSTDIR\etc\apcupsd" $2
    FileWrite $1 $2
    FileRead $0 $2
  ${Loop}

  FileClose $0
  FileClose $1

  Delete "$INSTDIR\etc\apcupsd\apcupsd.conf.in"
FunctionEnd

Function ShowReadme
  ExecShell "open" "$INSTDIR\ReleaseNotes.txt"
FunctionEnd

Function EditApcupsdConfEnter
  ; Skip this page if config file was preexisting
  ${If} $ExistingConfig == 1
    Abort
  ${EndIf}

  ; Also skip if apcupsd main package was not installed
  ${If} $MainInstalled != 1
    Abort
  ${EndIf}

  ; Configure header text and instantiate the page
  !insertmacro MUI_HEADER_TEXT "Edit Configuration File" "Configure Apcupsd for your UPS."
  !insertmacro MUI_INSTALLOPTIONS_INITDIALOG "EditApcupsdConf.ini"
  Pop $R0 ;HWND of dialog

  ; Set contents of text field
  !insertmacro MUI_INSTALLOPTIONS_READ $R0 "EditApcupsdConf.ini" "Field 1" "HWND"
  SendMessage $R0 ${WM_SETTEXT} 0 \
      "STR:The default configuration is suitable for UPSes connected with a USB cable. \
       All other types of connections require editing the client configuration file, \
       apcupsd.conf.$\r$\r\
       Please edit $INSTDIR\etc\apcupsd\apcupsd.conf to fit your installation. \
       When you click the Next button, Wordpad will open to allow you to do this.$\r$\r\
       Be sure to save your changes before closing Wordpad and before continuing \
       with the installation."

  ; Display the page
  !insertmacro MUI_INSTALLOPTIONS_SHOW
FunctionEnd

Function EditApcupsdConfExit
  ; Launch wordpad to edit apcupsd.conf if checkbox is checked
  !insertmacro MUI_INSTALLOPTIONS_READ $R1 "EditApcupsdConf.ini" "Field 2" "State"
  ${If} $R1 == 1
    ExecWait 'notepad "$INSTDIR\etc\apcupsd\apcupsd.conf"'
  ${EndIf}
FunctionEnd

Function InstallServiceEnter
  ; Skip if apcupsd main package was not installed
  ${If} $MainInstalled != 1
    Abort
  ${EndIf}

  ; Configure header text and instantiate the page
  !insertmacro MUI_HEADER_TEXT "Install/Start Service" "Install Apcupsd Service and start it."
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "InstallService.ini"
FunctionEnd

Function InstallServiceExit
  ; Create Start Menu Directory
  CreateDirectory "$SMPROGRAMS\Apcupsd"
  ; Create start menu link for configuring apcupsd
  CreateShortCut "$SMPROGRAMS\Apcupsd\Edit Configuration File.lnk" "notepad" "$INSTDIR\etc\apcupsd\apcupsd.conf" "$SYSDIR\shell32.dll" ${CONFIG_ICON_INDEX}

  ; If installed as a service already, remove it
  ReadRegDWORD $R0 HKLM "Software\Apcupsd" "InstalledService"
  ${If} $R0 == 1
    ExecWait '"$INSTDIR\bin\apcupsd.exe" /quiet /remove'
    Sleep 1000
  ${EndIf}

  ; Install as service and create start menu shortcuts
  !insertmacro MUI_INSTALLOPTIONS_READ $R1 "InstallService.ini" "Field 2" "State"
  ${If} $R1 == 1
    ; Install service
    ExecWait '"$INSTDIR\bin\apcupsd.exe" /install'
    ${If} ${IsNT}
      ; Installed as a service and we're on NT
      CreateShortCut "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "$SYSDIR\net.exe" "start apcupsd" "$SYSDIR\shell32.dll" ${START_ICON_INDEX}
      CreateShortCut "$SMPROGRAMS\Apcupsd\Stop Apcupsd.lnk"  "$SYSDIR\net.exe" "stop apcupsd"  "$SYSDIR\shell32.dll" ${STOP_ICON_INDEX}
    ${Else}
      ; Installed as a service, but not on NT
      CreateShortCut "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "$INSTDIR\bin\apcupsd.exe" "/service" "$SYSDIR\shell32.dll" ${START_ICON_INDEX}
      CreateShortCut "$SMPROGRAMS\Apcupsd\Stop Apcupsd.lnk"  "$INSTDIR\bin\apcupsd.exe" "/kill"    "$SYSDIR\shell32.dll" ${STOP_ICON_INDEX}
    ${EndIf}
  ${Else}
    ; Not installed as a service
    CreateShortCut "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "$INSTDIR\bin\apcupsd.exe" ""       "$SYSDIR\shell32.dll" ${START_ICON_INDEX}
    CreateShortCut "$SMPROGRAMS\Apcupsd\Stop Apcupsd.lnk"  "$INSTDIR\bin\apcupsd.exe" "/kill"  "$SYSDIR\shell32.dll" ${STOP_ICON_INDEX}
  ${EndIf}

  ; Start Apcupsd now, if so requested
  !insertmacro MUI_INSTALLOPTIONS_READ $R2 "InstallService.ini" "Field 4" "State"
  ${If} $R2 == 1
    ExecShell "" "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "" SW_HIDE
  ${Endif}  
FunctionEnd

Function ApctrayEnter
  ; Skip if apctray package was not installed
  ${If} $TrayInstalled != 1
    Abort
  ${EndIf}

  ; If Apctray is already configured to start automatically, start it now
  ; and skip this page
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "Apctray"
  ${If} $R0 != ""
    Exec $R0
    Abort
  ${EndIf}

  ; Configure header text and instantiate the page
  !insertmacro MUI_HEADER_TEXT "Configure Tray Icon" "Configure Apctray icon for your preferences."
  !insertmacro MUI_INSTALLOPTIONS_DISPLAY "Apctray.ini"
FunctionEnd

Function ApctrayExit
  ; We get called when checkbox changes or next button is pressed
  ; Figure out which this is.

  !insertmacro MUI_INSTALLOPTIONS_READ $R1 "Apctray.ini" "Settings" "State"
  ${If} $R1 == 2
    ; Return to the page
    Abort
  ${EndIf}

  ; Regular exit due to Next button press...

  ; (Un)Install apctray
  !insertmacro MUI_INSTALLOPTIONS_READ $R1 "Apctray.ini" "Field 2" "State"
  ${If} $R1 == 1
    ; Install and start
    ExecWait '"$INSTDIR\bin\apctray.exe" $R0 /install'
    Exec '"$INSTDIR\bin\apctray.exe" $R0'
  ${EndIf}
FunctionEnd

Section "-Startup"
  SetShellVarContext all

  ; Check for existing config file
  ${If} ${FileExists} "$INSTDIR\etc\apcupsd\apcupsd.conf"
    StrCpy $ExistingConfig 1
  ${Else}
    StrCpy $ExistingConfig 0
  ${EndIf}

  ; Save the new install path in the registry
  WriteRegStr HKLM "Software\Apcupsd\" "InstDir" "$INSTDIR"

  ; Create base installation directory
  CreateDirectory "$INSTDIR"

  ; Install common files
  SetOutPath "$INSTDIR"
  File COPYING.txt
  File ChangeLog.txt
  File ReleaseNotes.txt

  ; Clean up old non-txt versions of these files
  Delete /REBOOTOK "$INSTDIR\COPYING"
  Delete /REBOOTOK "$INSTDIR\ChangeLog"
  Delete /REBOOTOK "$INSTDIR\ReleaseNotes"

SectionEnd

Section "Apcupsd Service" SecService
  ; We're installing the main package
  StrCpy $MainInstalled 1

  ; Shutdown any apcupsd or apctray that might be running
  ExecWait '"$OrigInstDir\bin\apctray.exe" /kill'
  ExecWait '"$OrigInstDir\bin\apcupsd.exe" /kill'
  DetailPrint "Waiting for apcupsd and apctray to exit..."
  Sleep 3000

  ; Create installation directories
  CreateDirectory "$INSTDIR\bin"
  CreateDirectory "$INSTDIR\driver"
  CreateDirectory "$INSTDIR\etc"
  CreateDirectory "$INSTDIR\etc\apcupsd"
  CreateDirectory "c:\tmp"

  ;
  ; NOTE: If you add new files here, be sure to remove them
  ;       in the uninstaller!
  ;

  SetOutPath "$INSTDIR\bin"
  File ${WINDIR}\mingwm10.dll
  File ${WINDIR}\pthreadGCE.dll
  File ${WINDIR}\apcupsd.exe
  File ${WINDIR}\smtp.exe
  File ${WINDIR}\apcaccess.exe
  File ${WINDIR}\apctest.exe
  File ${WINDIR}\popup.exe 
  File ${WINDIR}\shutdown.exe
  File ${WINDIR}\email.exe
  File ${WINDIR}\background.exe

  !if "${USBTYPE}" == "generic"
    File ${DEPKGS}\lib\libusb0.dll
  !endif

  SetOutPath "$INSTDIR\driver"
  !if "${USBTYPE}" == "generic"
    File ${TOPDIR}\platforms\mingw\apcupsd.inf
    File ${TOPDIR}\platforms\mingw\apcupsd.cat
    File ${TOPDIR}\platforms\mingw\apcupsd_x64.cat
    File ${DEPKGS}\bin\libusb0.sys
    File ${DEPKGS}\bin\libusb0_x64.sys
    File ${DEPKGS}\lib\libusb0.dll
    File ${DEPKGS}\lib\libusb0_x64.dll
    File ${TOPDIR}\platforms\mingw\install.txt
  !else
    File ${TOPDIR}\platforms\mingw\winusb\install.txt
    File ${TOPDIR}\platforms\mingw\winusb\apcupsd.inf
    SetOutPath "$INSTDIR\driver\i386"
    File ${DEPKGS}\..\winddk\redist\wdf\x86\*.dll
    File ${DEPKGS}\..\winddk\redist\winusb\x86\*.dll
    SetOutPath "$INSTDIR\driver\amd64"
    File ${DEPKGS}\..\winddk\redist\wdf\amd64\*.dll
    File ${DEPKGS}\..\winddk\redist\winusb\amd64\*.dll
  !endif

  SetOutPath "$INSTDIR\etc\apcupsd"
  File ${TOPDIR}\platforms\mingw\apccontrol.bat
  File ${TOPDIR}\platforms\mingw\apcupsd.conf.in
  File /oname=onbattery.vbs.example ${TOPDIR}\platforms\mingw\onbattery.vbs
  File /oname=offbattery.vbs.example ${TOPDIR}\platforms\mingw\offbattery.vbs
  File /oname=commfailure.vbs.example ${TOPDIR}\platforms\mingw\commfailure.vbs

  ; Post-process apcupsd.conf.in into apcupsd.conf.new
  Call PostProcConfig

  ; Rename apcupsd.conf.new to apcupsd.conf if it does not already exist
  ${Unless} ${FileExists} "$INSTDIR\etc\apcupsd\apcupsd.conf"
    Rename apcupsd.conf.new apcupsd.conf
  ${EndUnless}
SectionEnd

Section "Tray Applet" SecApctray
  ; We're installing the apctray package
  StrCpy $TrayInstalled 1

  ; Shut down any running copy
  ${If} $MainInstalled != 1
    ExecWait '"$OrigInstDir\bin\apctray.exe" /kill'
    DetailPrint "Waiting for apctray to exit..."
    Sleep 2000
  ${EndIf}

  ; Install files
  CreateDirectory "$INSTDIR"
  CreateDirectory "$INSTDIR\bin"
  SetOutPath "$INSTDIR\bin"
  File ${WINDIR}\apctray.exe

  ; Create start menu link for apctray
  CreateDirectory "$SMPROGRAMS\Apcupsd"
  CreateShortCut "$SMPROGRAMS\Apcupsd\Apctray.lnk" "$INSTDIR\bin\apctray.exe"
SectionEnd

Section "Multimon CGI programs" SecMultimon
  CreateDirectory "$INSTDIR"
  CreateDirectory "$INSTDIR\cgi"
  CreateDirectory "$INSTDIR\etc"
  CreateDirectory "$INSTDIR\etc\apcupsd"

  SetOutPath "$INSTDIR\cgi"
  File ${WINDIR}\multimon.cgi
  File ${WINDIR}\upsstats.cgi
  File ${WINDIR}\upsfstats.cgi
  File ${WINDIR}\upsimage.cgi
  File ${WINDIR}\mingwm10.dll

  SetOutPath "$INSTDIR\etc\apcupsd"
  File ${TOPDIR}\src\cgi\apcupsd.css
  File /oname=hosts.conf.new ${TOPDIR}\platforms\etc\hosts.conf

  ; Rename hosts.conf.new to hosts.conf if it does not already exist
  ${Unless} ${FileExists} "$INSTDIR\etc\apcupsd\hosts.conf"
    Rename hosts.conf.new hosts.conf
  ${EndUnless}
SectionEnd

Section "USB Driver" SecUsbDrv
!if "${USBTYPE}" == "generic"
  ${If} ${IsNT}
    SetOutPath "$WINDIR\system32"
    File ${DEPKGS}\lib\libusb0.dll
    ExecWait 'rundll32 libusb0.dll,usb_install_driver_np_rundll $INSTDIR\driver\apcupsd.inf'
  ${Else}
    MessageBox MB_OK|MB_ICONEXCLAMATION \
       "The USB driver cannot be automatically installed on Win98 or WinMe. \
        Please see $INSTDIR\driver\install.txt for instructions on installing \
        the driver by hand."
  ${EndIf}
!else
  ${InstallUpgradeDriver} "$INSTDIR\driver" $INSTDIR\driver\apcupsd.inf "USB\VID_051d&PID_0002"
  ${If} $0 != 1
    MessageBox MB_OK|MB_ICONEXCLAMATION  \
      "The USB driver could not be automatically installed. You can ignore \
       this if you do not plan to use Apcupsd with a USB UPS. Otherwise, please \
       see $INSTDIR\driver\install.txt for instructions on installing the \
       USB driver by hand."
  ${EndIf}
!endif
SectionEnd

Section "Documentation" SecDoc
  SetOutPath "$INSTDIR\doc"
  CreateDirectory "$INSTDIR\doc"
  File ${TOPDIR}\doc\manual\manual.html
  File ${TOPDIR}\doc\manual\*.png
  File *.man.txt

  ; Create Start Menu entry
  CreateDirectory "$SMPROGRAMS\Apcupsd\Documentation"
  CreateShortCut "$SMPROGRAMS\Apcupsd\Documentation\Apcupsd User Manual.lnk"     "$INSTDIR\doc\manual.html"          "" "$SYSDIR\shell32.dll" ${MANUAL_ICON_INDEX}
  CreateShortCut "$SMPROGRAMS\Apcupsd\Documentation\apcupsd Reference.lnk"       "$INSTDIR\doc\apcupsd.man.txt"      "" "$SYSDIR\shell32.dll" ${HELP_ICON_INDEX}
  CreateShortCut "$SMPROGRAMS\Apcupsd\Documentation\apcaccess Reference.lnk"     "$INSTDIR\doc\apcaccess.man.txt"    "" "$SYSDIR\shell32.dll" ${HELP_ICON_INDEX}
  CreateShortCut "$SMPROGRAMS\Apcupsd\Documentation\apctest Reference.lnk"       "$INSTDIR\doc\apctest.man.txt"      "" "$SYSDIR\shell32.dll" ${HELP_ICON_INDEX}
  CreateShortCut "$SMPROGRAMS\Apcupsd\Documentation\apccontrol Reference.lnk"    "$INSTDIR\doc\apccontrol.man.txt"   "" "$SYSDIR\shell32.dll" ${HELP_ICON_INDEX}
  CreateShortCut "$SMPROGRAMS\Apcupsd\Documentation\Configuration Reference.lnk" "$INSTDIR\doc\apcupsd.conf.man.txt" "" "$SYSDIR\shell32.dll" ${HELP_ICON_INDEX}
SectionEnd

Section "-Finish"
  ; Write the uninstall keys for Windows & create Start Menu entry
  SetShellVarContext all
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Apcupsd" "DisplayName" "Apcupsd"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Apcupsd" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\Apcupsd\Uninstall Apcupsd.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

;
; Initialization Callback
;
Function .onInit
  ; If there is an existing installation, default INSTDIR to location of that
  ; install. Otherwise, if user did not specify INSTDIR on cmdline (using /D), 
  ; default it to %SystemDrive%\apcupsd.
  ReadRegStr $0 HKLM "Software\Apcupsd" "InstDir"
  ${If} $0 != ''
    StrCpy $INSTDIR $0
  ${ElseIf} "$INSTDIR" == ''
    ReadEnvStr $0 SystemDrive
    ${If} $0 == ''
      StrCpy $0 'c:'
    ${EndIf}
    StrCpy $INSTDIR $0\apcupsd
  ${EndIf}

  ; Preserve a copy of original install dir string before user possibly
  ; changes it. This will be used to run existing apcupsd and apctray exes
  ; with /kill switch to shut down running instances.
  StrCpy $OrigInstDir $INSTDIR

  !if "${USBTYPE}" == "generic"
    ; If we're on WinNT or Win95, disable the USB driver section
    ${If} ${IsWinNT4}
    ${OrIf} ${IsWin95}
       SectionGetFlags ${SecUsbDrv} $0
       IntOp $1 ${SF_SELECTED} ~
       IntOp $0 $0 & $1
       IntOp $0 $0 | ${SF_RO}
       SectionSetFlags ${SecUsbDrv} $0
    ${EndIf}
  !endif
  
  ; Extract custom pages. Automatically deleted when installer exits.
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "EditApcupsdConf.ini"
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "InstallService.ini"
  !insertmacro MUI_INSTALLOPTIONS_EXTRACT "Apctray.ini"

  ; Nothing installed yet
  StrCpy $MainInstalled 0
  StrCpy $TrayInstalled 0
FunctionEnd


;
; Extra Page descriptions
;

LangString DESC_SecService ${LANG_ENGLISH} "Install Apcupsd on this system."
LangString DESC_SecApctray ${LANG_ENGLISH} "Install Apctray. Shows status icon in the system tray."
LangString DESC_SecUsbDrv ${LANG_ENGLISH} "Install USB driver. Required if you have a USB UPS. Not available on Windows 95 or NT."
LangString DESC_SecDoc ${LANG_ENGLISH} "Install Documentation on this system."
LangString DESC_SecMultimon ${LANG_ENGLISH} "Install MULTIMON cgi scripts for web-based monitoring."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecService} $(DESC_SecService)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecApctray} $(DESC_SecApctray)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMultimon} $(DESC_SecMultimon)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUsbDrv} $(DESC_SecUsbDrv)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDoc} $(DESC_SecDoc)
!insertmacro MUI_FUNCTION_DESCRIPTION_END



; Uninstall section

UninstallText "This will uninstall Apcupsd. Hit next to continue."

Section "Uninstall"

  ; Shutdown any apcupsd & apctray that might be running
  ExecWait '"$INSTDIR\bin\apctray.exe" /kill'
  ExecWait '"$INSTDIR\bin\apcupsd.exe" /kill'
  DetailPrint "Waiting for apcupsd and apctray to exit..."
  Sleep 3000

  ; Remove apcuspd service, if needed
  ReadRegDWORD $R0 HKLM "Software\Apcupsd" "InstalledService"
  ${If} $R0 == 1
    ExecWait '"$INSTDIR\bin\apcupsd.exe" /quiet /remove'
  ${EndIf}

  ; Remove apctray autorun, if needed
  ReadRegStr $R0 HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "Apctray"
  ${If} $R0 != ""
    ExecWait '"$INSTDIR\bin\apctray.exe" /quiet /remove'
  ${EndIf}

  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Apcupsd"
  DeleteRegKey HKLM "Software\Apcupsd"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Run" "Apctray"

  ; remove start menu items
  SetShellVarContext all
  RMDir /r /REBOOTOK "$SMPROGRAMS\Apcupsd"

  ; remove files and uninstaller (preserving config for now)
  Delete /REBOOTOK "$INSTDIR\bin\mingwm10.dll"
  Delete /REBOOTOK "$INSTDIR\bin\pthreadGCE.dll"
  Delete /REBOOTOK "$INSTDIR\bin\libusb0.dll"
  Delete /REBOOTOK "$INSTDIR\bin\apcupsd.exe"
  Delete /REBOOTOK "$INSTDIR\bin\smtp.exe"
  Delete /REBOOTOK "$INSTDIR\bin\apcaccess.exe"
  Delete /REBOOTOK "$INSTDIR\bin\apctest.exe"
  Delete /REBOOTOK "$INSTDIR\bin\popup.exe"
  Delete /REBOOTOK "$INSTDIR\bin\shutdown.exe"
  Delete /REBOOTOK "$INSTDIR\bin\email.exe"
  Delete /REBOOTOK "$INSTDIR\bin\background.exe"
  Delete /REBOOTOK "$INSTDIR\bin\apctray.exe"
  Delete /REBOOTOK "$INSTDIR\driver\libusb0.dll"
  Delete /REBOOTOK "$INSTDIR\driver\libusb0_x64.dll"
  Delete /REBOOTOK "$INSTDIR\driver\libusb0.sys"
  Delete /REBOOTOK "$INSTDIR\driver\libusb0_x64.sys"
  Delete /REBOOTOK "$INSTDIR\driver\apcupsd.inf"
  Delete /REBOOTOK "$INSTDIR\driver\apcupsd.cat"
  Delete /REBOOTOK "$INSTDIR\driver\apcupsd_x64.cat"
  Delete /REBOOTOK "$INSTDIR\driver\install.txt"
  Delete /REBOOTOK "$INSTDIR\driver\i386\*.dll"
  Delete /REBOOTOK "$INSTDIR\driver\amd64\*.dll"
  Delete /REBOOTOK "$INSTDIR\README*"
  Delete /REBOOTOK "$INSTDIR\COPYING*"
  Delete /REBOOTOK "$INSTDIR\ChangeLog*"
  Delete /REBOOTOK "$INSTDIR\ReleaseNotes*"
  Delete /REBOOTOK "$INSTDIR\Uninstall.exe"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apccontrol.bat"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apcupsd.conf.new"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\hosts.conf.new"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\onbattery.vbs.example"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\offbattery.vbs.example"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\commfailure.vbs.example"
  Delete /REBOOTOK "$INSTDIR\doc\*"
  Delete /REBOOTOK "$INSTDIR\cgi\multimon.cgi"
  Delete /REBOOTOK "$INSTDIR\cgi\upsstats.cgi"
  Delete /REBOOTOK "$INSTDIR\cgi\upsfstats.cgi"
  Delete /REBOOTOK "$INSTDIR\cgi\upsimage.cgi"
  Delete /REBOOTOK "$INSTDIR\cgi\mingwm10.dll"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apcupsd.css"

  ; Delete conf if user approves
  ${If} ${FileExists} "$INSTDIR\etc\apcupsd\apcupsd.conf"
  ${OrIf} ${FileExists} "$INSTDIR\etc\apcupsd\apcupsd.events"
    ${If} ${Cmd} 'MessageBox MB_YESNO|MB_ICONQUESTION "Would you like to delete the current configuration and events files?" /SD IDYES IDYES'
      Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apcupsd.conf"
      Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apcupsd.events"
      Delete /REBOOTOK "$INSTDIR\etc\apcupsd\hosts.conf"
    ${EndIf}
  ${EndIf}

  ; remove directories used
  RMDir "$INSTDIR\bin"
  RMDir "$INSTDIR\cgi"
  RMDir "$INSTDIR\driver"
  RMDir "$INSTDIR\etc\apcupsd"
  RMDir "$INSTDIR\etc"
  RMDir "$INSTDIR\doc"
  RMDir "$INSTDIR\examples"
  RMDir "$INSTDIR"
  RMDir "C:\tmp"
  
SectionEnd

; eof
