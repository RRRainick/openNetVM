#ifndef _ONVM_NFLIB_DMT_H_
#define _ONVM_NFLIB_DMT_H_

#include <stdint.h>
#include <stdbool.h>
#include <rte_mbuf.h>
#include "onvm_dmt_types.h"
#include "onvm_flow_table.h"
#include "onvm_nflib.h"

#define ONVM_NFLIB_DMT_MPW_ENTRIES 1024

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

void
onvm_nflib_dmt_print_bitmap(struct onvm_pkt_meta *meta);

void
onvm_nflib_dmt_synthesize_bitmap(struct onvm_dmt_nf_info *info, struct onvm_pkt_meta *meta);
/*
 * NF should define their own dmt_nf_match_field and dmt_nf_rewrite_field.
 * 0x0 if not defined
 */
void
onvm_nflib_dmt_nf_setup(struct onvm_nf_local_ctx *nf_local_ctx);

void
onvm_nflib_dmt_nf_cleanup(struct onvm_nf_local_ctx *nf_local_ctx);

void
onvm_nflib_dmt_record_match_data(struct onvm_pkt_parse_ctx *parse_ctx, struct onvm_pkt_meta *meta);

void
onvm_nflib_dmt_record_rewrite_data(struct onvm_pkt_parse_ctx *parse_ctx, struct onvm_pkt_meta *meta, port_t out_port);


bool
onvm_nflib_dmt_do_cache(struct onvm_pkt_meta *meta, const mpw_t mpw_threshold);

void
onvm_nflib_dmt_format_cache_req(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta, struct cache_request *cache_req, action_t action, state_t state);

#endif // _ONVM_NFLIB_DMT_H_
