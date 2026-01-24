/*********************************************************************
 *                     openNetVM
 *              https://sdnfv.github.io
 *
 *   BSD LICENSE
 *
 *   Copyright(c)
 *            2015-2019 George Washington University
 *            2015-2019 University of California Riverside
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * The name of the author may not be used to endorse or promote
 *       products derived from this software without specific prior
 *       written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * dmt_ingress.c - record match data, then forward packet to destination NF
 ********************************************************************/

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <unistd.h>

#include <rte_malloc.h>
#include <rte_common.h>
#include <rte_ip.h>
#include <rte_mbuf.h>

#include "onvm_common.h"
#include "onvm_dmt_pkt_types.h"
#include "onvm_dmt_types.h"
#include "onvm_nflib.h"
#include "onvm_nflib_dmt.h"
#include "onvm_pkt_helper.h"
#include "rte_ether.h"

#define NF_TAG "dmt_ingress"

match_t dmt_nf_match_field = 0;
match_t dmt_nf_rewrite_field = 0;

#define DMT_IPV4_IDX 0
#define DMT_IPV6_IDX 1
#define DMT_ICN_IDX 2
#define DMT_IPN_IDX 3
#define DMT_NUM_NF 4

static uint16_t destinations[DMT_NUM_NF];
static uint8_t dest_action;

static const struct rte_ether_addr TARGET_SRC_MAC = {
    .addr_bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
};

/* number of package between each print */
static uint32_t print_delay = 1000000;

/*
 * Print a usage message
 */
static void
usage(const char *progname) {
        printf("Usage:\n");
        printf("%s [EAL args] -- [NF_LIB args] -- -p <print_delay>\n", progname);
        printf("%s -F <CONFIG_FILE.json> [EAL args] -- [NF_LIB args] -- [NF args]\n\n", progname);
        printf("Flags:\n");
        printf(" - `-d DST_ARRAY`: Destination Service IDs to forward to. A comma-separated list of %d IDs for IPV4, IPV6, ICN, IPN traffic.\n", DMT_NUM_NF);
        printf(" - `-t DST_ARRAY`: Destination Port IDs to forward to. A comma-separated list of %d IDs for IPV4, IPV6, ICN, IPN traffic.\n", DMT_NUM_NF);
        printf(" - `-p <print_delay>`: number of packets between each print, e.g. `-p 1` prints every packets.\n");
}

/*
 * Parse the application arguments.
 */
static int
parse_app_args(int argc, char *argv[], const char *progname) {
        int c;
        int dst_flag = 0;
        int port_flag = 0;
        char *dst_array_str = NULL;

        while ((c = getopt(argc, argv, "d:p:t:")) != -1) {
                switch (c) {
                        case 'd':
                                dst_array_str = strdup(optarg);
                                dest_action = ONVM_NF_ACTION_TONF;
                                dst_flag = 1;
                                break;
                        case 't':
                                dst_array_str = strdup(optarg);
                                dest_action = ONVM_NF_ACTION_OUT;
                                port_flag = 1;
                                break;
                        case 'p':
                                print_delay = strtoul(optarg, NULL, 10);
                                break;
                        case '?':
                                usage(progname);
                                if (optopt == 'p')
                                        RTE_LOG(INFO, APP, "Option -%c requires an argument.\n", optopt);
                                else if (isprint(optopt))
                                        RTE_LOG(INFO, APP, "Unknown option `-%c'.\n", optopt);
                                else
                                        RTE_LOG(INFO, APP, "Unknown option character `\\x%x'.\n", optopt);
                                free(dst_array_str);
                                return -1;
                        default:
                                usage(progname);
                                free(dst_array_str);
                                return -1;
                }
        }

        if (dst_flag && port_flag) {
                RTE_LOG(INFO, APP, "%s -d and -t options are mutually exclusive.\n", NF_TAG);
                free(dst_array_str);
                return -1;
        }

        if (dst_flag || port_flag) {
                char *token;
                int i = 0;
                for (token = strtok(dst_array_str, ","); token != NULL && i < DMT_NUM_NF; token = strtok(NULL, ",")) {
                    destinations[i++] = strtoul(token, NULL, 10);
                }

                if (i != DMT_NUM_NF) {
                    RTE_LOG(INFO, APP, "Expected %d destinations, but got %d.\n", DMT_NUM_NF, i);
                    usage(progname);
                    free(dst_array_str);
                    return -1;
                }
        }

        free(dst_array_str);

        if (!dst_flag && !port_flag) {
                RTE_LOG(INFO, APP, "%s requires a destination NF with the -d flag or a port with the -t flag.\n", NF_TAG);
                return -1;
        }
        return optind;
}

/*
 * This function displays stats. It uses ANSI terminal codes to clear
 * screen when called. It is called from a single non-master
 * thread in the server process, when the process is run with more
 * than one lcore enabled.
 */
