//	<% wlifaces(1); %>
//	<% wlbands(1); %>
function wl_unit(uidx)
{
    return wl_ifaces[uidx][2];
}
function wl_sunit(uidx)
{
    return wl_ifaces[uidx][3];
}
function wl_fface(uidx)
{
    return wl_ifaces[uidx][1];
}
function wl_uidx(unit)
{
    for (var u = 0; u < wl_ifaces.length; ++u) {
        if (wl_ifaces[u][2] == unit) return u;
    }
    return -1;
}
function wl_ifidx(ifname)
{
    for (var u = 0; u < wl_ifaces.length; ++u) {
        if (wl_ifaces[u][0] == ifname) return u;
    }
    return -1;
}
function wl_ifidxx(ifname)
{
    for (var u = 0; u < wl_ifaces.length; ++u) {
        if (wl_ifaces[u][1] == ifname) return u;
    }
    return -1;
}
function wl_display_ifname(uidx)
{
    return ((wl_bands[uidx].length == 1) ? ((wl_bands[uidx][0] == '1') ? '5 GHz / ' : '2.4 GHz / ') : '') + wl_ifaces[uidx][0];
}
