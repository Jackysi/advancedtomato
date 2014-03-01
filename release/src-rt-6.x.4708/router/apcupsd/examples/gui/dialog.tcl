
proc dialog_create {class {win "auto"}} {
    if {$win == "auto"} {
        set count 0
        set win ".dialog[incr count]"
        while {[winfo exists $win]} {
            set win ".dialog[incr count]"
        }
    }
    toplevel $win -class $class

    frame $win.info
    pack $win.info -expand yes -fill both -padx 4 -pady 4

    frame $win.sep -height 2 -borderwidth 1 -relief sunken
    pack $win.sep -fill x -pady 4

    frame $win.controls
    pack $win.controls -fill x -padx 4 -pady 4

    wm title $win $class
    wm group $win .

    after idle [format {
        update idletasks
        wm minsize %s [winfo reqwidth %s] [winfo reqheight %s]
    } $win $win $win]

    return $win
}

proc dialog_info {win} {
    return "$win.info"
}

proc dialog_controls {win} {
    return "$win.controls"
}

proc dialog_wait {win varName} {
    dialog_safeguard $win

    set x [expr [winfo rootx .]+50]
    set y [expr [winfo rooty .]+50]
    wm geometry $win "+$x+$y"

    wm deiconify $win
    grab set $win

    vwait $varName

    grab release $win
    wm withdraw $win
}

bind modalDialog <ButtonPress> {
    wm deiconify %W
    raise %W
}
proc dialog_safeguard {win} {
    if {[lsearch [bindtags $win] modalDialog] < 0} {
        bindtags $win [linsert [bindtags $win] 0 modalDialog]
    }
}
