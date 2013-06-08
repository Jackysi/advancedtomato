
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <dnscrypt/plugin.h>
#include <ldns/ldns.h>
#include <ldns/util.h>

DCPLUGIN_MAIN(__FILE__);

#define EDNS_HEADER           "4f56" "0014" "4f444e5300" "00"
#define EDNS_HEADER_CLIENT_IP "10"
#define EDNS_CLIENT_IP        "7f000001"
#define EDNS_HEADER_FODDER    "40"
#define EDNS_FODDER           "deadbeefabad1dea"

#define EDNS_DATA EDNS_HEADER \
    EDNS_HEADER_CLIENT_IP EDNS_CLIENT_IP EDNS_HEADER_FODDER EDNS_FODDER

#define EDNS_CLIENT_IP_OFFSET (sizeof EDNS_HEADER - 1U + \
                               sizeof EDNS_HEADER_CLIENT_IP - 1U)

#define EDNS_FODDER_OFFSET (sizeof EDNS_HEADER - 1U + \
                            sizeof EDNS_HEADER_CLIENT_IP - 1U + \
                            sizeof EDNS_CLIENT_IP - 1U + \
                            sizeof EDNS_HEADER_FODDER - 1U)

const char *
dcplugin_description(DCPlugin * const dcplugin)
{
    return "Apply the OpenDNS settings defined for a specific IP address";
}

const char *
dcplugin_long_description(DCPlugin * const dcplugin)
{
    return
        "The IP address must be a hex-encoded IPv4 address.\n"
        "\n"
        "Usage:\n"
        "\n"
        "# dnscrypt-proxy --plugin \\\n"
        "  libdcplugin_example_ldns_opendns_set_client_ip.la,7f000001";
}

int
dcplugin_init(DCPlugin * const dcplugin, int argc, char *argv[])
{
    char   *edns_hex;
    size_t  edns_hex_size = sizeof EDNS_DATA;

    ldns_init_random(NULL, 0U);
    edns_hex = malloc(edns_hex_size);
    dcplugin_set_user_data(dcplugin, edns_hex);
    if (edns_hex == NULL) {
        return -1;
    }
    memcpy(edns_hex, EDNS_DATA, edns_hex_size);
    assert(sizeof EDNS_CLIENT_IP - 1U == (size_t) 8U);
    if (argc > 1 && strlen(argv[1]) == (size_t) 8U) {
        memcpy(edns_hex + EDNS_CLIENT_IP_OFFSET,
               argv[1], sizeof EDNS_CLIENT_IP - 1U);
    }
    return 0;
}

int
dcplugin_destroy(DCPlugin *dcplugin)
{
    free(dcplugin_get_user_data(dcplugin));

    return 0;
}

static void
fill_with_random_hex_data(char * const str, size_t size)
{
    size_t   i = (size_t) 0U;
    uint16_t rnd;

    while (i < size) {
        rnd = ldns_get_random();
        str[i++] = "0123456789abcdef"[rnd & 0xf];
        str[i++] = "0123456789abcdef"[(rnd >> 8) & 0xf];
    }
}

DCPluginSyncFilterResult
dcplugin_sync_pre_filter(DCPlugin *dcplugin, DCPluginDNSPacket *dcp_packet)
{
    uint8_t  *new_packet;
    ldns_rdf *edns_data;
    char     *edns_data_str;
    ldns_pkt *packet;
    size_t    new_packet_size;

    ldns_wire2pkt(&packet, dcplugin_get_wire_data(dcp_packet),
                  dcplugin_get_wire_data_len(dcp_packet));

    edns_data_str = dcplugin_get_user_data(dcplugin);
    fill_with_random_hex_data(edns_data_str + EDNS_FODDER_OFFSET,
                              sizeof EDNS_FODDER - 1U);
    edns_data = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_HEX, edns_data_str);
    ldns_pkt_set_edns_data(packet, edns_data);

    ldns_pkt2wire(&new_packet, packet, &new_packet_size);
    if (dcplugin_get_wire_data_max_len(dcp_packet) >= new_packet_size) {
        dcplugin_set_wire_data(dcp_packet, new_packet, new_packet_size);
    }

    free(new_packet);
    ldns_pkt_free(packet);

    return DCP_SYNC_FILTER_RESULT_OK;
}
