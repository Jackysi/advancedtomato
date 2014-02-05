proc DrawTickLines {c imwid imhgt} {
   $c create line 50  60 150  60 -fill darkgrey 
   $c create line 50 120 150 120 -fill darkgrey 
   $c create line 50 180 150 180 -fill darkgrey 
   $c create line 50 240 150 240 -fill darkgrey 
   $c create line 50 300 150 300 -fill darkgrey 
   $c create line 50 0 50 300 150 300 150 1 50 1 -fill black
}

proc DrawText {c min step imwid imhgt} {
   set next $min
   $c create text 46 300 -anchor se -text [format "%d" $next]
   set next [expr $next + $step]
   $c create text 46 240 -anchor e -text [format "%d" $next]
   set next [expr $next + $step]
   $c create text 46 180 -anchor e -text [format "%d" $next]
   set next [expr $next + $step]
   $c create text 46 120 -anchor e -text [format "%d" $next]
   set next [expr $next + $step]
   $c create text 46 60 -anchor e -text [format "%d" $next]
   set next [expr $next + $step]
   $c create text 46  0 -anchor ne -text [format "%d" $next]

}


proc DrawBattVolt {c imwid imhgt} {
   global BattV NomBattV 
   switch -- $NomBattV {
      12.0 {
         set minv 3
         set maxv 18
         set hip [expr 12 + 3]
         set lowp [expr 12 - 3]
      }
      24.0 {
         set minv 15
         set maxv 30
         set hip [expr 24 + 5]
         set lowp [expr 24 - 5]
      }
      48.0 {
         set minv 30
         set maxv 60
         set hip [expr 48 + 7]
         set lowp [expr 48 - 7]
      }
      default {
         set minv 0
         set maxv [expr 10 * (($BattV / 10) + 1)]
         set hip [expr $BattV + 5]
         set lowp [expr $BattV -5]
      }

   }
   set deltav  [expr $maxv - $minv]
   set battpos [expr $imhgt - (($BattV - $minv) * $imhgt / $deltav)]
   set hipos  [expr $imhgt - (($hip - $minv) * $imhgt / $deltav)]
   set lowpos [expr $imhgt - (($lowp - $minv) * $imhgt / $deltav)]
   $c create rectangle 50 0 150 $imhgt -fill green
   $c create rectangle 50 0 150 $hipos -fill red -width 0
   $c create rectangle 50 $lowpos 150 $imhgt -fill red -width 0
   $c create rectangle 75 $battpos 125 $imhgt -fill black
   $c create text 75 [expr $imhgt + 10] -anchor w -text [format "%.1f VDC" $BattV]
   DrawTickLines $c $imwid $imhgt
   DrawText $c $minv [expr $deltav/5] $imwid $imhgt

}

