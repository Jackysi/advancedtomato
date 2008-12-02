#! /bin/sh

tc qdisc add dev $1 root handle 1:0 htb
tc class add dev $1 parent 1:0 classid 1:1 htb rate $2kbit ceil $2kbit 
tc qdisc add dev $1 parent 1:1 handle 100: prio
tc qdisc add dev $1 parent 100:1 handle 200: pfifo limit 100
tc qdisc add dev $1 parent 100:2 handle 300: pfifo limit 100
tc qdisc add dev $1 parent 100:3 handle 400: pfifo limit 100
tc filter add dev $1 parent 1:0 prio 1 protocol ip u32 \
	match ip protocol 0x0 0x0 flowid 1:1
