#!/bin/sh
#
# Supplementary MOTD script with information about the network / router settings
#
# Version 0.5 written by Monter, modified by Shibby
#
MOTD_ON=`/bin/nvram get sshd_motd`

#only if enable
if /usr/bin/[ "$MOTD_ON" == "1" ]; then

PROFILE=$1
if /usr/bin/[ "$PROFILE" == "init" ]; then
sed -i "/mymotd/d" /root/.profile
/bin/echo "/usr/sbin/mymotd" >> /root/.profile
else

# Function calculates number of bit in a netmask
#
mask2cidr() {
    nbits=0
    IFS=.
    for dec in $1 ; do
        case $dec in
            255) let nbits+=8;;
            254) let nbits+=7;;
            252) let nbits+=6;;
            248) let nbits+=5;;
            240) let nbits+=4;;
            224) let nbits+=3;;
            192) let nbits+=2;;
            128) let nbits+=1;;
            0);;
            *) /bin/echo "Error: $dec is not recognised"; exit 1
        esac
    done
    /bin/echo "$nbits"
}

LAN1=`/bin/nvram get lan1_ipaddr | /usr/bin/wc -w`
LAN2=`/bin/nvram get lan2_ipaddr | /usr/bin/wc -w`
LAN3=`/bin/nvram get lan3_ipaddr | /usr/bin/wc -w`
WAN1=`/bin/nvram get wan_ipaddr | /bin/grep -v "0.0.0.0" | /usr/bin/wc -w`
WAN2=`/bin/nvram get wan2_ipaddr | /bin/grep -v "0.0.0.0" | /usr/bin/wc -w`
MULTI=`/bin/nvram show | /bin/grep 'wan3_' | /usr/bin/wc -l`
if [ "$MULTI" -gt "0" ]; then
    MULTIWAN=1
    WAN3=`/bin/nvram get wan3_ipaddr | /bin/grep -v "0.0.0.0" | /usr/bin/wc -w`
    WAN4=`/bin/nvram get wan4_ipaddr | /bin/grep -v "0.0.0.0" | /usr/bin/wc -w`
else
    MULTIWAN=0
fi
DUAL=`/bin/nvram get landevs | /bin/grep wl1 | /usr/bin/wc -l`
TRI=`/bin/nvram get landevs | /bin/grep wl2 | /usr/bin/wc -l`

LAN_MASK=`/bin/nvram get lan_netmask`
LAN_NUMBITS=$(mask2cidr $LAN_MASK)

/bin/echo -e "\033[1;34m ======================================================== \033[0m"
/bin/echo -e "\033[1;32m Welcome to the `/bin/nvram get t_model_name` [`/bin/nvram get router_name`]\033[0m"
/bin/echo -e "\033[1;31m Uptime: `/usr/bin/uptime | /bin/sed -e 's/,  load/\n Load/'`\033[0m"
MEMBUFF=`/bin/nvram get t_cafree`
if /usr/bin/[ "$MEMBUFF" == 1 ]; then
  /usr/bin/awk '/MemTotal:/{total=$2} \
       /MemFree:/{free=$2} \
       /Buffers:/{buffers=$2} \
       /^Cached:/{cached=$2} \
       END{ \
        printf " Mem usage: %0.1f", ((total-free-buffers-cached)*100/total); printf "%c", 37; \
        printf " (used %0.2f", (total-free-buffers-cached)/1024; printf " of %0.2f", total/1024; printf " MB)\n"; \
      }' /proc/meminfo
  if /usr/bin/[ `cat /proc/meminfo | /bin/grep SwapTotal | /usr/bin/awk '{ print $2 }'` -gt 0 ]; then
  /usr/bin/awk '/SwapTotal:/{stotal=$2} \
       /SwapFree:/{sfree=$2} \
       /SwapCached:/{scached=$2} \
       END{ \
        printf " Swap usage: %0.1f", ((stotal-sfree-scached)*100/stotal); printf "%c", 37; \
        printf " (used %0.2f", (stotal-sfree-scached)/1024; printf " of %0.2f", stotal/1024; printf " MB)\n"; \
       }' /proc/meminfo
  fi
