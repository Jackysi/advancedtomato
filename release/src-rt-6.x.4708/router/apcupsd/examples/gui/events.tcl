proc GetEvents {} {
   .events.text delete 0.0 end
   set fd [open "| ./client localhost:7000 events" "r"]
   while {[gets $fd line] > 0} {
      .events.text insert end "\n"
      .events.text insert end $line
   }    
   catch {close $fd}
   .events.text delete 0.0 2.0
}
