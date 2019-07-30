#ifndef SRC_NET_AF_IPV4_IPV4_H
#define SRC_NET_AF_IPV4_IPV4_H

#include <protura/compiler.h>
#include <protura/fs/procfs.h>

extern struct procfs_entry_ops ipv4_route_ops;

extern struct procfs_dir *ipv4_dir_procfs;

struct ip_header {
    uint8_t ihl :4;
    uint8_t version :4;

    uint8_t tos;

    uint16_t total_length;

    uint16_t id;

    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t csum;
    uint32_t source_ip;
    uint32_t dest_ip;
} __packed;

struct tcp_header {
    n16 source;
    n16 dest;
    n32 seq;
    n32 ack_seq;

    n16 res1 :4;
    n16 hl   :4;
    n16 fin  :1;
    n16 syn  :1;
    n16 rst  :1;
    n16 psh  :1;
    n16 ack  :1;
    n16 urg  :1;
    n16 res2 :2;
    n16 window;
    n16 check;
    n16 urg_ptr;
} __packed;

#endif
