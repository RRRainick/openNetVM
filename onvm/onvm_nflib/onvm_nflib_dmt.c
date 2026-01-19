#include <rte_common.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_log.h>
#include <errno.h>

#include "onvm_nflib_dmt.h"
#include "onvm_common.h"
#include "onvm_dmt_types.h"
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
                        RTE_LOG(INFO, APP, "meta->win_idx: %u, data->win_idx: %u\n", meta->win_idx, data->win_idx);
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
                                meta->min_mpw = 0; /* limit cache_req rate at 1 pps at most */
                                data->mpw++;
                        }
                        RTE_LOG(INFO, APP, "meta->min_mpw: %u \n", meta->min_mpw);
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
        info->mpw_table = onvm_ft_create(ONVM_NFLIB_DMT_MPW_ENTRIES, sizeof(struct onvm_dmt_mpw_data));
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

void
onvm_nflib_dmt_print_bitmap(struct onvm_pkt_meta *meta) {
        rewrite_t i;
        for (i = 0; i < REWRITE_LEN; i++) {
                if (meta->bitmap[i]) {
                        RTE_LOG(INFO, APP, "meta->bitmap[%u] = 0x%02hhX\n", i, meta->bitmap[i]);
                }
        }
}

void
onvm_nflib_dmt_synthesize_bitmap(struct onvm_dmt_nf_info *info, struct onvm_pkt_meta *meta) {
        rewrite_t i;
        match_t j;

        for (i = 0; i < REWRITE_LEN; i++) {
                if (ONVM_CHECK_BIT(info->rewrite_field, i)) {
                        for (j = 0; j < MATCH_LEN; j++) {
                                if (ONVM_CHECK_BIT(info->match_field, j)) {
                                        if (meta->bitmap[j] == 0) {
                                                meta->bitmap[i] = ONVM_SET_BIT(meta->bitmap[i], j);
                                        }
                                        else {
                                                meta->bitmap[i] |= meta->bitmap[j];
                                        }
                                }
                        }
                }
        }
}

void
onvm_nflib_dmt_record_match_data(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta) {
        struct rte_ipv4_hdr *ipv4 = onvm_pkt_ipv4_hdr(pkt);
        if (ipv4) {
                meta->match_data.inet4_saddr = ipv4->src_addr;
                meta->match_data.inet4_daddr = ipv4->dst_addr;
        }

        if (onvm_pkt_is_tcp(pkt)) {
                struct rte_tcp_hdr *tcp = onvm_pkt_tcp_hdr(pkt);
                if (tcp) {
                        meta->match_data.inet_sport = tcp->src_port;
                        meta->match_data.inet_dport = tcp->dst_port;
                }
        } else if (onvm_pkt_is_udp(pkt)) {
                struct rte_udp_hdr *udp = onvm_pkt_udp_hdr(pkt);
                if (udp) {
                        meta->match_data.inet_sport = udp->src_port;
                        meta->match_data.inet_dport = udp->dst_port;
                }
        }
}

void
onvm_nflib_dmt_record_rewrite_data(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta) {
        struct rte_ether_hdr *eth = onvm_pkt_ether_hdr(pkt);
        if (eth) {
                rte_ether_addr_copy(&eth->s_addr, &meta->rewrite_data.eth_saddr);
                rte_ether_addr_copy(&eth->d_addr, &meta->rewrite_data.eth_daddr);
        }

        struct rte_ipv4_hdr *ipv4 = onvm_pkt_ipv4_hdr(pkt);
        if (ipv4) {
                meta->rewrite_data.inet4_saddr = ipv4->src_addr;
                meta->rewrite_data.inet4_daddr = ipv4->dst_addr;
                meta->rewrite_data.proto = ipv4->next_proto_id;
        }

        if (onvm_pkt_is_tcp(pkt)) {
                struct rte_tcp_hdr *tcp = onvm_pkt_tcp_hdr(pkt);
                if (tcp) {
                        meta->rewrite_data.inet_sport = tcp->src_port;
                        meta->rewrite_data.inet_dport = tcp->dst_port;
                }
        } else if (onvm_pkt_is_udp(pkt)) {
                struct rte_udp_hdr *udp = onvm_pkt_udp_hdr(pkt);
                if (udp) {
                        meta->rewrite_data.inet_sport = udp->src_port;
                        meta->rewrite_data.inet_dport = udp->dst_port;
                }
        }

        meta->rewrite_data.out_port = pkt->port;
}

bool
onvm_nflib_dmt_do_cache(struct onvm_pkt_meta *meta, const mpw_t mpw_threshold) {
        return meta->min_mpw > mpw_threshold && meta->min_mpw != MPW_MAX;
}

void
onvm_nflib_dmt_format_cache_req(struct onvm_pkt_meta *meta, struct cache_request *cache_req) {
        struct cache_data *data = &cache_req->cache_data;
        rewrite_t i;

        memset(data, 0, sizeof(struct cache_data));

        for (i = 0; i < REWRITE_LEN; i++) {
                if (meta->bitmap[i] != 0) {
                        data->match_field |= meta->bitmap[i];
                        data->rewrite_field = ONVM_SET_BIT(data->rewrite_field, i);
                }
        }

        data->match_data = meta->match_data;
        data->rewrite_data = meta->rewrite_data;

        data->proto = meta->rewrite_data.proto;
}
