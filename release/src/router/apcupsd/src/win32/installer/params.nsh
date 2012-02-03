;
; -- written by Alexis de Valence --
; GetONEParameter

; Usage:
;   Push 3                 ; to get the 3rd parameter of the command line
;   Call GetONEParameter
;   Pop $R0                ; saves the result in $R0
; returns an empty string if not found

Function GetONEParameter
   Exch $R0
   Push $R1
   Push $R2
   Push $R3
   Push $R4
   Push $R5
   Push $R6

; init variables
   IntOp $R5 $R0 + 1
   StrCpy $R2 0
   StrCpy $R4 1
   StrCpy $R6 0

   loop3: ; looking for a char that's not a space
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 " " loop3
     StrCpy $R3 $R2   ; found the begining of the current parameter


   loop:          ; scanning for the end of the current parameter

     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 " " loop2
     StrCmp $R0 "" last
     IntOp $R2 $R2 + 1
     Goto loop

   last: ; there will be no other parameter to extract
   StrCpy $R6 1

   loop2: ; found the end of the current parameter

   IntCmp $R4 $R5 0 NextParam end
   StrCpy $R6 1 ; to quit after this process

   IntOp $R1 $R2 - $R3 ;number of letter of current parameter
   StrCpy $R0 $CMDLINE $R1 $R3        ; stores the result in R0

   NextParam:
   IntCmp $R6 1 end ; leave if found or if not enough parameters

   ; process the next parameter
   IntOp $R4 $R4 + 1

   Goto loop3

   end:

   Pop $R6  ; restore R0 - R6 to their initial value
   Pop $R5
   Pop $R4
   Pop $R3
   Pop $R2
   Pop $R1

   Exch $R0    ;Puts the result on the stack

 FunctionEnd

; -- written by Michel Meyers --
; ParameterGiven - checks first 9 parameters on the command line
; Usage:
;   Push "/parameter"                 ; to check command line for /parameter
;   Call ParameterGiven
;   Pop $R0                ; saves the result in $R0 (result = true or false)

 Function ParameterGiven
   Exch $R0
   Push $R1
   Push $R2
   Push $R3
   
   StrCpy $R1 0
   StrCpy $R3 0
   loopme:
   StrCmp $R1 9 AllChecked
   IntOp $R1 $R1 + 1
   Push $R1
   Call GetONEParameter
   Pop $R2                ; saves the result in $R2
   StrCmp $R0 $R2 Found
   Goto loopme
   
   Found:
   StrCpy $R3 1
   Goto loopme
   
   AllChecked:
   Exch $R3
  
FunctionEnd
