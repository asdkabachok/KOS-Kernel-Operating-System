// tcp.c — Transmission Control Protocol FIXED
#include "net.h"

static struct socket* sockets = NULL;
static uint16_t next_port = 49152; // Ephemeral ports
static uint32_t tcp_seq_num = 0;

uint16_t tcp_checksum(struct ipv4_header* ip, struct tcp_header* tcp, void* data, uint16_t len) {
    struct {
        uint32_t src;
        uint32_t dst;
        uint8_t zero;
        uint8_t proto;
        uint16_t tcp_len;
    } pseudo_header;
    
    pseudo_header.src = ip->src_ip;
    pseudo_header.dst = ip->dst_ip;
    pseudo_header.zero = 0;
    pseudo_header.proto = IP_P_TCP;
    pseudo_header.tcp_len = htons(sizeof(struct tcp_header) + len);
    
    uint32_t sum = 0;
    uint16_t* ptr;
    
    // Pseudo header
    ptr = (uint16_t*)&pseudo_header;
    for (size_t i = 0; i < sizeof(pseudo_header) / 2; i++) {
        sum += *ptr++;
    }
    
    // TCP header
    ptr = (uint16_t*)tcp;
    for (size_t i = 0; i < sizeof(struct tcp_header) / 2; i++) {
        sum += *ptr++;
    }
    
    // Data
    ptr = (uint16_t*)data;
    for (size_t i = 0; i < len / 2; i++) {
        sum += *ptr++;
    }
    
    if (len & 1) {
        sum += ((uint8_t*)data)[len - 1];
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)~sum;
}

const char* tcp_state_name(uint8_t state) {
    switch (state) {
        case TCP_CLOSED:       return "CLOSED";
        case TCP_LISTEN:       return "LISTEN";
        case TCP_SYN_SENT:     return "SYN_SENT";
        case TCP_SYN_RECV:     return "SYN_RECV";
        case TCP_ESTABLISHED:  return "ESTABLISHED";
        case TCP_FIN_WAIT1:    return "FIN_WAIT1";
        case TCP_FIN_WAIT2:    return "FIN_WAIT2";
        case TCP_CLOSE_WAIT:   return "CLOSE_WAIT";
        case TCP_CLOSING:      return "CLOSING";
        case TCP_LAST_ACK:     return "LAST_ACK";
        case TCP_TIME_WAIT:    return "TIME_WAIT";
        default:               return "UNKNOWN";
    }
}

void tcp_init(void) {
    sockets = NULL;
    next_port = 49152;
    tcp_seq_num = 0x12345678; // Initial sequence number
}

struct socket* tcp_socket(void) {
    struct socket* sock = (struct socket*)kzalloc(sizeof(struct socket));
    if (!sock) return NULL;
    
    sock->type = SOCK_STREAM;
    sock->state = TCP_CLOSED;
    sock->window = TCP_RX_BUF_SIZE;
    sock->rx_buf = (uint8_t*)kmalloc(TCP_RX_BUF_SIZE);
    sock->tx_buf = (uint8_t*)kmalloc(TCP_TX_BUF_SIZE);
    sock->cwnd = TCP_MSS * 2;
    sock->ssthresh = 65535;
    sock->rto = 100;
    sock->srtt = 0;
    sock->rttvar = 0;
    
    // Add to list
    sock->next = sockets;
    sockets = sock;
    
    return sock;
}

int tcp_bind(struct socket* sock, struct sockaddr_in* addr) {
    if (!sock || sock->state != TCP_CLOSED) return -1;
    
    sock->local_addr = ntohl(addr->sin_addr);
    sock->local_port = ntohs(addr->sin_port);
    
    return 0;
}

int tcp_listen(struct socket* sock, int backlog) {
    (void)backlog;
    if (!sock || sock->state != TCP_CLOSED) return -1;
    if (sock->local_port == 0) return -1;
    
    sock->state = TCP_LISTEN;
    return 0;
}

