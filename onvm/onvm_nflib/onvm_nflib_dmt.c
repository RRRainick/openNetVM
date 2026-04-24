#include <rte_common.h>
#include <rte_byteorder.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_memcpy.h>
#include <rte_log.h>
#include <rte_hash.h>
#include <rte_cycles.h>
#include <errno.h>

#include "onvm_nflib_dmt.h"
#include "onvm_common.h"
#include "onvm_dmt_types.h"
#include "onvm_flow_table.h"
#include "onvm_pkt_helper.h"

#define EXPIRE_TIME 5

match_t dmt_nf_match_field __attribute__((weak)) = 0x0;
rewrite_t dmt_nf_rewrite_field __attribute__((weak)) = 0x0;

static int
update_status(uint64_t elapsed_cycles, struct onvm_dmt_mpw_data *data) {
        if (unlikely(data == NULL)) {
                return -1;
        }
        if ((elapsed_cycles - data->last_update_cycles) / rte_get_timer_hz() >= EXPIRE_TIME) {
                data->is_active = 0;
        } else {
                data->is_active = 1;
        }

        return 0;
}

static int
clear_entries(struct onvm_ft *table) {
        struct onvm_dmt_mpw_data *data = NULL;
        struct onvm_ft_dmt_tuple *key = NULL;
        uint32_t next = 0;
        int ret = 0;
        uint64_t current_cycles = rte_get_tsc_cycles();

        RTE_LOG(INFO, APP, "Clearing expired entries\n");

        while (onvm_ft_iterate(table, (const void **)&key, (void **)&data, &next) > -1) {
                if (update_status(current_cycles, data) < 0) {
                        return -1;
                }

                if (!data->is_active) {
                        ret = onvm_ft_remove_dmt_key(table, key);
                        if (ret < 0) {
                                RTE_LOG(INFO, APP, "Key should have been removed, but was not\n");
                        }
                }
        }
        return 0;
}

