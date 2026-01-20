#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <errno.h>

#include "../../onvm/onvm_nflib/onvm_dmt_types.h"

#define PORT 1234


void print_mac(const struct rte_ether_addr *addr);
void print_ip(uint32_t ip);
void print_cache_data(struct cache_data *data);

void print_mac(const struct rte_ether_addr *addr) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           addr->addr_bytes[0], addr->addr_bytes[1],
           addr->addr_bytes[2], addr->addr_bytes[3],
           addr->addr_bytes[4], addr->addr_bytes[5]);
}

void print_ip(uint32_t ip) {
    struct in_addr addr;
    addr.s_addr = ip;
    printf("%s", inet_ntoa(addr));
}

void print_cache_data(struct cache_data *data) {
    printf("--------------------------------------------------\n");
    printf("Received Cache Data:\n");
    printf("Action: %u\n", data->action);
    printf("State: %u\n", data->state);
    printf("Type: %u\n", data->type);
    printf("Match Field Bitmap: 0x%02X\n", data->match_field);
    printf("Rewrite Field Bitmap: 0x%02X\n", data->rewrite_field);

    printf("\nMatch Data:\n");
    printf("  Src IP: "); print_ip(data->match_data.inet4_saddr); printf("\n");
    printf("  Dst IP: "); print_ip(data->match_data.inet4_daddr); printf("\n");
    printf("  Src Port: %u\n", ntohs(data->match_data.inet_sport));
    printf("  Dst Port: %u\n", ntohs(data->match_data.inet_dport));

    printf("\nRewrite Data:\n");
    printf("  Src IP: "); print_ip(data->rewrite_data.inet4_saddr); printf("\n");
    printf("  Dst IP: "); print_ip(data->rewrite_data.inet4_daddr); printf("\n");
    printf("  Src Port: %u\n", ntohs(data->rewrite_data.inet_sport));
    printf("  Dst Port: %u\n", ntohs(data->rewrite_data.inet_dport));
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
