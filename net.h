// net.h — TCP/IP stack definitions FIXED
#ifndef NET_H
#define NET_H

#include "kernel.h"

// Ethernet
#define ETH_ALEN        6
#define ETH_P_IP        0x0800
#define ETH_P_ARP       0x0806

struct eth_header {
    uint8_t dst[ETH_ALEN];
    uint8_t src[ETH_ALEN];
    uint16_t type;
} __attribute__((packed));

// IP
#define IP_P_ICMP       1
#define IP_P_TCP        6
#define IP_P_UDP        17

struct ip_header {
    uint8_t ihl_version;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
} __attribute__((packed));

// TCP
#define TCP_FIN         0x01
#define TCP_SYN         0x02
#define TCP_RST         0x04
#define TCP_PSH         0x08
#define TCP_ACK         0x10
#define TCP_URG         0x20

// Note: tcp_header defined in kernel.h

// UDP
// Note: udp_header defined in kernel.h

// ICMP
#define ICMP_ECHOREPLY  0
#define ICMP_ECHO       8

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

// ARP
struct arp_header {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[ETH_ALEN];
    uint32_t spa;
    uint8_t tha[ETH_ALEN];
    uint32_t tpa;
} __attribute__((packed));

// Socket
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define AF_INET         2
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    uint32_t sin_addr;
    uint8_t sin_zero[8];
};

struct socket {
    int type;
    uint32_t local_addr;
    uint16_t local_port;
    uint32_t remote_addr;
    uint16_t remote_port;
    uint8_t state;
    uint32_t snd_una;
    uint32_t snd_nxt;
    uint32_t rcv_nxt;
    uint16_t window;
    uint8_t* rx_buf;
    uint8_t* tx_buf;
    uint32_t rx_head;
    uint32_t rx_tail;
    uint32_t tx_head;
    uint32_t tx_tail;
    struct socket* next;
    int error;
    uint32_t unacked_len;
    uint32_t last_ack;
    uint8_t dup_acks;
    uint32_t cwnd;
    uint32_t ssthresh;
    uint32_t srtt;
    uint32_t rttvar;
    uint32_t rto;
    uint8_t time_wait_timer;
    struct socket* ofo_queue;
};

// TCP states
#define TCP_CLOSED      0
#define TCP_LISTEN      1
#define TCP_SYN_SENT    2
#define TCP_SYN_RECV    3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT1   5
#define TCP_FIN_WAIT2   6
#define TCP_CLOSE_WAIT  7
#define TCP_CLOSING     8
#define TCP_LAST_ACK    9
#define TCP_TIME_WAIT   10

#define TCP_RX_BUF_SIZE     65536
#define TCP_TX_BUF_SIZE     65536
#define TCP_MSS             1460
#define TCP_MAX_RETRIES     3
#define TCP_RTO_BASE        100

// Network device
struct net_device {
    uint8_t mac[6];
    uint32_t ip;
    uint32_t gateway;
    uint32_t subnet;
    bool (*tx)(void* data, uint16_t len);
    void (*rx)(void* data, uint16_t len);
    struct net_device* next;
};

// Network functions
void network_init(void);
void network_poll(void);
void network_rx(void* packet, uint16_t len);

// IP
void ip_send(uint32_t dst, uint8_t proto, void* data, uint16_t len);
void ip_rx(struct ip_header* ip, void* data, uint16_t len);
uint16_t ip_checksum(void* data, uint32_t len);

// TCP
void tcp_init(void);
void tcp_rx(struct tcp_header* tcp, void* payload, uint16_t len);
void tcp_tx(struct socket* sock, uint8_t flags, void* data, uint16_t len);
struct socket* tcp_socket(void);
int tcp_bind(struct socket* sock, struct sockaddr_in* addr);
int tcp_listen(struct socket* sock, int backlog);
struct socket* tcp_accept(struct socket* sock);
int tcp_connect(struct socket* sock, struct sockaddr_in* addr);
int tcp_send(struct socket* sock, void* data, uint16_t len);
int tcp_recv(struct socket* sock, void* buf, uint16_t len);
void tcp_close(struct socket* sock);
void tcp_timer_tick(void);
void tcp_cleanup(void);
const char* tcp_state_name(uint8_t state);

// UDP
void udp_rx(struct udp_header* udp, void* payload, uint16_t len);
void udp_tx(uint32_t dst, uint16_t dport, uint16_t sport, void* data, uint16_t len);

// ICMP
void icmp_rx(struct ip_header* ip, struct icmp_header* icmp, void* data, uint16_t len);
void icmp_echo_reply(uint32_t dst, uint16_t id, uint16_t seq, void* data, uint16_t len);

// ARP
void arp_init(void);
void arp_request(uint32_t ip);
void arp_rx(struct arp_packet* arp);
uint8_t* arp_lookup(uint32_t ip);
void arp_add(uint32_t ip, uint8_t* mac);

// Network device management
void net_register_device(struct net_device* dev);
struct net_device* net_get_device(void);

// Utility
uint16_t checksum(void* data, size_t len);
uint16_t tcp_checksum(struct ipv4_header* ip, struct tcp_header* tcp, void* payload, uint16_t len);
uint16_t udp_checksum(struct ipv4_header* ip, struct udp_header* udp, void* payload, uint16_t len);

#endif
