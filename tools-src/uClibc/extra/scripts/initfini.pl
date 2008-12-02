#!/usr/bin/perl

use strict;
use Getopt::Long;

my($initfini)	    = "initfini.s";
my($crti)	    = "crti.S";
my($crtn)	    = "crtn.S";
my($alignval)	    = "";
my($endp)	    = 0;
my($end)	    = 0;
my($omitcrti)	    = 0;
my($omitcrtn)	    = 0;
my($line);

# Get commandline parameters
Getopt::Long::Configure("no_ignore_case", "bundling");
&GetOptions(	"initfini=s" => \$initfini,
		"crti=s" => \$crti,
		"crtn=s" => \$crtn,
		);

chomp($initfini);
chomp($crti);
chomp($crtn);


if ($initfini) {
	open(INITFINI,"<$initfini") or
		    die "(fatal) Can't open $initfini$!";
} else {
    die "(fatal) Please give me an --initfini argument$!";
}
while(<INITFINI>) {
    if (/\.endp/) {
	$endp=1;
	next;
    }
    if (/\.end/) {
	$end=1;
	next;
    }
    if (/\.align(.*)/) {
	$alignval=$1;
	next;
    }

}
close(INITFINI);





if ($initfini) {
	open(INITFINI,"<$initfini") or
		    die "(fatal) Can't open $initfini$!";
} else {
    die "(fatal) Please give me an --initfini argument$!";
}

if ($crti) {
	open(CRTI,">$crti") or
		    die "(fatal) Can't open $crti$!";
} else {
    die "(fatal) Please give me a --asm argument$!";
}
if ($crtn) {
	open(CRTN,">$crtn") or
		    die "(fatal) Can't open $crtn$!";
} else {
    die "(fatal) Please give me a --asm argument$!";
}

while(<INITFINI>) {
    if (/HEADER_ENDS/) {
	$omitcrti = 1;
	$omitcrtn = 1;
	next;
    }
    if (/PROLOG_BEGINS/) {
	$omitcrti = 0;
	$omitcrtn = 0;
	next;
    }
    if (/^_init:/ || /^_fini:/) {
	$omitcrtn = 1;
    }
    if (/PROLOG_PAUSES/) {
	$omitcrti = 1;
	next;
    }
    if (/PROLOG_UNPAUSES/) {
	$omitcrti = 0;
	next;
    }
    if (/PROLOG_ENDS/) {
	$omitcrti = 1;
	next;
    }
    if (/EPILOG_BEGINS/) {
	$omitcrtn = 0;
	next;
    }
    if (/EPILOG_ENDS/) {
	$omitcrtn = 1;
	next;
    }
    if (/TRAILER_BEGINS/) {
	$omitcrti = 0;
	$omitcrtn = 0;
	next;
    }
    if (/END_INIT/) {
	if ($endp) {
	    s/END_INIT/.endp _init/;
	} else {
	    if($end) {
		s/END_INIT/.end _init/;
	    } else {
		s/END_INIT//;
	    }
	}
    }
    if (/END_FINI/) {
	if ($endp) {
	    s/END_FINI/.endp _fini/;
	} else {
	    if($end) {
		s/END_FINI/.end _fini/;
	    } else {
		s/END_FINI//;
	    }
	}
    }
    if (/ALIGN/) {
	if($alignval) {
	    s/ALIGN/.align $alignval/;
	} else {
	    s/ALIGN//;
	}
    }
    if (!$omitcrti) {
	print CRTI;
    }
    if (!$omitcrtn) {
	print CRTN;
    }
}
close(INITFINI);
close(CRTI);
close(CRTN);

