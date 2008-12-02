#! /bin/sh

tc qdisc add dev eth0 root handle 1: prio
tc qdisc add dev eth0 parent 1:1 handle 10: pfifo
tc qdisc add dev eth0 parent 1:2 handle 20: pfifo
tc qdisc add dev eth0 parent 1:3 handle 30: pfifo
