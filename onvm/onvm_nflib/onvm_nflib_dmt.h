#ifndef _ONVM_NFLIB_DMT_H_
#define _ONVM_NFLIB_DMT_H_

#include <stdint.h>
#include <stdbool.h>
#include <rte_mbuf.h>
#include "onvm_common.h"
#include "onvm_flow_table.h"
#include "onvm_nflib.h"

/* mpw */
#define ONVM_NFLIB_DMT_NUM_HIT_FT_ENTRIES 1024

/* bitmap */
typedef uint8_t match_t;
typedef uint8_t rewrite_t;
#define MATCH_LEN (sizeof(match_t) * CHAR_BIT)
#define REWRITE_LEN (sizeof(rewrite_t) * CHAR_BIT)

extern match_t dmt_nf_match_field;
extern rewrite_t dmt_nf_rewrite_field;


struct onvm_dmt_mpw_data {
        win_idx_t win_idx;
        mpw_t mpw;
};

struct onvm_dmt_nf_info {
        struct onvm_ft *mpw_table;
        match_t match_field;
        rewrite_t rewrite_field;
};

int onvm_nflib_dmt_update_mpw_table(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta, struct onvm_ft *mpw_table, bool hit);

struct onvm_dmt_nf_info *
onvm_nflib_dmt_get_nf_info(struct onvm_nf_local_ctx *nf_local_ctx);

/*
 * NF should define their own dmt_nf_match_field and dmt_nf_rewrite_field.
 * 0x0 if not defined
 */
void onvm_nflib_dmt_nf_setup(struct onvm_nf_local_ctx *nf_local_ctx);

void onvm_nflib_dmt_nf_cleanup(struct onvm_nf_local_ctx *nf_local_ctx);

#endif // _ONVM_NFLIB_DMT_H_
