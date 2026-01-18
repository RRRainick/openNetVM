#include <rte_common.h>
#include <rte_ip.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_log.h>
#include <errno.h>

#include "onvm_nflib_dmt.h"
#include "onvm_common.h"
#include "onvm_pkt_helper.h"

match_t dmt_nf_match_field __attribute__((weak)) = 0x0;
rewrite_t dmt_nf_rewrite_field __attribute__((weak)) = 0x0;

static int
onvm_nflib_dmt_add_mpw_entry(struct rte_mbuf *pkt, struct onvm_ft *mpw_table) {
        int idx;
        struct onvm_dmt_mpw_data *data = NULL;

        idx = onvm_ft_add_pkt(mpw_table, pkt, (char **)&data);

        switch (idx) {
                case -EPROTONOSUPPORT:
                        RTE_LOG(INFO, APP, "Unsupported protocol\n");
                        break;
                case -EINVAL:
                        RTE_LOG(INFO, APP, "Bad argument\n");
                        break;
                case -ENOSPC:
                        RTE_LOG(INFO, APP, "Not enough space");
                        break;
                default:
                        data->mpw = 0;
                        data->win_idx = 0;
                        RTE_LOG(INFO, APP, "create entry\n");
                        break;
        }

        return 0;
}

static int
onvm_nflib_dmt_init_nf_info(struct onvm_nf_local_ctx *nf_local_ctx) {
        struct onvm_nf *nf = nf_local_ctx->nf;
        nf->data = rte_malloc(NULL, sizeof(struct onvm_dmt_nf_info), 0); /* nf private data, auto-free by NF Manager */

        if (!nf->data) {
                RTE_LOG(ERR, APP, "Failed to allocate memory for NF info\n");
                return -1;
        }

        return 0;
}

/************************************API**************************************/


int
onvm_nflib_dmt_update_mpw_table(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta, struct onvm_ft *mpw_table, bool hit) {
        int idx;
        struct onvm_dmt_mpw_data *data = NULL;
        win_idx_t delta = 0;

        if (!hit)
                return 0;

        idx = onvm_ft_lookup_pkt(mpw_table, pkt, (char **)&data);

        switch (idx) {
                case -ENOENT:
                        onvm_nflib_dmt_add_mpw_entry(pkt, mpw_table);
                        break;
                case -EINVAL:
                        RTE_LOG(INFO, APP, "Bad argument\n");
                        break;
                case -EPROTONOSUPPORT:
                        RTE_LOG(INFO, APP, "Unsupported protocol\n");
                        break;
                default: /* match */
                        delta = meta->win_idx - data->win_idx;
                        RTE_LOG(INFO, APP, "meta->win_idx: %u, meta->min_mpw: %u, data->win_idx: %u\n", meta->win_idx,
                                meta->min_mpw, data->win_idx);
                        if (delta > 0) {
                                if (delta == 1) { /* update cycle reached */
                                        meta->min_mpw = RTE_MIN(meta->min_mpw, data->mpw);
                                        RTE_LOG(INFO, APP, "min_mpw=%u\n", meta->min_mpw);
                                }
                                else { /* obsolete data info */
                                        meta->min_mpw = 1;
                                }
                                data->win_idx = meta->win_idx;
                                data->mpw = 1;
                        }
                        else {                   /* delta == 0 */
                                meta->min_mpw = 0; /* only one packet trigger cache request */
                                data->mpw++;
                        }
                        break;
        }

        return 0;
}

struct onvm_dmt_nf_info *
onvm_nflib_dmt_get_nf_info(struct onvm_nf_local_ctx *nf_local_ctx) {
        struct onvm_nf *nf = nf_local_ctx->nf;
        return (struct onvm_dmt_nf_info *)nf->data;
}

void
onvm_nflib_dmt_nf_setup(struct onvm_nf_local_ctx *nf_local_ctx) {
        struct onvm_dmt_nf_info *info = NULL;

        if (onvm_nflib_dmt_init_nf_info(nf_local_ctx) < 0) {
                rte_exit(EXIT_FAILURE, "Unable to initialize NF info\n");
        }

        info = onvm_nflib_dmt_get_nf_info(nf_local_ctx);
        info->mpw_table = onvm_ft_create(ONVM_NFLIB_DMT_NUM_HIT_FT_ENTRIES, sizeof(struct onvm_dmt_mpw_data));
        if (info->mpw_table == NULL) {
                rte_exit(EXIT_FAILURE, "Unable to create mpw table\n");
        }

        info->match_field = dmt_nf_match_field;
        info->rewrite_field = dmt_nf_rewrite_field;
        RTE_LOG(INFO, APP, "match_field: 0x%02hhX, rewrite_field: 0x%02hhX\n", info->match_field, info->rewrite_field);

        return;
}

void
onvm_nflib_dmt_nf_cleanup(struct onvm_nf_local_ctx *nf_local_ctx) {
        struct onvm_dmt_nf_info *info = onvm_nflib_dmt_get_nf_info(nf_local_ctx);

        onvm_ft_free(info->mpw_table);
}
