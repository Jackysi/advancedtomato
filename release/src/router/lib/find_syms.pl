#!/usr/bin/perl -w
#-------------------------------------------------------------------------------
#
#   Copyright (C) 2004 Broadcom Corporation.
#   All rights reserved.
# 
#   These coded instructions and statements contain unpublished trade
#   secrets and proprietary information of Broadcom Corporation.  They
#   are protected by federal copyright law and by trade secret law,
#   and may not be disclosed to third parties or used, copied, or
#   duplicated in any form, in whole or in part, without the prior written
#   consent of Broadcom Corporation.
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#
#   This script determines which symbols can be resolved by a particular
#   library.  The library symbols are expected to be in a map file specified
#	as the first argument.  The set of binaries to check for resolution vs.
#	the library are in the subsequent arguments.
#
#   For each undefined symbol (from stdin) that has a match in the library,
#   this script prints a line of the form 'EXTERN(symbol);'.  This can be used
#   in subsequent make commands.
#
#-------------------------------------------------------------------------------

use strict;
use Getopt::Long;
use IO::File;


#-------------------------------------------------------------------------------
# Usage

sub usage
{
print STDOUT<<EOF

Usage: $0 --map file1 [--map ...] --bin file1 [--bin ...]

This script determines which dynamic symbols in a set of binary objects
can be resolved by a particular set of shared libraries.  The libraries
are not specified directly, but rather through a set of map files.

For each undefined symbol that can be resolved, the script prints a line
of the form 'EXTERN(symbol);' on standard output.

EOF
}


#-------------------------------------------------------------------------------
# Main script

# Must specify one or more map and binary files on the command line.

my $args = {};

if (!GetOptions( $args, "map=s@", "bin=s@" ) or
	not defined $args->{ map } or
	not defined $args->{ bin })
{
	usage();
	exit( 1 );
}


# Extract the library symbol names into a hash table.  Note that
# multiple symbols can be defined on a single line.

my %lib_symtbl;
foreach my $map_file (@{$args->{ map }})
{
	my $map_handle = new IO::File( $map_file, "r" ) or
		die "File $map_file open failed\n";

	while (my $line = $map_handle->getline())
	{
		chomp $line;
		foreach my $sym ($line =~ /\s+(\S+);/og)
		{
			$lib_symtbl{ $sym } += 1;
		}
	}
	$map_handle->close();
}


# Extract the undefined symbols contained in the binaries.

my %undef_symtbl;
foreach my $bin_file (@{$args->{ bin }})
{
	foreach my $line (`mipsel-linux-nm --dynamic -u --no-sort $bin_file`)
	{
		chomp $line;
		$undef_symtbl{ $line } += 1;
	}
}


# Find all of the undefined symbols that can be resolved.  Generate
# output suitable for subsequent use in the makefile.

foreach my $sym (sort keys %undef_symtbl)
{
	if (exists $lib_symtbl{ $sym })
	{
		print "EXTERN($sym);\n";
	}
}
