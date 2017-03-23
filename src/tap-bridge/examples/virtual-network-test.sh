#!/bin/sh
brctl addbr br-client
brctl addbr br-server
tunctl -t tap-client
tunctl -t tap-server
ifconfig tap-client 0.0.0.0 promisc up
ifconfig tap-server 0.0.0.0 promisc up
brctl addif br-client tap-client
ifconfig br-client up
brctl addif br-server tap-server
ifconfig br-server up
lxc-create -n client -t ubuntu -f lxc-client.conf
lxc-create -n server -t ubuntu -f lxc-server.conf
lxc-start -n client -d
lxc-start -n server -d
