#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <errno.h>

#include "onvm_dmt_types.h"
#include "onvm_dmt_pkt_types.h"

#define PORT 1234

static void print_mac(const struct rte_ether_addr *addr) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           addr->addr_bytes[0], addr->addr_bytes[1],
           addr->addr_bytes[2], addr->addr_bytes[3],
           addr->addr_bytes[4], addr->addr_bytes[5]);
}

static void print_ipv4(uint32_t ip) {
    /* convert byte order */
    // struct in_addr addr;
    // addr.s_addr = ip;
    // printf("%s", inet_ntoa(addr));
    printf("%u.%u.%u.%u", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

static void print_icn_addr(icn_addr_t addr) {
    printf("%hhu", addr);
}

static void print_ipn_addr(ipn_addr_t addr) {
    printf("%u", addr);
}

static void print_mf_addr(mf_addr_t addr) {
    printf("%u", addr);
}

static void print_ndn_name(ndn_addr_t addr) {
    printf("%hu", addr);
}

static void print_geo_addr(geo_addr_t addr) {
    printf("%" PRIu64, addr);
}

static void print_ipv6(const uint8_t *ip) {
    char buf[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, ip, buf, sizeof(buf))) {
        printf("%s", buf);
    } else {
        printf("Invalid IPv6");
    }
}

static void print_cache_data(struct cache_data *data) {
    printf("--------------------------------------------------\n");
    printf("Received Cache Data:\n");
    printf("Action: %u\n", data->action);
    printf("State: %u\n", data->state);
    printf("Type: 0x%04X (%s)\n", data->type,
           data->type == CACHE_REQ_TYPE_IPV4 ? "IPv4" : 
           (data->type == CACHE_REQ_TYPE_IPV6 ? "IPv6" : 
           (data->type == CACHE_REQ_TYPE_ICN ? "ICN" :
           (data->type == CACHE_REQ_TYPE_IPN ? "IPN" :
           (data->type == CACHE_REQ_TYPE_GEO ? "GEO" :
           (data->type == CACHE_REQ_TYPE_MF ? "MF" :
           (data->type == CACHE_REQ_TYPE_NDN ? "NDN" : "Unknown")))))));
    printf("Match Field Bitmap: 0x%02X\n", data->match_field);
    printf("Rewrite Field Bitmap: 0x%02X\n", data->rewrite_field);

    printf("\nMatch Data:\n");
    if (data->type == CACHE_REQ_TYPE_IPV6) {
        printf("  Src IP: "); print_ipv6(data->match_data.inet6_saddr); printf("\n");
        printf("  Dst IP: "); print_ipv6(data->match_data.inet6_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_ICN) {
        printf("  Src ICN: "); print_icn_addr(data->match_data.icn_saddr); printf("\n");
        printf("  Dst ICN: "); print_icn_addr(data->match_data.icn_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_IPN) {
        printf("  Src IPN: "); print_ipn_addr(data->match_data.ipn_saddr); printf("\n");
        printf("  Dst IPN: "); print_ipn_addr(data->match_data.ipn_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_GEO) {
        printf("  SO GN Addr: "); print_geo_addr(data->match_data.geo_gn_saddr); printf("\n");
        printf("  DE GN Addr: "); print_geo_addr(data->match_data.geo_gn_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_MF) {
        printf("  Src NA: "); print_mf_addr(data->match_data.mf_na_saddr); printf("\n");
        printf("  Dest NA: "); print_mf_addr(data->match_data.mf_na_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_NDN) {
        printf("  Name: "); print_ndn_name(data->match_data.ndn_name_saddr); printf("\n");
    } else {
        printf("  Src IP: "); print_ipv4(data->match_data.inet4_saddr); printf("\n");
        printf("  Dst IP: "); print_ipv4(data->match_data.inet4_daddr); printf("\n");
    }
    printf("  Src Port: %u\n", data->match_data.inet_sport);
    printf("  Dst Port: %u\n", data->match_data.inet_dport);

    printf("\nRewrite Data:\n");
    if (data->type == CACHE_REQ_TYPE_IPV6) {
        printf("  Src IP: "); print_ipv6(data->rewrite_data.inet6_saddr); printf("\n");
        printf("  Dst IP: "); print_ipv6(data->rewrite_data.inet6_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_ICN) {
        printf("  Src ICN: "); print_icn_addr(data->rewrite_data.icn_saddr); printf("\n");
        printf("  Dst ICN: "); print_icn_addr(data->rewrite_data.icn_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_IPN) {
        printf("  Src IPN: "); print_ipn_addr(data->rewrite_data.ipn_saddr); printf("\n");
        printf("  Dst IPN: "); print_ipn_addr(data->rewrite_data.ipn_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_GEO) {
        printf("  SO GN Addr: "); print_geo_addr(data->rewrite_data.geo_gn_saddr); printf("\n");
        printf("  DE GN Addr: "); print_geo_addr(data->rewrite_data.geo_gn_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_MF) {
        printf("  Src NA: "); print_mf_addr(data->rewrite_data.mf_na_saddr); printf("\n");
        printf("  Dest NA: "); print_mf_addr(data->rewrite_data.mf_na_daddr); printf("\n");
    } else if (data->type == CACHE_REQ_TYPE_NDN) {
        printf("  Name: "); print_ndn_name(data->rewrite_data.ndn_name_saddr); printf("\n");
    } else {
        printf("  Src IP: "); print_ipv4(data->rewrite_data.inet4_saddr); printf("\n");
        printf("  Dst IP: "); print_ipv4(data->rewrite_data.inet4_daddr); printf("\n");
    }
    printf("  Src Port: %u\n", data->rewrite_data.inet_sport);
    printf("  Dst Port: %u\n", data->rewrite_data.inet_dport);
    printf("  Proto: %u\n", data->rewrite_data.proto);
    printf("  Out Port: %u\n", data->rewrite_data.out_port);
    printf("  Dec TTL: %u\n", data->rewrite_data.dec_ttl);
    printf("  Src MAC: "); print_mac(&data->rewrite_data.eth_saddr); printf("\n");
    printf("  Dst MAC: "); print_mac(&data->rewrite_data.eth_daddr); printf("\n");
    printf("--------------------------------------------------\n");
}

int main(void) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    ssize_t valread;
    struct cache_data data;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 1234
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("DMT Server listening on port %d...\n", PORT);

    while (1) {
        printf("Waiting for connection...\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        printf("Connection accepted from %s:%d\n", 
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        while (1) {
            size_t total_read = 0;
            char *ptr = (char *)&data;
            int disconnected = 0;

            while (total_read < sizeof(struct cache_data)) {
                valread = recv(new_socket, ptr + total_read, sizeof(struct cache_data) - total_read, 0);
                if (valread < 0) {
                    perror("recv");
                    disconnected = 1;
                    break;
                } else if (valread == 0) {
                    printf("Client disconnected\n");
                    disconnected = 1;
                    break;
                }
                total_read += valread;
            }

            if (disconnected) {
                close(new_socket);
                break;
            }

            print_cache_data(&data);
        }
    }

    return 0;
}