proc DrawUtility {c imwid imhgt} {
   global LineV LowTrans HiTrans
    
   set minv 175
   set deltav 75
   set utilpos [expr $imhgt - (($LineV - $minv) * $imhgt / $deltav)]
   set hipos  [expr $imhgt - (($HiTrans - $minv) * $imhgt / $deltav)]
   set lowpos [expr $imhgt - (($LowTrans - $minv) * $imhgt / $deltav)]
   $c create rectangle 50 0 150 $imhgt -fill green
   $c create rectangle 50 0 150 $hipos -fill red
   $c create rectangle 50 $lowpos 150 $imhgt -fill red
   $c create rectangle 75 $utilpos 125 $imhgt -fill black
   $c create text 75 [expr $imhgt + 10] -anchor w -text [format "%.1f VAC" $LineV]
   DrawTickLines $c $imwid $imhgt
   DrawText $c $minv [expr $deltav/5] $imwid $imhgt
}
proc DisplayStatus {} {
   global BattChg TimeLeft LoadPct HostName Release LineV RetPct
   global LineFreq BattV NomBattV ITemp
   global Model Status
   set imhgt 300
   set imwid 100

   frame .main.text -borderwidth 2 
   pack .main.text -side left -expand no -pady 4

   label .main.text.lab00 -text "Host:"
   label .main.text.lab01 -text $HostName
   grid .main.text.lab00 -row 0 -column 0 -sticky e
   grid .main.text.lab01 -row 0 -column 1 -sticky ew

   label .main.text.lab10 -text "UPS Model:"
   label .main.text.lab11 -text $Model
   grid .main.text.lab10 -row 1 -column 0 -sticky e
   grid .main.text.lab11 -row 1 -column 1 -sticky ew

   label .main.text.lab20 -text "UPS Status:"
   label .main.text.lab21 -text $Status
   grid .main.text.lab20 -row 2 -column 0 -sticky e
   grid .main.text.lab21 -row 2 -column 1 -sticky ew

   label .main.text.lab30 -text "Internal Temp:"
   label .main.text.lab31 -text $ITemp
   grid .main.text.lab30 -row 3 -column 0 -sticky e
   grid .main.text.lab31 -row 3 -column 1 -sticky ew

   label .main.text.lab40 -text "Apcupsd Release:"
   label .main.text.lab41 -text $Release
   grid .main.text.lab40 -row 4 -column 0 -sticky e
   grid .main.text.lab41 -row 4 -column 1 -sticky ew


   grid columnconfigure .main.text 1 -weight 1

   frame .main.image1
   pack .main.image1 -side left -pady 4
   canvas .main.image1.c -width [expr $imwid+50] -height [expr $imhgt + 20]
   DrawBattVolt .main.image1.c $imwid $imhgt
   pack .main.image1.c

   frame .main.image2 
   pack .main.image2 -side left -pady 4
   canvas .main.image2.c -width [expr $imwid+50] -height [expr $imhgt + 20]
   DrawUtility .main.image2.c $imwid $imhgt
   pack .main.image2.c      

   frame .main.image3
   pack .main.image3 -side left -pady 4
   canvas .main.image3.c -width [expr $imwid+75] -height [expr $imhgt + 20]
   DrawUtility .main.image3.c $imwid $imhgt
   pack .main.image3.c
}


proc GetStatus {} {
   global BattChg TimeLeft LoadPct HostName Release LineV RetPct
   global LineFreq BattV NomBattV ITemp LowTrans HiTrans
   global Model Status
   foreach {i} {BattChg TimeLeft LoadPct HostName Release LineV \
                RetPct LineFreq BattV ITemp NomBattV LowTrans \
                HiTrans Model Status} {
      set $i 0
    }
   set fd [open "| ./client localhost:7000 status" "r"]
   while {[gets $fd line] > 0} {
      scan $line "%9s" label
      switch -- $label {
         BCHARGE {
            scan $line "%*s : %f Percent" BattChg
         }
         TIMELEFT {
            scan $line "%*s : %f Minutes" TimeLeft
         }
         LOADPCT {
            scan $line "%*s : %f Percent" LoadPct
         }
         HOSTNAME {
            scan $line "%*s : %s" HostName
         }
         RELEASE {
            scan $line "%*s : %s" Release
         }
         LINEV {
            scan $line "%*s : %f Volts" LineV
         }
         LOTRANS {
            scan $line "%*s : %f Volts" LowTrans
         }
         HITRANS {
            scan $line "%*s : %f Volts" HiTrans
         }
         RETPCT {
            scan $line "%*s : %f Percent" RetPct
         }
         ITEMP {
            scan $line "%*s : %f C Internal" ITemp
         }
         LINEFREQ {
            scan $line "%*s : %f Hz" LineFreq
         }
         BATTV {
            scan $line "%*s : %f Volts" BattV
         }
         NOMBATTV {
            scan $line "%*s : %f" NomBattV
         }
         MODEL {
            scan $line "%*s : %s" Model
         }
         STATUS {
            scan $line "%*s : %s" Status
         }

         default {
         }
      }
   }    
   catch {close $fd}
}