static int
onvm_nflib_dmt_add_mpw_entry(struct onvm_pkt_parse_ctx *parse_ctx, struct onvm_ft *mpw_table) {
        int idx;
        struct onvm_dmt_mpw_data *data = NULL;

        idx = onvm_ft_add_dmt_key_parse_ctx(mpw_table, parse_ctx, (char **)&data);
        RTE_LOG(INFO, APP, "rte_hash_count: %u, mpw_table->cnt: %u\n", rte_hash_count(mpw_table->hash), mpw_table->cnt);
        if (rte_hash_count(mpw_table->hash) >= (mpw_table->cnt) / 2) {
                clear_entries(mpw_table);
        }

        switch (idx) {
                case -EPROTONOSUPPORT:
                        RTE_LOG(INFO, APP, "Unsupported protocol\n");
                        break;
                case -EINVAL:
                        RTE_LOG(INFO, APP, "Bad argument\n");
                        break;
                case -ENOSPC:
                        RTE_LOG(INFO, APP, "Not enough space\n");
                        break;
                default:
                        data->mpw = 0;
                        data->win_idx = 0;
                        data->is_active = 1;
                        data->last_update_cycles = rte_get_tsc_cycles();
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
onvm_nflib_dmt_update_mpw_table(struct onvm_pkt_parse_ctx *parse_ctx, struct onvm_pkt_meta *meta, struct onvm_ft *mpw_table, bool hit) {
        int idx;
        struct onvm_dmt_mpw_data *data = NULL;
        win_idx_t delta = 0;

        if (!hit)
                return 0;

        idx = onvm_ft_lookup_dmt_key_parse_ctx(mpw_table, parse_ctx, (char **)&data);

        switch (idx) {
                case -ENOENT:
                        onvm_nflib_dmt_add_mpw_entry(parse_ctx, mpw_table);
                        break;
                case -EINVAL:
                        RTE_LOG(INFO, APP, "Bad argument\n");
                        break;
                case -EPROTONOSUPPORT:
                        RTE_LOG(INFO, APP, "Unsupported protocol\n");
                        break;
                default: /* match */
                        data->last_update_cycles = rte_get_tsc_cycles();
                        delta = meta->win_idx - data->win_idx;
                        // RTE_LOG(INFO, APP, "meta->win_idx: %u, data->win_idx: %u\n", meta->win_idx, data->win_idx);
                        if (delta > 0) {
                                if (delta == 1) { /* update cycle reached */
                                        meta->min_mpw = RTE_MIN(meta->min_mpw, data->mpw);
                                        // RTE_LOG(INFO, APP, "min_mpw=%u\n", meta->min_mpw);
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
                        // RTE_LOG(INFO, APP, "meta->min_mpw: %u \n", meta->min_mpw);
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
        info->mpw_table = onvm_dmt_ft_create(ONVM_NFLIB_DMT_MPW_ENTRIES, sizeof(struct onvm_dmt_mpw_data));
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
onvm_nflib_dmt_record_match_data(struct onvm_pkt_parse_ctx *parse_ctx, struct onvm_pkt_meta *meta) {
        /*
         * normal data type convert to cpu endian.
         * byte array retain network endian. See setValue function on p4-runtime
         */
        switch (parse_ctx->ether_type) {
                case RTE_ETHER_TYPE_IPV4:
                        meta->match_data.inet4_saddr = rte_be_to_cpu_32(parse_ctx->inet4_hdr->src_addr);
                        meta->match_data.inet4_daddr = rte_be_to_cpu_32(parse_ctx->inet4_hdr->dst_addr);
                        break;
                case RTE_ETHER_TYPE_IPV6:
                        onvm_pkt_ipv6_addr_copy(parse_ctx->inet6_hdr->src_addr, meta->match_data.inet6_saddr);
                        onvm_pkt_ipv6_addr_copy(parse_ctx->inet6_hdr->dst_addr, meta->match_data.inet6_daddr);
                        break;
                case CACHE_REQ_TYPE_ICN:
                        meta->match_data.icn_saddr = parse_ctx->icn_hdr->src_addr;
                        meta->match_data.icn_daddr = parse_ctx->icn_hdr->dst_addr;
                        break;
                case CACHE_REQ_TYPE_IPN:
                        meta->match_data.ipn_saddr = rte_be_to_cpu_32(parse_ctx->ipn_hdr->src_addr);
                        meta->match_data.ipn_daddr = rte_be_to_cpu_32(parse_ctx->ipn_hdr->dst_addr);
                        break;
                case CACHE_REQ_TYPE_GEO:
                        meta->match_data.geo_saddr = rte_be_to_cpu_32(parse_ctx->geo_hdr->so_pv);
                        meta->match_data.geo_daddr = rte_be_to_cpu_32(parse_ctx->geo_hdr->de_pv);
                        break;
        }

        switch (parse_ctx->inet_proto) {
                case IP_PROTOCOL_TCP:
                        meta->match_data.inet_sport = rte_be_to_cpu_16(parse_ctx->tcp_hdr->src_port);
                        meta->match_data.inet_dport = rte_be_to_cpu_16(parse_ctx->tcp_hdr->dst_port);
                        break;
                case IP_PROTOCOL_UDP:
                        meta->match_data.inet_sport = rte_be_to_cpu_16(parse_ctx->udp_hdr->src_port);
                        meta->match_data.inet_dport = rte_be_to_cpu_16(parse_ctx->udp_hdr->dst_port);
                        break;
                case IP_PROTOCOL_INVALID:
                        meta->match_data.inet_sport = 0;
                        meta->match_data.inet_dport = 0;
                        break;
        }
}

void
onvm_nflib_dmt_record_rewrite_data(struct onvm_pkt_parse_ctx *parse_ctx, struct onvm_pkt_meta *meta, port_t out_port) {
        /*
         * normal data type convert to cpu endian.
         * byte array retain network endian. See setValue function on p4-runtime
         */
        rte_ether_addr_copy(&parse_ctx->eth->s_addr, &meta->rewrite_data.eth_saddr);
        rte_ether_addr_copy(&parse_ctx->eth->d_addr, &meta->rewrite_data.eth_daddr);

        switch (parse_ctx->ether_type) {
                case RTE_ETHER_TYPE_IPV4:
                        meta->rewrite_data.inet4_saddr = rte_be_to_cpu_32(parse_ctx->inet4_hdr->src_addr);
                        meta->rewrite_data.inet4_daddr = rte_be_to_cpu_32(parse_ctx->inet4_hdr->dst_addr);
                        break;
                case RTE_ETHER_TYPE_IPV6:
                        onvm_pkt_ipv6_addr_copy(parse_ctx->inet6_hdr->src_addr, meta->rewrite_data.inet6_saddr);
                        onvm_pkt_ipv6_addr_copy(parse_ctx->inet6_hdr->dst_addr, meta->rewrite_data.inet6_daddr);
                        break;
                case CACHE_REQ_TYPE_ICN:
                        meta->rewrite_data.icn_saddr = parse_ctx->icn_hdr->src_addr;
                        meta->rewrite_data.icn_daddr = parse_ctx->icn_hdr->dst_addr;
                        break;
                case CACHE_REQ_TYPE_IPN:
                        meta->rewrite_data.ipn_saddr = rte_be_to_cpu_32(parse_ctx->ipn_hdr->src_addr);
                        meta->rewrite_data.ipn_daddr = rte_be_to_cpu_32(parse_ctx->ipn_hdr->dst_addr);
                        break;
                case CACHE_REQ_TYPE_GEO:
                        meta->rewrite_data.geo_saddr = rte_be_to_cpu_32(parse_ctx->geo_hdr->so_pv);
                        meta->rewrite_data.geo_daddr = rte_be_to_cpu_32(parse_ctx->geo_hdr->de_pv);
                        break;
        }

        meta->rewrite_data.proto = parse_ctx->inet_proto;
        switch (parse_ctx->inet_proto) {
                case IP_PROTOCOL_TCP:
                        meta->rewrite_data.inet_sport = rte_be_to_cpu_16(parse_ctx->tcp_hdr->src_port);
                        meta->rewrite_data.inet_dport = rte_be_to_cpu_16(parse_ctx->tcp_hdr->dst_port);
                        break;
                case IP_PROTOCOL_UDP:
                        meta->rewrite_data.inet_sport = rte_be_to_cpu_16(parse_ctx->udp_hdr->src_port);
                        meta->rewrite_data.inet_dport = rte_be_to_cpu_16(parse_ctx->udp_hdr->dst_port);
                        break;
                case IP_PROTOCOL_INVALID:
                        meta->rewrite_data.inet_sport = 0;
                        meta->rewrite_data.inet_dport = 0;
                        break;
        }

        meta->rewrite_data.out_port = out_port;
}

bool
onvm_nflib_dmt_do_cache(struct onvm_pkt_meta *meta, const mpw_t mpw_threshold) {
        return meta->min_mpw > mpw_threshold && meta->min_mpw != MPW_MAX;
}

void
onvm_nflib_dmt_format_cache_req(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta, struct cache_request *cache_req, action_t action, state_t state) {
        struct rte_ether_hdr *eth = onvm_pkt_ether_hdr(pkt);
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

        data->type = rte_be_to_cpu_16(eth->ether_type);
        data->action = action;
        data->state = state;
}
