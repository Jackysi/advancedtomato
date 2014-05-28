#!/bin/bash

resolvers=(`awk -F"," '{if (NR!=1) {print $1}}' $1`)
list="["

x=0
while [ $x -lt ${#resolvers[*]} ]
do
	list+="['${resolvers[$x]}','${resolvers[$x]}']"
	if [ $x -lt $(( ${#resolvers[*]} - 1 )) ];
	then
		list+=","
	fi
	x=$(( $x + 1 ))
done

list+="]"

sed -i 's/_dnscrypt_resolvers_/'"$list"'/' $2
