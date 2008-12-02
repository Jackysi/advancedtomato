#! /bin/sh

tc qdisc add dev eth0 root handle 1:0 cbq bandwidth 100Mbit avpkt 1000 cell 8
tc class add dev eth0 parent 1:0 classid 1:1 cbq bandwidth 100Mbit rate 1Mbit \
	weight 0.1Mbit prio 8 allot 1514 cell 8 maxburst 20 avpkt 1000 bounded
tc qdisc add dev eth0 parent 1:1 handle 100: prio
tc qdisc add dev eth0 parent 100:1 handle 200: pfifo
tc qdisc add dev eth0 parent 100:2 handle 300: pfifo
tc qdisc add dev eth0 parent 100:3 handle 400: pfifo
tc filter add dev eth0 parent 1:0 prio 1 protocol ip u32 \
	match ip protocol 0x6 0xff flowid 1:1
