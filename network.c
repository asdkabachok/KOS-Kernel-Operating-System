// network.c — Network stack initialization FIXED
#include "net.h"

static struct net_device* devices = NULL;
static uint8_t arp_cache[256][6]; // Simple ARP cache
static uint32_t arp_cache_ip[256];
static int arp_cache_count = 0;

void network_init(void) {
    tcp_init();
    arp_init();
    kprintf("Network: Initialized\n");
}

void network_poll(void) {
    // Poll network devices for received packets
    // In real implementation, this would be called from interrupt or timer
}

void network_rx(void* packet, uint16_t len) {
    if (len < sizeof(struct eth_header)) return;
    
    struct eth_header* eth = (struct eth_header*)packet;
    uint16_t type = ntohs(eth->type);
    
    switch (type) {
        case ETH_P_IP:
            ip_rx((struct ip_header*)((uint8_t*)packet + sizeof(struct eth_header)),
                  (void*)(len - sizeof(struct eth_header)), len - sizeof(struct eth_header));
            break;
        case ETH_P_ARP:
            arp_rx((struct arp_packet*)((uint8_t*)packet + sizeof(struct eth_header)));
            break;
    }
}

void arp_init(void) {
    memset(arp_cache_ip, 0, sizeof(arp_cache_ip));
    memset(arp_cache, 0, sizeof(arp_cache));
    arp_cache_count = 0;
    kprintf("ARP: Initialized\n");
}

void arp_request(uint32_t ip) {
    // Send ARP request
    // Simplified - in real implementation would queue packet and send
    (void)ip;
}

void arp_rx(struct arp_packet* arp) {
    if (!arp) return;
    
    if (ntohs(arp->opcode) == ARP_OP_REQUEST) { // ARP Request
        // Check if request is for our IP
        uint32_t target_ip = ntohl(arp->target_ip);
        struct net_device* dev = net_get_device();
        if (dev && target_ip == dev->ip) {
            // Send ARP reply
            // Simplified
        }
    } else if (ntohs(arp->opcode) == ARP_OP_REPLY) { // ARP Reply
        // Add to cache
        arp_add(ntohl(arp->sender_ip), arp->sender_mac);
    }
}

uint8_t* arp_lookup(uint32_t ip) {
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache_ip[i] == ip) {
            return arp_cache[i];
        }
    }
    return NULL;
}

void arp_add(uint32_t ip, uint8_t* mac) {
    // Check if already exists
    for (int i = 0; i < arp_cache_count; i++) {
        if (arp_cache_ip[i] == ip) {
            memcpy(arp_cache[i], mac, ETH_ALEN);
            return;
        }
    }
    
    // Add new entry
    if (arp_cache_count < 256) {
        arp_cache_ip[arp_cache_count] = ip;
        memcpy(arp_cache[arp_cache_count], mac, ETH_ALEN);
        arp_cache_count++;
    }
}

void net_register_device(struct net_device* dev) {
    dev->next = devices;
    devices = dev;
    kprintf("Network: Registered %02X:%02X:%02X:%02X:%02X:%02X\n",
        dev->mac[0], dev->mac[1], dev->mac[2],
        dev->mac[3], dev->mac[4], dev->mac[5]);
}

struct net_device* net_get_device(void) {
    return devices;
}

// ICMP implementation
void icmp_rx(struct ip_header* ip, struct icmp_header* icmp, void* data, uint16_t len) {
    if (icmp->type == ICMP_ECHO) {
        // Echo request - send reply
        icmp_echo_reply(ntohl(ip->saddr), ntohs(icmp->id), ntohs(icmp->sequence), data, len);
    }
}

void icmp_echo_reply(uint32_t dst, uint16_t id, uint16_t seq, void* data, uint16_t len) {
    struct icmp_header icmp;
    icmp.type = ICMP_ECHOREPLY;
    icmp.code = 0;
    icmp.checksum = 0;
    icmp.id = htons(id);
    icmp.sequence = htons(seq);
    
    uint16_t total = sizeof(struct icmp_header) + len;
    uint8_t* packet = (uint8_t*)kmalloc(total);
    
    memcpy(packet, &icmp, sizeof(struct icmp_header));
    if (len && data) {
        memcpy(packet + sizeof(struct icmp_header), data, len);
    }
    
    // Calculate checksum
    uint16_t* p = (uint16_t*)packet;
    uint32_t sum = 0;
    for (uint16_t i = 0; i < total / 2; i++) {
        sum += *p++;
    }
    if (total & 1) sum += *(uint8_t*)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    ((struct icmp_header*)packet)->checksum = ~sum;
    
    ip_send(dst, IP_P_ICMP, packet, total);
    kfree(packet);
}

// UDP implementation  
void udp_rx(struct udp_header* udp, void* payload, uint16_t len) {
    (void)udp;
    (void)payload;
    (void)len;
}

void udp_tx(uint32_t dst, uint16_t dport, uint16_t sport, void* data, uint16_t len) {
    struct udp_header udp;
    udp.src_port = htons(sport);
    udp.dst_port = htons(dport);
    udp.length = htons(sizeof(struct udp_header) + len);
    udp.checksum = 0;
    
    uint16_t total = sizeof(struct udp_header) + len;
    uint8_t* packet = (uint8_t*)kmalloc(total);
    
    memcpy(packet, &udp, sizeof(struct udp_header));
    if (len && data) {
        memcpy(packet + sizeof(struct udp_header), data, len);
    }
    
    ip_send(dst, IP_P_UDP, packet, total);
    kfree(packet);
}