else
  /usr/bin/awk '/MemTotal:/{total=$2} \
       /MemFree:/{free=$2} \
       END{ \
        printf " Mem : used %0.1f", ((total-free)*100/total); printf "%c", 37; \
        printf " (%0.2f", (total-free)/1024; printf " of %0.2f", total/1024; printf " MB)\n"; \
      }' /proc/meminfo
  if /usr/bin/[ `cat /proc/meminfo | /bin/grep SwapTotal | /usr/bin/awk '{ print $2 }'` -gt 0 ]; then
  /usr/bin/awk '/SwapTotal:/{stotal=$2} \
       /SwapFree:/{sfree=$2} \
       END{ \
        printf " Swap usage: %0.1f", ((stotal-sfree)*100/stotal); printf "%c", 37; \
        printf " (used %0.2f", (stotal-sfree)/1024; printf " of %0.2f", stotal/1024; printf " MB)\n"; \
       }' /proc/meminfo
  fi
fi
if /usr/bin/[ "$WAN1" == "1" ]; then 
    WAN1_MASK=`/bin/nvram get wan_netmask`
    WAN1_NUMBITS=$(mask2cidr $WAN1_MASK)
    /bin/echo " WAN1: `/bin/nvram get wan_ipaddr`/$WAN1_NUMBITS @ `/bin/nvram get wan_hwaddr`"
fi
if /usr/bin/[ "$WAN2" == "1" ]; then 
    WAN2_MASK=`/bin/nvram get wan2_netmask`
    WAN2_NUMBITS=$(mask2cidr $WAN2_MASK)
    /bin/echo " WAN2: `/bin/nvram get wan2_ipaddr`/$WAN2_NUMBITS @ `/bin/nvram get wan2_hwaddr`"
fi
if /usr/bin/[ "$MULTIWAN" == "1" ]; then
    if /usr/bin/[ "$WAN3" == "1" ]; then 
        WAN3_MASK=`/bin/nvram get wan3_netmask`
        WAN3_NUMBITS=$(mask2cidr $WAN3_MASK)
        /bin/echo " WAN3: `/bin/nvram get wan3_ipaddr`/$WAN3_NUMBITS @ `/bin/nvram get wan3_hwaddr`"
    fi
    if /usr/bin/[ "$WAN4" == "1" ]; then 
        WAN4_MASK=`/bin/nvram get wan4_netmask`
        WAN4_NUMBITS=$(mask2cidr $WAN4_MASK)
        /bin/echo " WAN4: `/bin/nvram get wan4_ipaddr`/$WAN4_NUMBITS @ `/bin/nvram get wan4_hwaddr`"
    fi
fi
/bin/echo " LAN : `/bin/nvram get lan_ipaddr`/$LAN_NUMBITS @ DHCP: `/bin/nvram get dhcpd_startip` - `/bin/nvram get dhcpd_endip`"
if /usr/bin/[ "$LAN1" == "1" ]; then 
    LAN1_MASK=`/bin/nvram get lan1_netmask`
    LAN1_NUMBITS=$(mask2cidr $LAN1_MASK)
    /bin/echo " LAN1: `/bin/nvram get lan1_ipaddr`/$LAN1_NUMBITS @ DHCP: `/bin/nvram get dhcpd1_startip` - `/bin/nvram get dhcpd1_endip`";
fi
if /usr/bin/[ "$LAN2" == "1" ]; then 
    LAN2_MASK=`/bin/nvram get lan2_netmask`
    LAN2_NUMBITS=$(mask2cidr $LAN2_MASK)
    /bin/echo " LAN2: `/bin/nvram get lan2_ipaddr`/$LAN2_NUMBITS @ DHCP: `/bin/nvram get dhcpd2_startip` - `/bin/nvram get dhcpd2_endip`";
fi
if /usr/bin/[ "$LAN3" == "1" ]; then 
    LAN3_MASK=`/bin/nvram get lan3_netmask`
    LAN3_NUMBITS=$(mask2cidr $LAN3_MASK)
    /bin/echo " LAN3: `/bin/nvram get lan3_ipaddr`/$LAN3_NUMBITS @ DHCP: `/bin/nvram get dhcpd3_startip` - `/bin/nvram get dhcpd3_endip`";