int tcp_connect(struct socket* sock, struct sockaddr_in* addr) {
    if (!sock || sock->state != TCP_CLOSED) return -1;
    
    sock->remote_addr = ntohl(addr->sin_addr);
    sock->remote_port = ntohs(addr->sin_port);
    
    if (sock->local_port == 0) {
        sock->local_port = next_port++;
    }
    
    if (sock->local_addr == 0) {
        struct net_device* dev = net_get_device();
        if (dev) sock->local_addr = dev->ip;
    }
    
    // Send SYN
    sock->state = TCP_SYN_SENT;
    sock->snd_una = tcp_seq_num;
    sock->snd_nxt = tcp_seq_num + 1;
    
    tcp_tx(sock, TCP_SYN, NULL, 0);
    
    return 0;
}

void tcp_tx(struct socket* sock, uint8_t flags, void* data, uint16_t len) {
    if (!sock) return;
    
    struct ipv4_header ip;
    struct tcp_header tcp;
    
    ip.src_ip = htonl(sock->local_addr);
    ip.dst_ip = htonl(sock->remote_addr);
    
    tcp.src_port = htons(sock->local_port);
    tcp.dst_port = htons(sock->remote_port);
    tcp.seq = htonl(sock->snd_nxt);
    tcp.ack = htonl(sock->rcv_nxt);
    tcp.data_off = sizeof(struct tcp_header) / 4;
    tcp.flags = flags;
    tcp.window = htons(sock->window);
    tcp.urgent = 0;
    tcp.checksum = 0;
    
    // Calculate checksum
    tcp.checksum = tcp_checksum(&ip, &tcp, data, len);
    
    // Build packet and send
    uint16_t total = sizeof(struct tcp_header) + len;
    uint8_t* packet = (uint8_t*)kmalloc(total);
    if (!packet) return;
    
    memcpy(packet, &tcp, sizeof(struct tcp_header));
    if (len && data) {
        memcpy(packet + sizeof(struct tcp_header), data, len);
    }
    
    ip_send(sock->remote_addr, IP_P_TCP, packet, total);
    
    kfree(packet);
    
    if (flags & TCP_SYN) {
        if (!(flags & TCP_ACK)) {
            sock->snd_nxt++;
        }
    }
    if (flags & TCP_FIN) {
        sock->snd_nxt++;
    }
    if (len) {
        sock->snd_nxt += len;
    }
}

void tcp_rx(struct tcp_header* tcp, void* payload, uint16_t len) {
    uint16_t dport = ntohs(tcp->dst_port);
    uint16_t sport = ntohs(tcp->src_port);
    
    // Find socket
    struct socket* sock = sockets;
    while (sock) {
        if (sock->local_port == dport &&
            sock->remote_port == sport) {
            break;
        }
        if (sock->state == TCP_LISTEN && sock->local_port == dport) {
            break;
        }
        sock = sock->next;
    }
    
    if (!sock) {
        // No socket found, send RST
        return;
    }
    
    uint8_t flags = tcp->flags;
    uint32_t seq = ntohl(tcp->seq);
    uint32_t ack = ntohl(tcp->ack);
    
    switch (sock->state) {
        case TCP_LISTEN:
            if (flags & TCP_SYN) {
                // New connection
                struct socket* new_sock = tcp_socket();
                if (!new_sock) break;
                
                new_sock->state = TCP_SYN_RECV;
                new_sock->local_addr = sock->local_addr;
                new_sock->local_port = sock->local_port;
                // Note: remote_addr not available without IP header
                new_sock->remote_port = sport;
                new_sock->rcv_nxt = seq + 1;
                new_sock->snd_una = sock->snd_nxt;
                new_sock->snd_nxt = sock->snd_nxt;
                
                // Send SYN-ACK
                tcp_tx(new_sock, TCP_SYN | TCP_ACK, NULL, 0);
            }
            break;
            
        case TCP_SYN_SENT:
            if ((flags & TCP_SYN) && (flags & TCP_ACK)) {
                if (ack == sock->snd_una + 1) {
                    sock->state = TCP_ESTABLISHED;
                    sock->rcv_nxt = seq + 1;
                    sock->snd_una = ack;
                    tcp_tx(sock, TCP_ACK, NULL, 0);
                }
            }
            break;
            
        case TCP_SYN_RECV:
            if (flags & TCP_ACK) {
                sock->state = TCP_ESTABLISHED;
            }
            break;
            
        case TCP_ESTABLISHED:
            if (flags & TCP_ACK) {
                sock->snd_una = ack;
            }
            if (len > 0) {
                if (seq == sock->rcv_nxt) {
                    // In-order data
                    uint32_t space = TCP_RX_BUF_SIZE - (sock->rx_tail - sock->rx_head);
                    if (len > space) len = space;
                    
                    uint32_t tail = sock->rx_tail % TCP_RX_BUF_SIZE;
                    if (tail + len > TCP_RX_BUF_SIZE) {
                        uint32_t first = TCP_RX_BUF_SIZE - tail;
                        memcpy(sock->rx_buf + tail, payload, first);
                        memcpy(sock->rx_buf, (uint8_t*)payload + first, len - first);
                    } else {
                        memcpy(sock->rx_buf + tail, payload, len);
                    }
                    
                    sock->rx_tail += len;
                    sock->rcv_nxt += len;
                    
                    tcp_tx(sock, TCP_ACK, NULL, 0);
                }
            }
            
            if (flags & TCP_FIN) {
                sock->state = TCP_CLOSE_WAIT;
                sock->rcv_nxt++;
                tcp_tx(sock, TCP_ACK, NULL, 0);
            }
            break;
            
        case TCP_CLOSE_WAIT:
        case TCP_LAST_ACK:
            if (flags & TCP_ACK) {
                sock->state = TCP_CLOSED;
            }
            break;
            
        default:
            break;
    }
}

