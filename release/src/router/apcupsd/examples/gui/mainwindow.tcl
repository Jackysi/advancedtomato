proc CreateMainWindow {} {
   global statustext StatusText
   set StatusText " Ready ..."
   . configure -menu .mbar
   menu .mbar
   .mbar add cascade -label "File" -underline 0 -menu .mbar.file
   menu .mbar.file

   .mbar.file add separator
   .mbar.file add command -label "Exit" -underline 0 -command {ExitCmd}

   .mbar add cascade -label "Options" -underline 0 -menu .mbar.options
   menu .mbar.options
   .mbar.options add command -label "Refresh Now" -underline 0 -command {RefreshCmd}

   .mbar add cascade -label "Help" -underline 0 -menu .mbar.help
   menu .mbar.help
   .mbar.help add command -label "About Apcupsd" -underline 0 -command {AboutCmd}
   .mbar.help add command -label "Help" -underline 0 -command {HelpCmd}

   frame .sep1 -height 10 -borderwidth 0
   pack  .sep1 -fill x -side top
   text .sep1.text -relief groove -height 1  
   .sep1.text configure -tabs {2.5i 4.5i 6.5i}
   .sep1.text insert end "\tBattery V\tMains V\tMains V"
   pack .sep1.text -side top -fill x

   frame .main -borderwidth 0
   pack  .main -side top -fill x

   frame .sep2 -height 2 -borderwidth 1 -relief sunken
   pack  .sep2 -fill x -pady 4

   frame .elab 
   pack  .elab -side top -fill x
   label .elab.lab1 -text "Events:" -anchor e
   pack  .elab.lab1 -side left -padx 4

   frame .events
   pack  .events -fill x -padx 4 -pady 4

   scrollbar .events.sbar -command ".events.text yview"
   pack  .events.sbar -side right -fill y
   text  .events.text -height 10 -wrap word -yscrollcommand ".events.sbar set"
   pack  .events.text -side top -fill x

   frame .status -borderwidth 1 -relief raised
   pack  .status  -side bottom -fill x -pady 2
   label .statusLabel -text "Status:" -anchor e -fg magenta
   set statustext [label .statusText -relief sunken \
      -borderwidth 1 -textvariable StatusText -anchor w]
   grid columnconfigure .status 1 -weight 1
   grid .statusLabel -in .status -row 0 -column 0
   grid .statusText -in .status -row 0 -column 1 -sticky ew -padx 3
}
