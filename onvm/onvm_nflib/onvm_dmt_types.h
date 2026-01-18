#ifndef _ONVM_DMT_TYPES_H_
#define _ONVM_DMT_TYPES_H_

#include <limits.h>
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

#define BITMAP_L2SRC (0x01 << 0)
#define BITMAP_L2DST (0x01 << 1)
#define BITMAP_L3SRC (0x01 << 2)
#define BITMAP_L3DST (0x01 << 3)
#define BITMAP_L3PRO (0x01 << 4)
#define BITMAP_L4SRC (0x01 << 5)
#define BITMAP_L4DST (0x01 << 6)

#endif  // _ONVM_DMT_TYPES_H_
