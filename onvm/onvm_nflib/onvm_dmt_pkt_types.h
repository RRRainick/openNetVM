#ifndef _ONVM_DMT_PKT_TYPES_H_
#define _ONVM_DMT_PKT_TYPES_H_

#include <stdint.h>

#define INET6_ADDR_LEN  16 /**< Length of IPv6 address. */

#define DMT_ETHER_TYPE_ICN 0x88AB
#define DMT_ETHER_TYPE_IPN 0x8898

typedef uint8_t icn_addr_t;
typedef uint32_t ipn_addr_t;

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

#endif  // _ONVM_DMT_PKT_TYPES_H_
