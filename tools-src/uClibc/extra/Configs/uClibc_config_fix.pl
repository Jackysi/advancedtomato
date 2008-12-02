#!/usr/bin/perl
# Silly script to fixup the uClibc config file
# (c) Erik Andersen <andersee@codepoet.org>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


# The easiest way to use this script is to put something like this in
# your Makefile for building uClibc...  Adjust config optiongs to 
# taste of course....  And of course you will want to add some defines
# into your Makefile to set ARCH, STAGING_DIR, KERNEL_DIR, and whatnot.
#
# $(UCLIBC_DIR)/extra/Configs/uClibc_config_fix.pl --arch=$(ARCH) --cross="$(CROSS)" \
#        --devel_prefix=$(STAGING_DIR) --kernel_dir=$(KERNEL_DIR) --large_file=false \
#        --rpc_support=true --c99_math=true --shared_support=true --ldso_path="/lib" \
#        --shadow=true --file=$(UCLIBC_DIR)/extra/Configs/Config.$(ARCH) > $(UCLIBC_DIR)/Config;
#
# Have fun,
#  -Erik
#

use strict;
use Getopt::Long;

# User Specified variables (commandline)
my($arch)	    = "";
my($cross)	    = "";
my($xcc)	    = "";
my($native_cc)	    = "";
my($debug)	    = "";
my($mmu)	    = "";
my($large_file)	    = "";
my($rpc_support)    = "";
my($c99_math)	    = "";
my($long_long)	    = "";
my($float)	    = "";
my($threads)	    = "";
my($shadow)	    = "";
my($filename)	    = "";
my($shared_support) = "";
my($kernel_dir)	    = "";
my($devel_prefix)   = "";
my($ldso_path)	    = "";
my($line);
my($got_arch);

# Get commandline parameters
Getopt::Long::Configure("no_ignore_case", "bundling");
&GetOptions(	"arch=s" => \$arch,
		"cross=s" => \$cross,
		"cc=s" => \$xcc,
		"native_cc=s" => \$native_cc,
		"devel_prefix=s" => \$devel_prefix,
		"kernel_dir=s" => \$kernel_dir,
		"debug=s" => \$debug,
		"mmu=s" => \$mmu,
		"large_file=s" => \$large_file,
		"rpc_support=s" => \$rpc_support,
		"c99_math=s" => \$c99_math,
		"long_long=s" => \$long_long,
		"float=s" => \$float,
		"threads=s" => \$threads,
		"shadow=s" => \$shadow,
		"shared_support=s" => \$shared_support,
		"ldso_path=s" => \$ldso_path,
		"file=s" => \$filename,
		);
chomp($arch);
chomp($cross);
chomp($xcc);
chomp($native_cc);
chomp($devel_prefix);
chomp($kernel_dir);
chomp($debug);
chomp($mmu);
chomp($large_file);
chomp($rpc_support);
chomp($c99_math);
chomp($long_long);
chomp($float);
chomp($threads);
chomp($shadow);
chomp($shared_support);
chomp($ldso_path);
chomp($filename);

if ($filename) {
	open(FILE,"<$filename") or
		    die "(fatal) Can't open $filename:$!";
} else {
    die "(fatal) Please give me a --file argument$!";
}


while($line = <FILE>) {
    if ($arch && $line =~ /^TARGET_ARCH.*/) {
	print "TARGET_ARCH=$arch\n";
	$got_arch=1;
	next;
    } 
    if ($cross && $line =~ /^CROSS.*/) {
	print "CROSS=$cross\n";
	next;
    }
    if ($xcc && $line =~ /^CC.*/) {
	print "CC=$xcc\n";
	next;
    }
    if ($native_cc && $line =~ /^NATIVE_CC.*/) {
	print "NATIVE_CC=$native_cc\n";
	next;
    }
    if ($devel_prefix && $line =~ /^DEVEL_PREFIX.*/) {
	print "DEVEL_PREFIX=$devel_prefix\n";
	next;
    }
    if ($kernel_dir && $line =~ /^KERNEL_SOURCE.*/) {
	print "KERNEL_SOURCE=$kernel_dir\n";
	next;
    }
    if ($debug && $line =~ /^DODEBUG.*/) {
	print "DODEBUG=$debug\n";
	next;
    }
    if ($mmu && $line =~ /^HAS_MMU.*/) {
	print "HAS_MMU=$mmu\n";
	next;
    }
    if ($large_file && $line =~ /^DOLFS.*/) {
	print "DOLFS=$large_file\n";
	next;
    }
    if ($rpc_support && $line =~ /^INCLUDE_RPC.*/) {
	print "INCLUDE_RPC=$rpc_support\n";
	next;
    }
    if ($shadow && $line =~ /^HAS_SHADOW.*/) {
	print "HAS_SHADOW=$shadow\n";
	next;
    }
    if ($c99_math && $line =~ /^DO_C99_MATH.*/) {
	print "DO_C99_MATH=$c99_math\n";
	next;
    }
    if ($long_long && $line =~ /^HAS_LONG_LONG.*/) {
	print "HAS_LONG_LONG=$long_long\n";
	next;
    }
    if ($float && $line =~ /^HAS_FLOATING_POINT.*/) {
	print "HAS_FLOATING_POINT=$float\n";
	next;
    }
    if ($threads && $line =~ /^INCLUDE_THREADS.*/) {
	print "INCLUDE_THREADS=$threads\n";
	next;
    }
    if ($shared_support && $shared_support =~ /true/ ) {
	if ($line =~ /^BUILD_UCLIBC_LDSO.*/) {
	    print "BUILD_UCLIBC_LDSO=true\n";
	    next;
	}
	if ($line =~ /^HAVE_SHARED.*/) {
	    print "HAVE_SHARED=true\n";
	    next;
	}
	# Force PIC to be true when HAVE_SHARED is true
	if ($line =~ /^DOPIC.*/) {
	    print "DOPIC=true\n";
	    next;
	}
	if ($ldso_path && $line =~ /^SHARED_LIB_LOADER_PATH.*/) {
	    print "SHARED_LIB_LOADER_PATH=$ldso_path\n";
	    next;
	}
    } else {
	if ($line =~ /^BUILD_UCLIBC_LDSO.*/) {
	    print "BUILD_UCLIBC_LDSO=false\n";
	    next;
	}
	if ($line =~ /^HAVE_SHARED.*/) {
	    print "HAVE_SHARED=false\n";
	    next;
	}
	if ($line =~ /^DOPIC.*/) {
	    print "DOPIC=false\n";
	    next;
	}
    }
    print "$line";
}

if ($arch && ! $got_arch) {
    print "TARGET_ARCH=$arch\n";
}