static void
do_stats_display(struct rte_mbuf *pkt) {
        const char clr[] = {27, '[', '2', 'J', '\0'};
        const char topLeft[] = {27, '[', '1', ';', '1', 'H', '\0'};
        static uint64_t pkt_process = 0;

        struct rte_ipv4_hdr *ip;

        pkt_process += print_delay;

        /* Clear screen and move to top left */
        printf("%s%s", clr, topLeft);

        printf("PACKETS\n");
        printf("-----\n");
        printf("Port : %d\n", pkt->port);
        printf("Size : %d\n", pkt->pkt_len);
        printf("Type : %d\n", pkt->packet_type);
        printf("Number of packet processed : %" PRIu64 "\n", pkt_process);

        ip = onvm_pkt_ipv4_hdr(pkt);
        if (ip != NULL) {
                onvm_pkt_print(pkt);
        } else {
                printf("Not IP4\n");
        }

        printf("\n\n");
}

static int
packet_handler(struct rte_mbuf *pkt, struct onvm_pkt_meta *meta,
               __attribute__((unused)) struct onvm_nf_local_ctx *nf_local_ctx) {
        static uint32_t counter = 0;
        struct onvm_dmt_nf_info *info = onvm_nflib_dmt_get_nf_info(nf_local_ctx);
        struct onvm_pkt_parse_ctx *parse_ctx = (struct onvm_pkt_parse_ctx *) rte_malloc(NULL, sizeof(struct onvm_pkt_parse_ctx), 0);


        if (counter++ == print_delay) {
                do_stats_display(pkt);
                counter = 0;
        }

        if (onvm_pkt_parse(pkt, parse_ctx)) {
                meta->action = ONVM_NF_ACTION_DROP;
                RTE_LOG(INFO, APP, "rss: %u: Unkown packet type(%hu), drop\n", pkt->hash.rss, parse_ctx->ether_type);
                goto end;
        }

        /*
         * NF Function
         * Ingress: filter non-tester packet
         */
        if (memcmp(&parse_ctx->eth->s_addr, &TARGET_SRC_MAC, RTE_ETHER_ADDR_LEN) != 0) {
                meta->action = ONVM_NF_ACTION_DROP;
                goto end;
        }

        onvm_nflib_dmt_record_match_data(parse_ctx, meta);
        onvm_nflib_dmt_synthesize_bitmap(info, meta);
        onvm_nflib_dmt_update_mpw_table(pkt, parse_ctx, meta, info->mpw_table, true);

        /*
         * NF Function
         * Ingress: set packet destination based on protocol
         */
        meta->action = dest_action;
        switch (parse_ctx->ether_type) {
                case RTE_ETHER_TYPE_IPV4:
                        meta->destination = destinations[DMT_IPV4_IDX];
                        break;
                case RTE_ETHER_TYPE_IPV6:
                        meta->destination = destinations[DMT_IPV6_IDX];
                        break;
                case DMT_ETHER_TYPE_ICN:
                        meta->destination = destinations[DMT_ICN_IDX];
                        break;
                case DMT_ETHER_TYPE_IPN:
                        meta->destination = destinations[DMT_IPN_IDX];
                        break;
                default:
                        meta->action = ONVM_NF_ACTION_DROP;
                        break;
        }

end:
        rte_free(parse_ctx);
        return 0;
}

int
main(int argc, char *argv[]) {
        int arg_offset;
        struct onvm_nf_local_ctx *nf_local_ctx;
        struct onvm_nf_function_table *nf_function_table;
        const char *progname = argv[0];

        nf_local_ctx = onvm_nflib_init_nf_local_ctx();
        onvm_nflib_start_signal_handler(nf_local_ctx, NULL);

        nf_function_table = onvm_nflib_init_nf_function_table();
        nf_function_table->pkt_handler = &packet_handler;
        nf_function_table->setup = &onvm_nflib_dmt_nf_setup;

        if ((arg_offset = onvm_nflib_init(argc, argv, NF_TAG, nf_local_ctx, nf_function_table)) < 0) {
                onvm_nflib_stop(nf_local_ctx);
                if (arg_offset == ONVM_SIGNAL_TERMINATION) {
                        printf("Exiting due to user termination\n");
                        return 0;
                } else {
                        rte_exit(EXIT_FAILURE, "Failed ONVM init\n");
                }
        }

        argc -= arg_offset;
        argv += arg_offset;

        if (parse_app_args(argc, argv, progname) < 0) {
                onvm_nflib_stop(nf_local_ctx);
                rte_exit(EXIT_FAILURE, "Invalid command-line arguments\n");
        }

        onvm_nflib_run(nf_local_ctx);

        onvm_nflib_dmt_nf_cleanup(nf_local_ctx);
        onvm_nflib_stop(nf_local_ctx);
        printf("If we reach here, program is ending\n");
        return 0;
}
