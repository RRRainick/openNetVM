#ifndef _ONVM_DMT_TYPES_H_
#define _ONVM_DMT_TYPES_H_

#include <limits.h>
#include <rte_ether.h>
#include <stdint.h>
#include "onvm_dmt_pkt_types.h"

/* mpw */
typedef uint32_t win_idx_t;
typedef uint32_t mpw_t;
#define MPW_MAX UINT32_MAX

/* bitmap */
typedef uint8_t match_t;
typedef uint8_t rewrite_t;
#define MATCH_LEN (sizeof(match_t) * CHAR_BIT)
#define REWRITE_LEN (sizeof(rewrite_t) * CHAR_BIT)

/* bitmap field */
#define BITMAP_L2_SADDR (0x01 << 0)
#define BITMAP_L2_DADDR (0x01 << 1)
#define BITMAP_L3_SADDR (0x01 << 2)
#define BITMAP_L3_DADDR (0x01 << 3)
#define BITMAP_L3_PROTO (0x01 << 4)
#define BITMAP_L4_SPROT (0x01 << 5)
#define BITMAP_L4_DPORT (0x01 << 6)

typedef uint8_t port_t;
typedef uint8_t action_t;
typedef uint8_t state_t;
/* cache_req action */
#define CACHE_REQ_ACTION_INSERT 1
#define CACHE_REQ_ACTION_REMOVE 2
#define CACHE_REQ_ACTION_REMOVE_ALL 3
/* cache_req state */
#define CACHE_REQ_STATE_STATELESS 1
#define CACHE_REQ_STATE_STATIC 2
#define CACHE_REQ_STATE_DYNAMIC 3
#define CACHE_REQ_STATE_PAYLOAD 4
/* cache_req type(ether type) */
typedef uint16_t type_t;
#define CACHE_REQ_TYPE_IPV4 RTE_ETHER_TYPE_IPV4
#define CACHE_REQ_TYPE_IPV6 RTE_ETHER_TYPE_IPV6
#define CACHE_REQ_TYPE_ICN DMT_ETHER_TYPE_ICN
#define CACHE_REQ_TYPE_IPN DMT_ETHER_TYPE_IPN
#define CACHE_REQ_TYPE_GEO DMT_ETHER_TYPE_GEO
#define CACHE_REQ_TYPE_MF DMT_ETHER_TYPE_MF
#define CACHE_REQ_TYPE_NDN DMT_ETHER_TYPE_NDN


struct dmt_match_data {
        union {
                uint32_t inet4_saddr;
                uint8_t  inet6_saddr[INET6_ADDR_LEN];
                icn_addr_t icn_saddr;
                ipn_addr_t ipn_saddr;
                geo_addr_t geo_gn_saddr;
                mf_addr_t mf_na_saddr;
                ndn_addr_t ndn_name_saddr;
        };
        union {
                uint32_t inet4_daddr;
                uint8_t  inet6_daddr[INET6_ADDR_LEN];
                icn_addr_t icn_daddr;
                ipn_addr_t ipn_daddr;
                geo_addr_t geo_gn_daddr;
                mf_addr_t mf_na_daddr;
                ndn_addr_t ndn_name_daddr;
        };
        uint16_t inet_sport;
        uint16_t inet_dport;
}__attribute__((packed));

struct dmt_rewrite_data {
        union {
                uint32_t inet4_saddr;
                uint8_t  inet6_saddr[INET6_ADDR_LEN];
                icn_addr_t icn_saddr;
                ipn_addr_t ipn_saddr;
                geo_addr_t geo_gn_saddr;
                mf_addr_t mf_na_saddr;
                ndn_addr_t ndn_name_saddr;
        };
        union {
                uint32_t inet4_daddr;
                uint8_t  inet6_daddr[INET6_ADDR_LEN];
                icn_addr_t icn_daddr;
                ipn_addr_t ipn_daddr;
                geo_addr_t geo_gn_daddr;
                mf_addr_t mf_na_daddr;
                ndn_addr_t ndn_name_daddr;
        };
        uint16_t inet_sport;
        uint16_t inet_dport;
        uint8_t proto;
        struct rte_ether_addr eth_daddr;
        port_t out_port; /* out port on p4 switch */
        uint8_t dec_ttl;
        struct rte_ether_addr eth_saddr;
}__attribute__((packed));

/* cache_req data */
struct cache_data {
        action_t action;
        state_t state;
        match_t match_field;
        rewrite_t rewrite_field;
        struct dmt_match_data match_data;
        struct dmt_rewrite_data rewrite_data;
        type_t type;
}__attribute__((packed));

#endif  // _ONVM_DMT_TYPES_H_
