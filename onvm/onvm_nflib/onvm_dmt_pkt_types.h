#ifndef _ONVM_DMT_PKT_TYPES_H_
#define _ONVM_DMT_PKT_TYPES_H_

#include <stdint.h>

#define INET6_ADDR_LEN  16 /**< Length of IPv6 address. */

#define DMT_ETHER_TYPE_ICN 0x88AB
#define DMT_ETHER_TYPE_IPN 0x8898
#define DMT_ETHER_TYPE_GEO 0x8947
#define DMT_ETHER_TYPE_MF 0x27c0
#define DMT_ETHER_TYPE_NDN 0x8624

typedef uint8_t icn_addr_t;
typedef uint32_t ipn_addr_t;
typedef uint64_t geo_addr_t;
typedef uint32_t mf_addr_t;
typedef uint16_t ndn_addr_t;

struct dmt_icn_header {
    uint8_t saved:1;
    uint8_t ctrl:7;
    icn_addr_t src_addr;
    icn_addr_t dst_addr;
} __attribute__((__packed__));

struct dmt_ipn_header {
    uint8_t version:4;
    uint8_t ctrl_high:4;
    uint16_t ctrl_low:4;
    uint16_t survival_time:12;
    ipn_addr_t src_addr;
    ipn_addr_t dst_addr;
    uint32_t create_time;
} __attribute__((__packed__));

struct dmt_geo_header {
    uint8_t basic_version_next_header;
    uint8_t basic_reserved;
    uint8_t lifetime;
    uint8_t rhl;
    uint8_t common_next_header_reserved0;
    uint8_t header_type_subtype;
    uint8_t traffic_class;
    uint8_t flags;
    uint16_t payload_length;
    uint8_t max_hop_limit;
    uint8_t common_reserved1;
    uint16_t sequence_number;
    uint16_t guc_reserved;
    geo_addr_t so_gn_addr;
    uint32_t so_timestamp;
    uint32_t so_latitude;
    uint32_t so_longitude;
    uint16_t pai_speed;
    uint16_t heading;
    geo_addr_t de_gn_addr;
    uint32_t de_timestamp;
    uint32_t de_latitude;
    uint32_t de_longitude;
} __attribute__((__packed__));

struct dmt_mf_header {
    uint32_t mf_type;
    uint32_t src_guid;
    uint32_t dest_guid;
    mf_addr_t src_na;
    mf_addr_t dest_na;
    uint32_t pld_size;
    uint32_t seq_num;
} __attribute__((__packed__));

struct dmt_ndn_header {
    uint8_t type;
    uint16_t total_len;
    uint8_t name_type;
    uint8_t name_len;
    ndn_addr_t name;
} __attribute__((__packed__));

#endif  // _ONVM_DMT_PKT_TYPES_H_
