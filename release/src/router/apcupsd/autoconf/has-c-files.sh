#!/bin/sh
/bin/ls *.c > /dev/null 2>&1
if test $? = 0
then
    echo yes
else
    echo no
fi
