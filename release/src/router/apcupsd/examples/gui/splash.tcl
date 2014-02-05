proc DisplaySplash {} {
   wm withdraw .
   toplevel .splash -borderwidth 4 -relief raised
   wm overrideredirect .splash 1
   after idle {
      update idletasks
      set xmax [winfo screenwidth .splash]
      set ymax [winfo screenheight .splash]
      set x0 [expr ($xmax-[winfo reqwidth .splash])/2]
      set y0 [expr ($ymax-[winfo reqheight .splash])/2]
      wm geometry .splash "+$x0+$y0"
   }
   image create photo .splash.image -file "apcupsd_logo.gif"
   label .splash.label -image .splash.image
   pack  .splash.label
   update
}
