#! /usr/bin/awk -f
# Contributed by Christian MICHON <christian_michon@yahoo.fr> to
#   eliminate the compile time dependancy on perl introduced by 
#   Erik's older initfini.pl 
# vim:ai:sw=2:

BEGIN \
{ alignval="";
  endp=0;
  end=0;
  system("touch crt[in].S");
  system("/bin/rm -f crt[in].S");
  omitcrti=0;
  omitcrtn=0;
  glb_idx = 0;
  while(getline < "initfini.S")
  { if(/\.endp/) {endp=1}
    if(/\.end/) {end=1}
    if(/\.align/) {alignval=$2}
# here comes some special stuff for the SuperH targets
#  We search for all labels, which uses the _GLOBAL_OFFSET_TABLE_
#  definition, and store them in the glb_label array.
    if(/_GLOBAL_OFFSET_TABLE_/) {
      glb_label[glb_idx] = last;
      glb_idx += 1;
      glb = $0;
    }
    last = $1;
  }
  close("initfini.S");
}
# special rules for the SuperH targets (They do nothing on other targets)
/SH_GLB_BEGINS/ && glb_idx==0 {omitcrti +=1}
/_init_SH_GLB/  && glb_idx>=1 {print glb_label[0] glb >> "crti.S"}
/_fini_SH_GLB/  && glb_idx>=2 {print glb_label[1] glb >> "crti.S"}
/SH_GLB_ENDS/   && glb_idx==0 {omitcrti -=1}
/SH_GLB/ || /_GLOBAL_OFFSET_TABLE_/{getline}
# special rules for H8/300 (sorry quick hack)
/.h8300h/ {end=0}

# rules for all targets
/HEADER_ENDS/{omitcrti=1;omitcrtn=1;getline}
/PROLOG_BEGINS/{omitcrti=0;omitcrtn=0;getline}
/i_am_not_a_leaf/{getline}
/_init:/||/_fini:/{omitcrtn=1}
/PROLOG_PAUSES/{omitcrti=1;getline}
/PROLOG_UNPAUSES/{omitcrti=0;getline}
/PROLOG_ENDS/{omitcrti=1;getline}
/EPILOG_BEGINS/{omitcrtn=0;getline}
/EPILOG_ENDS/{omitcrtn=1;getline}
/TRAILER_BEGINS/{omitcrti=0;omitcrtn=0;getline}

/END_INIT/ \
{ if(endp)
  { gsub("END_INIT",".endp _init",$0)
  }
  else
  { if(end)
    { gsub("END_INIT",".end _init",$0)
    }
    else
    { gsub("END_INIT","",$0)
    }
  }
}

/END_FINI/ \
{ if(endp)
  { gsub("END_FINI",".endp _fini",$0)
  }
  else
  { if(end)
    { gsub("END_FINI",".end _fini",$0)
    }
    else
    { gsub("END_FINI","",$0)
    }
  }
}

/ALIGN/ \
{ if(alignval!="")
  { gsub("ALIGN",sprintf(".align %s",alignval),$0)
  }
  else
  { gsub("ALIGN","",$0)
  }
}

omitcrti==0 {print >> "crti.S"}
omitcrtn==0 {print >> "crtn.S"}

END \
{ close("crti.S");
  close("crtn.S");
}
