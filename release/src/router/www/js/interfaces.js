MAX_BRIDGE_ID = 3;
MAX_PORT_ID = 4;
MAX_VLAN_ID = 15;
function h_countbitsfromleft(num) {
    if (num == 255 ){
        return(8);
    }
    var i = 0;
    var bitpat=0xff00;
    while (i < 8){
        if (num == (bitpat & 0xff)){
            return(i);
        }
        bitpat=bitpat >> 1;
        i++;
    }
    return(Number.NaN);
}
function numberOfBitsOnNetMask(netmask) {
    var total = 0;
    var t = netmask.split('.');
    for (var i = 0; i<= 3 ; i++) {
        total += h_countbitsfromleft(t[i]);
    }
    return total;
}
function getNetworkAddress(ipaddress, netmask) {
    return fixIP(ntoa(aton(ipaddress) & aton(netmask)));
}
function getBroadcastAddress(network, netmask) {
    return fixIP(ntoa(aton(network) ^ (~ aton(netmask))));
}
function getAddress(ipaddress, network) {
    return fixIP(ntoa( (aton(network)) + (aton(ipaddress)) ));
}
