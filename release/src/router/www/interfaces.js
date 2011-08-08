function getNetworkAddress(ipaddress, netmask) {
	return fixIP(ntoa(aton(ipaddress) & aton(netmask)));
}

function getBroadcastAddress(network, netmask) {
	return fixIP(ntoa(aton(network) ^ (~ aton(netmask))));
}

