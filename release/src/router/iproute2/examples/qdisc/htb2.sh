#! /bin/sh

tc qdisc add dev $1 root handle 1:0 htb default 200
tc class add dev $1 parent 1:0 classid 1:1 htb rate $2kbit ceil $2kbit

tc class add dev $1 parent 1:1 classid 1:100 htb rate $3kbit ceil $2kbit 
tc class add dev $1 parent 1:1 classid 1:200 htb rate $4kbit ceil $2kbit 

tc qdisc add dev $1 parent 1:100 handle 100: pfifo limit 100
tc qdisc add dev $1 parent 1:200 handle 200: pfifo limit 100

tc filter add dev $1 parent 1:0 prio 1 protocol ip u32 \
	match ip tos 0x10 0xff flowid 1:100
