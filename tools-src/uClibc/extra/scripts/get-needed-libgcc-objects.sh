#!/bin/sh
#
# Manuel Novoa III       Jan 2001
#
# The purpose of this script is to extract the object files from libgcc.a
# that are needed by the shared uClibc so that they won't be linked with
# each application.
#
# I'm sure people with better shell programming skills can improve this.
# Feel free!  ;-)  At this point though, it gets the job done for me.
#
# Possible problems (paranioa mode):  Are any of the objects in libgcc.a
# needed to actually load the shared library on any archs?

#Enable this when debugging
#set -x

echo Finding missing symbols in libc.a ...
echo "    partial linking..."
rm -f libc.ldr
$LD $LDFLAGS -r -o libc.ldr $CRTOBJS --whole-archive ../libc.a

if $NM --undefined-only libc.ldr 2>&1 | grep -v "^main$" | grep -v "^_GLOBAL_OFFSET_TABLE_$" | grep -v "_gp_disp" > sym.need ; then
    EXIT_WITH_ERROR=0
    rm -f obj.need
    touch obj.need
    for SYM in `cat sym.need | sed -e 's/ U //g'` ; do
	if $NM -s $LIBGCC 2>&1 | grep -q $SYM" in " ; then
	    $NM -s $LIBGCC 2>&1 | grep $SYM" in " | cut -d' ' -f3 >> obj.need
	else
	    echo Symbol $SYM needed by libc.a but not found in libgcc.a
	    EXIT_WITH_ERROR=1
	fi
    done
    if [ $EXIT_WITH_ERROR != 0 ]; then
	exit $EXIT_WITH_ERROR
    fi
else
    echo No missing symbols found.
    exit 0
fi

rm -rf tmp-gcc
mkdir tmp-gcc
(cd tmp-gcc && $AR -x $LIBGCC)
rm -f libgcc.ldr

echo Extracting referenced libgcc.a objects ...

rm -f obj.need.0
touch obj.need.0

cmp -s obj.need obj.need.0 ; state=$?
while [ -s obj.need ] && [ $state -ne 0 ] ; do
    (cd tmp-gcc && cat ../obj.need | sort | uniq | xargs $LD $LDFLAGS -r -o ../libgcc.ldr)
    cp obj.need obj.need.0
    if $NM --undefined-only libgcc.ldr | grep -v "^_GLOBAL_OFFSET_TABLE_$" > sym.need 2>&1 ; then
	for SYM in `cat sym.need | sed -e 's/ U //g'` ; do
	    if $NM -s $LIBGCC 2>&1 | grep -q $SYM" in " ; then
		$NM -s $LIBGCC 2>&1 | grep $SYM" in " | cut -d' ' -f3 >> obj.need
	    fi
	done
    fi
    cmp -s obj.need obj.need.0 ; state=$?
done

cat obj.need | sort | uniq > obj.need.0
(cd tmp-gcc && cp `cat ../obj.need` ..)

if [ -s obj.need.0 ] ; then
    echo Objects added from $LIBGCC:
    cat obj.need.0
    (cd tmp-gcc && cat ../obj.need | sort | uniq | xargs $AR r ../libgcc-need.a)
else
    echo No objects added from $LIBGCC.
fi

if [ -s sym.need ] ; then
    echo Symbols missing from libgcc.a:
    cat sym.need
else
    echo Done
fi
exit 0
