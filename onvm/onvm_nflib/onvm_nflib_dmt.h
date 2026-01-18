#ifndef _ONVM_NFLIB_DMT_H_
#define _ONVM_NFLIB_DMT_H_

#include <stdint.h>
#include <stdbool.h>
#include <rte_mbuf.h>
#include "onvm_common.h"
#include "onvm_flow_table.h"
#include "onvm_nflib.h"

#define ONVM_NFLIB_DMT_NUM_HIT_FT_ENTRIES 1024

struct onvm_dmt_mpw_data {
        win_idx_t win_idx;
        mpw_t mpw;
};

struct onvm_dmt_nf_info {
        struct onvm_ft *mpw_table;
};

int onvm_nflib_dmt_update_mpw_table(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta, struct onvm_ft *mpw_table, bool hit);

struct onvm_dmt_nf_info *
onvm_nflib_dmt_get_nf_info(struct onvm_nf_local_ctx *nf_local_ctx);

void onvm_nflib_dmt_nf_setup(struct onvm_nf_local_ctx *nf_local_ctx);

void onvm_nflib_dmt_nf_cleanup(struct onvm_nf_local_ctx *nf_local_ctx);

#endif // _ONVM_NFLIB_DMT_H_
