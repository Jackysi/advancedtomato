proc AboutCmd {} {
   toplevel .about -class Dialog
   wm title .about "About Apcupsd"
   wm transient .about .
   image create photo .about.image -file "apcupsd_logo.gif"
   label .about.label -image .about.image -borderwidth 1
   set msg "Copyright 2001, Kern Sibbald\n"
   append msg "Licensed under GPL version 2\n"
   append msg "Apcupsd release 3.9.6 (21 Sep 2001)" 
   message .about.copyright -justify center -text $msg -width 400
   button .about.ok -borderwidth 1 -text " OK " -command {destroy .about}
   pack .about.label -side top
   pack .about.copyright -side top -pady 5
   pack .about.ok -side bottom -pady 5
}
