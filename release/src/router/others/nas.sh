#!/bin/sh

# wlX -> 0, 0.1, etc...
# br0, br1...br3
if [ "$2" = "" ]; then
	exit 1
fi

CRYPTO=$(nvram get wl${1}_crypto)
[ "${CRYPTO}" = "tkip" -o "${CRYPTO}" = "aes" -o "${CRYPTO}" = "aes+tkip" -o "${CRYPTO}" = "tkip+aes" ] && {
	[ "${CRYPTO}" = "tkip" ] && CRYPTO_NUM=2
	[ "${CRYPTO}" = "aes" ] && CRYPTO_NUM=4
	[ "${CRYPTO}" = "aes+tkip" ] && CRYPTO_NUM=6
	[ "${CRYPTO}" = "tkip+aes" ] && CRYPTO_NUM=6
	WIFI_IFNAME=$(nvram get wl${1}_ifname)
	BRIDGE_IFNAME=$2
	AUTH_MODE=$(nvram get wl${1}_akm)
	SSID=$(nvram get wl${1}_ssid)
	GTK_REKEY=$(nvram get wl${1}_wpa_gtk_rekey)
	[ "${GTK_REKEY}" = "" ] && exit 1
	[ "${AUTH_MODE}" = "psk" -o "${AUTH_MODE}" = "psk2" -o "${AUTH_MODE}" = "psk psk2" -o "${AUTH_MODE}" = "wpa" -o "${AUTH_MODE}" = "wpa2" -o "${AUTH_MODE}" = "wpa wpa2" ] && {
		[ "${AUTH_MODE}" = "wpa" ] && AUTH_NUM=2
		[ "${AUTH_MODE}" = "wpa2" ] && AUTH_NUM=64
		[ "${AUTH_MODE}" = "wpa wpa2" ] && AUTH_NUM=66
		[ "${AUTH_MODE}" = "psk" ] && AUTH_NUM=4
		[ "${AUTH_MODE}" = "psk2" ] && AUTH_NUM=128
		[ "${AUTH_MODE}" = "psk psk2" ] && AUTH_NUM=132
		PSK=$(nvram get wl${1}_wpa_psk)
		[ "${PSK}" = "" ] && exit 1
		echo nas -P /var/run/nas${2}.pid -l "${BRIDGE_IFNAME}" -H 34954 -i "${WIFI_IFNAME}" -A -m ${AUTH_NUM} -k "${PSK}" -s "${SSID}" -w "${CRYPTO_NUM}" -g "${GTK_REKEY}" > /etc/nas${2}.conf
		nas -P /var/run/nas${2}.pid -l "${BRIDGE_IFNAME}" -H 34954 -i "${WIFI_IFNAME}" -A -m ${AUTH_NUM} -k "${PSK}" -s "${SSID}" -w "${CRYPTO_NUM}" -g "${GTK_REKEY}" &
		exit 0
	}
	[ "${AUTH_MODE}" = "radius" ] && {
		RADIUS_PSK=$(nvram get wl${1}_radius_key)
		RADIUS_IP=$(nvram get wl${1}_radius_ipaddr)
		RADIUS_PORT=$(wl${1}_radius_port)
		[ "${RADIUS_PSK}" = "" ] && exit 1
		[ "${RADIUS_IP}" = "" ] && exit 1
		[ "${RADIUS_PORT}" = "" ] && exit 1
		echo nas -P /var/run/nas${2}.pid -l "${BRIDGE_IFNAME}" -H 34954 -i "${WIFI_IFNAME}" -A -m 0 -h "${RADIUS_IP}" -p "${RADIUS_PORT}" -k "${RADIUS_PSK}" -s "${SSID}" -w "${CRYPTO_NUM}" -g "${GTK_REKEY}" > /etc/nas${2}.conf
		nas -P /var/run/nas${2}.pid -l "${BRIDGE_IFNAME}" -H 34954 -i "${WIFI_IFNAME}" -A -m 0 -h "${RADIUS_IP}" -p "${RADIUS_PORT}" -k "${RADIUS_PSK}" -s "${SSID}" -w "${CRYPTO_NUM}" -g "${GTK_REKEY}" &
	}
}

