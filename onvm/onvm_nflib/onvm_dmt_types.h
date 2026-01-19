#ifndef _ONVM_DMT_TYPES_H_
#define _ONVM_DMT_TYPES_H_

#include <limits.h>
#include <rte_ether.h>
#include <stdint.h>

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

/* cache_req action */
#define CACHE_REQ_ACTION_INSERT 1
#define CACHE_REQ_ACTION_REMOVE 2
#define CACHE_REQ_ACTION_REMOVE_ALL 3
/* cache_req state */
#define CACHE_REQ_STATE_STATELESS 1
#define CACHE_REQ_STATE_STATIC 2
#define CACHE_REQ_STATE_DYNAMIC 3
#define CACHE_REQ_STATE_PAYLOAD 4

struct dmt_match_data {
        union {
                uint32_t inet4_saddr;
        };
        union {
                uint32_t inet4_daddr;
        };
        uint16_t inet_sport;
        uint16_t inet_dport;
};

struct dmt_rewrite_data {
        union {
                uint32_t inet4_saddr;
        };
        union {
                uint32_t inet4_daddr;
        };
        uint16_t inet_sport;
        uint16_t inet_dport;
        uint8_t proto;
        struct rte_ether_addr eth_saddr;
        uint8_t out_port;
        uint8_t dec_ttl;
        struct rte_ether_addr eth_daddr;
};

/* cache_req data */
struct cache_data {
        uint8_t action;
        uint8_t state;
        match_t match_field;
        rewrite_t rewrite_field;
        struct dmt_match_data match_data;
        struct dmt_rewrite_data rewrite_data;
        uint8_t proto;
};

#endif  // _ONVM_DMT_TYPES_H_
