#ifndef INCLUDE_PROTURA_NET_PACKET_H
#define INCLUDE_PROTURA_NET_PACKET_H

#include <protura/types.h>
#include <protura/list.h>
#include <protura/bits.h>
#include <protura/rwlock.h>
#include <protura/work.h>
#include <protura/string.h>
#include <protura/net/sockaddr.h>
#include <protura/net/ipv4/ipv4.h>
#include <protura/net/ipv4/tcp.h>

struct net_interface;

/*
enum packet_flags {
};
 */

struct packet {
    list_node_t packet_entry;
    flags_t flags;

    struct delay_work dwork;

    /* Data-link layer */
    struct net_interface *iface_tx;
    struct net_interface *iface_rx;

    /* address-resolution layer */
    n16 ll_type;
    char dest_mac[6];

    /* Address-family layer */
    in_addr_t route_addr;
    uint8_t protocol_type;

    /* Protocol layer */
    struct sockaddr src_addr;
    socklen_t src_len;

    struct sockaddr dest_addr;
    socklen_t dest_len;

    struct page *page;
    void *start, *head, *tail, *end;

    void *ll_head, *af_head, *proto_head;

    union {
        struct tcp_packet_cb tcp;
    } cb;

    /* If filled in, the socket that sent this packet */
    struct socket *sock;
};

#define PACKET_RESERVE_HEADER_SPACE 1024

#define PACKET_INIT(packet) \
    { \
        .packet_entry = LIST_NODE_INIT((packet).packet_entry), \
    }

static inline void packet_init(struct packet *packet)
{
    *packet = (struct packet)PACKET_INIT(*packet);
}

void net_packet_receive(struct packet *);
void net_packet_transmit(struct packet *);

struct packet *packet_new(int pal_flags);
void packet_free(struct packet *);
struct packet *packet_copy(struct packet *packet, int pal_flags);

void packet_add_header(struct packet *, const void *header, size_t header_len);
void packet_append_data(struct packet *, const void *data, size_t data_len);
int packet_append_user_data(struct packet *, struct user_buffer buf, size_t data_len);
void packet_pad_zero(struct packet *packet, size_t len);

static inline size_t packet_len(struct packet *packet)
{
    return packet->tail - packet->head;
}

static inline void packet_to_buffer(struct packet *packet, void *buf, size_t buf_len)
{
    size_t plen = packet_len(packet);
    size_t len = (buf_len < plen)? plen: buf_len;
    memcpy(buf, packet->head, len);
}

static inline struct page *packet_take_page(struct packet *packet)
{
    struct page *page = packet->page;
    packet->page = NULL;
    return page;
}

#endif
