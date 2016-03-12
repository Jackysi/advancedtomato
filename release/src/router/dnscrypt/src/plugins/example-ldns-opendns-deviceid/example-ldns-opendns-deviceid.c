
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <ws2tcpip.h>
#endif

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>

DCPLUGIN_MAIN(__FILE__);

#define EDNS_HEADER "0004000f4f70656e444e53"
#define EDNS_DEV_ID "cafebabedeadbeef"

const char *
dcplugin_description(DCPlugin * const dcplugin)
{
    return "Add an OpenDNS device identifier to outgoing queries";
}

const char *
dcplugin_long_description(DCPlugin * const dcplugin)
{
    return
        "This plugin tags outgoing packets with a 64-bit shared secret key,\n"
        "that the OpenDNS Umbrella service uses to identify users.\n"
        "\n"
        "If you happen to have an OpenDNS Umbrella account,\n"
        "your secret key ('device') can be retrieved with:\n"
        "\n"
        "$ dig txt debug.opendns.com\n"
        "\n"
        "# env OPENDNS_DEVICE_ID='<device id>' dnscrypt-proxy --plugin \n"
        "  libdcplugin_example_ldns_opendns_deviceid.la";
}

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    char   *device_id;
    char   *device_id_env;
    char   *edns_hex;
    size_t  edns_hex_size = sizeof EDNS_HEADER EDNS_DEV_ID;

    edns_hex = malloc(sizeof EDNS_HEADER EDNS_DEV_ID);
    dcplugin_set_user_data(dcplugin, edns_hex);
    if (edns_hex == NULL) {
        return -1;
    }
    memcpy(edns_hex, EDNS_HEADER EDNS_DEV_ID, edns_hex_size);
    assert(sizeof EDNS_DEV_ID - 1U == (size_t) 16U);
    device_id = device_id_env = getenv("OPENDNS_DEVICE_ID");
    if (device_id != NULL) {
        memcpy(edns_hex + sizeof EDNS_HEADER - (size_t) 1U,
               device_id, sizeof EDNS_DEV_ID);
    }
    if (device_id_env != NULL) {
        memset(device_id_env, 0, strlen(device_id_env));
    }
    return 0;
}

int
dcplugin_destroy(DCPlugin *dcplugin)
{
    free(dcplugin_get_user_data(dcplugin));

    return 0;
}

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    uint8_t  *new_packet;
    ldns_rdf *edns_data;
    ldns_pkt *packet;
    size_t    new_packet_size;

    ldns_wire2pkt(&packet, dcplugin_get_wire_data(dcp_packet),
                  dcplugin_get_wire_data_len(dcp_packet));

    edns_data = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_HEX,
                                     dcplugin_get_user_data(dcplugin));
    ldns_pkt_set_edns_data(packet, edns_data);

    ldns_pkt2wire(&new_packet, packet, &new_packet_size);
    if (dcplugin_get_wire_data_max_len(dcp_packet) >= new_packet_size) {
        dcplugin_set_wire_data(dcp_packet, new_packet, new_packet_size);
    }

    free(new_packet);
    ldns_pkt_free(packet);

    return DCP_SYNC_FILTER_RESULT_OK;
}
