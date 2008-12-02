#! /bin/sh

tc qdisc add dev eth0 root handle 1: cbq bandwidth 100Mbit avpkt 1000
tc class add dev eth0 parent 1:0 classid 1:1 \
	cbq bandwidth 100Mbit rate 100Mbit allot 1514 weight 10Mbit prio 8 \
	maxburst 20 avpkt 1000
tc class add dev eth0 parent 1:1 classid 1:100 \
	cbq bandwidth 100Mbit rate 98Mbit allot 1514 \
	weight 9.8Mbit prio 5 maxburst 20 avpkt 1000 \
	split 1:0 defmap bf
tc class add dev eth0 parent 1:1 classid 1:200 \
	cbq bandwidth 100Mbit rate 2Mbit allot 1514 \
	weight 0.2Mbit prio 5 maxburst 20 avpkt 1000 \
	split 1:0 defmap 40 bounded
tc qdisc add dev eth0 parent 1:100 sfq quantum 1514b perturb 15
tc qdisc add dev eth0 parent 1:200 sfq quantum 1514b perturb 15
	