fi
GETCH=`/bin/nvram get wl0_channel`
if /usr/bin/[ "$GETCH" == 0 ]; then CH="auto"; else CH=$GETCH; fi
NBAND=`/bin/nvram get wl0_nband`
if /usr/bin/[ "$NBAND" == 2 ]; then BAND="2,4GHz"; else BAND="5GHz"; fi
/bin/echo " WL0 : $BAND @ `/bin/nvram get wl0_ssid` @ channel: `/bin/nvram get wl0_country`$CH @ `/bin/nvram get wl0_hwaddr`"
if /usr/bin/[ "$DUAL" == "1" ]; then
    GETCH=`/bin/nvram get wl1_channel`
    if /usr/bin/[ "$GETCH" == 0 ]; then CH="auto"; else CH=$GETCH; fi
    NBAND=`/bin/nvram get wl1_nband`
    if /usr/bin/[ "$NBAND" == 2 ]; then BAND="2,4GHz"; else BAND="5GHz"; fi
    /bin/echo " WL1 : $BAND @ `/bin/nvram get wl1_ssid` @ channel: `/bin/nvram get wl1_country`$CH @ `/bin/nvram get wl1_hwaddr`"
fi
if /usr/bin/[ "$TRI" == "1" ]; then
    GETCH=`/bin/nvram get wl2_channel`
    if /usr/bin/[ "$GETCH" == 0 ]; then CH="auto"; else CH=$GETCH; fi
    NBAND=`/bin/nvram get wl2_nband`
    if /usr/bin/[ "$NBAND" == 2 ]; then BAND="2,4GHz"; else BAND="5GHz"; fi
    /bin/echo " WL2 : $BAND @ `/bin/nvram get wl2_ssid` @ channel: `/bin/nvram get wl2_country`$CH @ `/bin/nvram get wl2_hwaddr`"
fi
STHMD_ISENABLED=`/usr/sbin/cru l | /bin/grep "perm_on" | /usr/bin/wc -l`
STHMD_SUNENABLED=`/usr/sbin/cru l | /bin/grep "sun_on" | /usr/bin/wc -l`
STHMD_SUNSET=`/usr/sbin/cru l | /bin/grep "stealthsunset" | /usr/bin/awk '{ print $2" "$1 }'`
STHMD_SUNRISE=`/usr/sbin/cru l | /bin/grep "stealthsunrise" | /usr/bin/awk '{ print $2" "$1 }'`
STHMD_SCHON=`/usr/sbin/cru l | /bin/grep "stealthsheduleon" | /usr/bin/awk '{ print $2" "$1 }'`
STHMD_SCHOFF=`/usr/sbin/cru l | /bin/grep "stealthsheduleoff" | /usr/bin/awk '{ print $2" "$1 }'`
STHMD_SCHONC=`/bin/echo $STHMD_SCHON | /usr/bin/wc -c`
STHMD_SCHOFFC=`/bin/echo $STHMD_SCHOFF | /usr/bin/wc -c`
if /usr/bin/[ $STHMD_ISENABLED -gt 0 -o $STHMD_SUNENABLED -gt 0 -o $STHMD_SCHONC -gt 1 -o $STHMD_SCHOFFC -gt 1 ]; then
/bin/echo " -------------------------------------------------------- "
  if /usr/bin/[ $STHMD_ISENABLED -gt 0 ]; then
  /bin/echo " Detected the presence of stealthMode which is now active"
  fi
  if /usr/bin/[ $STHMD_SUNENABLED -gt 0 ]; then
  /bin/echo -e "  stealthMode Sunset mode is currently active and is set\n         for sunset at $( /bin/echo $STHMD_SUNSET | /usr/bin/awk '{$1=sprintf("%02d", $1);$2=sprintf("%02d", $2); print $1":"$2 }' ) and sunrise at $( /bin/echo $STHMD_SUNRISE | /usr/bin/awk '{$1=sprintf("%02d", $1);$2=sprintf("%02d", $2); print $1":"$2 }' )"
  fi
  if /usr/bin/[ $STHMD_SCHONC -gt 1 -a $STHMD_SCHOFFC -gt 1 ]; then
  /bin/echo -e "  stealthMode Scheduled mode is currently active and is\n       set for sunset at $( /bin/echo $STHMD_SCHON | /usr/bin/awk '{$1=sprintf("%02d", $1);$2=sprintf("%02d", $2); print $1":"$2 }' ) and sunrise at $( /bin/echo $STHMD_SCHOFF | /usr/bin/awk '{$1=sprintf("%02d", $1);$2=sprintf("%02d", $2); print $1":"$2 }' )"
  fi
fi
/bin/echo -e "\033[1;34m ======================================================== \033[0m"
/bin/echo ""
fi
fi
