#! /bin/sh
# Strip C comments
# by Stewart Ravenhall <stewart.ravenhall@ukonline.co.uk> -- 4 October 2000
# Un-Korn-ized by Paolo Bonzini <bonzini@gnu.org> -- 24 November 2000

# Strip everything between /* and */ inclusive

# Copes with multi-line comments,
# disassociated end comment symbols,
# disassociated start comment symbols,
# multiple comments per line

# Check given file exists
program=`echo $0|sed -e 's:.*/::'`
if [ "$#" != 3 ] && [ "$1" != "-" ] && [ ! -f "$1" ]; then
        print "$program: $1 does not exist"
        exit 2
fi

# Create shell variables for ASCII 1 (control-a)
# and ASCII 2 (control-b)
a="`echo | tr '\012' '\001'`"
b="`echo | tr '\012' '\002'`"

# The script is updated to strip both c and html comments
# The original was taken here: http://sed.sourceforge.net/grabbag/scripts/remccoms2.sh.txt
if [ $2 = "c" ] ; then
	p1="\/\*"
	p2="/\*"
	p3="\*/"
	p4="/*"
	p5="*/"
else
	p1="<!--"
	p2="<!--"
	p3="-->"
	p4="<!--"
	p5="-->"
fi

sed -ri '
        # If no start comment then go to end of script
        /'"$p1"'/!b
        :a
        s:'"$p2"':'"$a"':g
        s:'"$p3"':'"$b"':g

        # If no end comment
	/'"$b"'/!{
		:b

                # If not last line then read in next one
                $!{
                        N
                        ba
                }

                # If last line then remove from start
                # comment to end of line
                # then go to end of script
                s:'"$a[^$b]"'*$::
                bc
        }

        # Remove comments
        '"s:$a[^$b]*$b"'::g
	/'"$a"'/ bb

        :c
        s:'"$a"':'"$p4"':g
        s:'"$b"':'"$p5"':g
' $1

sed -i $1 -e "/^$/d"