int tcp_send(struct socket* sock, void* data, uint16_t len) {
    if (!sock || sock->state != TCP_ESTABLISHED) return -1;
    if (!data || len == 0) return 0;
    
    tcp_tx(sock, TCP_ACK | TCP_PSH, data, len);
    return len;
}

int tcp_recv(struct socket* sock, void* buf, uint16_t len) {
    if (!sock) return -1;
    if (sock->state != TCP_ESTABLISHED && sock->state != TCP_CLOSE_WAIT) return -1;
    
    uint32_t available = sock->rx_tail - sock->rx_head;
    if (available == 0) return 0;
    
    if (len > available) len = available;
    
    uint32_t head = sock->rx_head % TCP_RX_BUF_SIZE;
    if (head + len > TCP_RX_BUF_SIZE) {
        uint32_t first = TCP_RX_BUF_SIZE - head;
        memcpy(buf, sock->rx_buf + head, first);
        memcpy((uint8_t*)buf + first, sock->rx_buf, len - first);
    } else {
        memcpy(buf, sock->rx_buf + head, len);
    }
    
    sock->rx_head += len;
    return len;
}

void tcp_close(struct socket* sock) {
    if (!sock) return;
    
    switch (sock->state) {
        case TCP_ESTABLISHED:
            sock->state = TCP_FIN_WAIT1;
            tcp_tx(sock, TCP_FIN | TCP_ACK, NULL, 0);
            break;
        case TCP_CLOSE_WAIT:
            sock->state = TCP_LAST_ACK;
            tcp_tx(sock, TCP_FIN | TCP_ACK, NULL, 0);
            break;
        case TCP_SYN_SENT:
        case TCP_SYN_RECV:
        case TCP_CLOSED:
            sock->state = TCP_CLOSED;
            break;
        default:
            break;
    }
}

void tcp_timer_tick(void) {
    struct socket* sock = sockets;
    while (sock) {
        if (sock->state == TCP_TIME_WAIT) {
            if (sock->time_wait_timer > 0) {
                sock->time_wait_timer--;
            } else {
                sock->state = TCP_CLOSED;
            }
        }
        sock = sock->next;
    }
}

void tcp_cleanup(void) {
    struct socket** prev = &sockets;
    struct socket* sock = sockets;
    
    while (sock) {
        if (sock->state == TCP_CLOSED) {
            *prev = sock->next;
            if (sock->rx_buf) kfree(sock->rx_buf);
            if (sock->tx_buf) kfree(sock->tx_buf);
            kfree(sock);
            sock = *prev;
        } else {
            prev = &sock->next;
            sock = sock->next;
        }
    }
}
