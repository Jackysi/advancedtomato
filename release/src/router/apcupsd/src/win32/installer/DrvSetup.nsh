;
; Written by Kuba Ober
; Copyright (c) 2004 Kuba Ober
;
; Permission is hereby granted, free of charge, to any person obtaining a 
; copy of this software and associated documentation files (the "Software"), 
; to deal in the Software without restriction, including without limitation 
; the rights to use, copy, modify, merge, publish, distribute, sublicense, 
; and/or sell copies of the Software, and to permit persons to whom the 
; Software is furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in 
; all copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
; DEALINGS IN THE SOFTWARE.
;
; U S A G E
;
; Push "c:\program files\yoursoftware\driver" 
;  -- the directory of the .inf file
; Push "c:\program files\yoursoftware\driver\driver.inf"
;  -- the filepath of the .inf file (directory + filename)
; Push "USB\VID_1234&PID_5678"
;  -- the HID (Hardware ID) of your device
; Call InstallUpgradeDriver
;
; Your driver (minimally the .inf and .sys files) should already by installed
; by your NSIS script.
;
; Typically, you would put the driver either in $INSTDIR or $INSTDIR\Driver
; It's up to you, of course.
;
; The driver (i.e. .inf, .sys and related files) must be present for the
; lifetime of your application, you shouldn't remove them after calling
; this function!
;
; You DON'T want to put the driver in any of system directories. Windows
; will do it when the device is first plugged in.

!define InstallUpgradeDriver '!insertmacro "_installUpgradeDriverConstructor"'

!macro _installUpgradeDriverConstructor PATH INF HID
  Push "${PATH}"
  Push "${INF}"
  Push "${HID}"
  Call InstallUpgradeDriver
!macroend

; BOOL UpdateDriverForPlugAndPlayDevices(HWND, PSTR, PSTR, DWORD, PBOOL);
!define sysUpdateDriverForPlugAndPlayDevices "newdev::UpdateDriverForPlugAndPlayDevices(i, t, t, i, *i) i"

; the masked value of ERROR_NO_SUCH_DEVINST is 523
!define ERROR_NO_SUCH_DEVINST -536870389
 
!define INSTALLFLAG_FORCE 1

Function InstallUpgradeDriver
 
   Pop $R0 ; HID
   Pop $R1 ; INFPATH
   Pop $R2 ; INFDIR

   ; Check Windows version
   ${If} ${AtLeastWin2000}
      Goto lbl_upgrade
   ${EndIf}

   DetailPrint "This version of Windows does not support driver updates."
   Goto lbl_done

   ; Upgrade the driver if the device is already plugged in
lbl_upgrade:
   System::Get '${sysUpdateDriverForPlugAndPlayDevices}'
   Pop $0
   StrCmp $0 'error' lbl_noapi
   DetailPrint "Updating the USB driver..."
   ; 0, HID, INFPATH, 0, 0
   Push $INSTDIR ; Otherwise this function will swallow it, dunno why
   System::Call '${sysUpdateDriverForPlugAndPlayDevices}?e (0, R0, R1, ${INSTALLFLAG_FORCE}, 0) .r0'
   Pop $1 ; last error
   Pop $INSTDIR
   IntCmp $0 1 lbl_done
   IntCmp $1 ${ERROR_NO_SUCH_DEVINST} lbl_notplugged

   DetailPrint "Driver update failed: ($0,$1)"
   Goto lbl_done

lbl_notplugged:
   DetailPrint "The device is not plugged in, cannot update the driver."
   Goto lbl_done

lbl_noapi:
   DetailPrint "This version of Windows does not support driver updates."

lbl_done:
FunctionEnd
