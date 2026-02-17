// ip.c — Internet Protocol v4 FIXED
#include "net.h"

static uint32_t local_ip = 0xC0A80001;  // 192.168.0.1 default
static uint32_t gateway_ip = 0xC0A80000; // 192.168.0.0
static uint32_t subnet_mask = 0xFFFFFF00; // /24
static uint16_t ip_id = 0;

extern struct net_device* net_get_device(void);

uint16_t ip_checksum(void* data, uint32_t len) {
    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)data;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len) {
        sum += *(uint8_t*)ptr;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

void ip_send(uint32_t dst, uint8_t proto, void* data, uint16_t len) {
    uint16_t total_len = sizeof(struct ip_header) + len;
    
    struct ip_header* ip = (struct ip_header*)kmalloc(sizeof(struct ip_header) + len);
    if (!ip) return;
    
    ip->ihl_version = 0x45; // IPv4, 5 dwords header
    ip->tos = 0;
    ip->tot_len = htons(total_len);
    ip->id = htons(ip_id++);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = proto;
    ip->check = 0;
    ip->saddr = htonl(local_ip);
    ip->daddr = htonl(dst);
    
    memcpy((uint8_t*)ip + sizeof(struct ip_header), data, len);
    ip->check = ip_checksum(ip, sizeof(struct ip_header));
    
    // Check if destination is on local network
    if ((dst & subnet_mask) != (local_ip & subnet_mask)) {
        dst = gateway_ip;
    }
    
    // Get MAC address via ARP
    uint8_t* dst_mac = arp_lookup(dst);
    if (!dst_mac) {
        arp_request(dst);
        kfree(ip);
        return;
    }
    
    // Build ethernet frame and send
    struct eth_header* eth = (struct eth_header*)kmalloc(sizeof(struct eth_header) + total_len);
    if (!eth) {
        kfree(ip);
        return;
    }
    
    memcpy(eth->dst, dst_mac, ETH_ALEN);
    struct net_device* dev = net_get_device();
    if (dev) {
        memcpy(eth->src, dev->mac, ETH_ALEN);
    } else {
        memset(eth->src, 0, ETH_ALEN);
    }
    eth->type = htons(ETH_P_IP);
    
    memcpy((uint8_t*)eth + sizeof(struct eth_header), ip, total_len);
    
    if (dev && dev->tx) {
        dev->tx(eth, sizeof(struct eth_header) + total_len);
    }
    
    kfree(eth);
    kfree(ip);
}

void ip_rx(struct ip_header* ip, void* data, uint16_t len) {
    // Verify checksum
    uint16_t check = ip->check;
    ip->check = 0;
    if (ip_checksum(ip, sizeof(struct ip_header)) != check) {
        return; // Bad checksum
    }
    
    // Verify header length
    uint8_t ihl = (ip->ihl_version & 0x0F) * 4;
    if (ihl < 20) return;
    
    // Verify destination
    uint32_t dst = ntohl(ip->daddr);
    if (dst != local_ip && dst != 0xFFFFFFFF) {
        return; // Not for us
    }
    
    uint16_t data_len = ntohs(ip->tot_len) - ihl;
    void* payload = (uint8_t*)ip + ihl;
    
    switch (ip->protocol) {
        case IP_P_ICMP:
            icmp_rx(ip, (struct icmp_header*)payload, (uint8_t*)payload + sizeof(struct icmp_header), data_len - sizeof(struct icmp_header));
            break;
        case IP_P_TCP:
            tcp_rx((struct tcp_header*)payload, (uint8_t*)payload + sizeof(struct tcp_header), data_len - sizeof(struct tcp_header));
            break;
        case IP_P_UDP:
            udp_rx((struct udp_header*)payload, (uint8_t*)payload + sizeof(struct udp_header), data_len - sizeof(struct udp_header));
            break;
    }
}
