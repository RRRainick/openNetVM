#ifndef _ONVM_DMT_PKT_TYPES_H_
#define _ONVM_DMT_PKT_TYPES_H_

#include <stdint.h>

#define INET6_ADDR_LEN  16 /**< Length of IPv6 address. */

#define DMT_ETHER_TYPE_ICN 0x88AB

typedef uint8_t icn_addr_t;

struct dmt_icn_header {
    uint8_t saved:1;
    uint8_t ctrl:7;
    icn_addr_t src_addr;
    icn_addr_t dst_addr;
} __attribute__((__packed__));

#endif  // _ONVM_DMT_PKT_TYPES_H_
