#
# Apcupsd Graphic Interface
#            

proc MessageBox {msg type} {
   tk_messageBox -icon $type -message $msg
}

proc ExitCmd {} {
   exit 0
}

proc RefreshCmd {} {
   GetStatus
   destroy .main.text
   destroy .main.image1
   destroy .main.image2
   destroy .main.image3
   DisplayStatus
   GetEvents
   update
}

proc HelpCmd {} {
   exec netscape www.sibbald.com/apcupsd/manual/ &
}

wm title . "Apcupsd"
# wm minsize . 600 390

foreach File [list \
   mainwindow.tcl \
   about.tcl \
   dialog.tcl \
   splash.tcl \
   status.tcl \
   events.tcl \
             ] {
      if {![file exists "$File"]} {
         puts "Unable to find required source file $File"
         exit 1
      } else {
         source $File
      }
}
unset File

DisplaySplash
CreateMainWindow
GetStatus
DisplayStatus
GetEvents
update
after 1000
destroy .splash
wm deiconify .
 
