#ifndef _ONVM_DMT_TYPES_H_
#define _ONVM_DMT_TYPES_H_

#include <stdint.h>
#include <limits.h>

/* mpw */
typedef uint32_t win_idx_t;
typedef uint32_t mpw_t;
#define MPW_MAX UINT32_MAX

/* bitmap */
typedef uint8_t match_t;
typedef uint8_t rewrite_t;
#define MATCH_LEN (sizeof(match_t) * CHAR_BIT)
#define REWRITE_LEN (sizeof(rewrite_t) * CHAR_BIT)

#endif // _ONVM_DMT_TYPES_H_
