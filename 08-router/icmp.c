#include "include/icmp.h"
#include "include/ip.h"
#include "include/rtable.h"
#include "include/arp.h"
#include "include/base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// send icmp packet
void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
	/*fprintf(stderr, "TODO: malloc and send icmp packet.\n");*/
	struct ether_header *in_eth_h = (struct ether_header*)(in_pkt);
	struct iphdr *ip = packet_to_ip_hdr(in_pkt);
	int packet_len;
	char *packet;
	
	if (type == 8){
		packet_len = len;
	}
	else {
		packet_len = ETHER_HDR_SIZE+IP_BASE_HDR_SIZE+ICMP_HDR_SIZE+IP_HDR_SIZE(ip) + 8;
	}

	packet = (char *)malloc(packet_len);
	struct ether_header *eth_h = (struct ether_header *)(packet);
	struct iphdr *p_ip = (struct iphdr *)(packet + ETHER_HDR_SIZE);
	struct icmphdr *icmp = (struct icmphdr *)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);

	memcpy(eth_h->ether_dhost, in_eth_h->ether_dhost, ETH_ALEN);
	memcpy(eth_h->ether_shost, in_eth_h->ether_dhost, ETH_ALEN);
	eth_h->ether_type = htons(ETH_P_IP);

	rt_entry_t *entry = longest_prefix_match(ntohl(ip->saddr));
	ip_init_hdr(p_ip, entry->iface->ip, ntohl(ip->saddr), packet_len-ETHER_HDR_SIZE, 1);

	if (type == 8){
		char *in_pkt_rest = (char *)(in_pkt + ETHER_HDR_SIZE + IP_HDR_SIZE(ip) + ICMP_HDR_SIZE - 4);
		char *packet_rest = packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + ICMP_HDR_SIZE - 4;
		icmp->type = 0;
		icmp->code = 0;
		int data_size = len - ETHER_HDR_SIZE - IP_HDR_SIZE(ip) - ICMP_HDR_SIZE + 4;
		memcpy(packet_rest, in_pkt_rest, data_size);
		icmp->checksum = icmp_checksum(icmp, data_size + ICMP_HDR_SIZE - 4);
	}
	else {
		char *packet_rest = packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + ICMP_HDR_SIZE;
		icmp->type = type;
		icmp->code = code;
		int data_size = IP_HDR_SIZE(ip) + 8;
		memset(packet_rest - 4, 0, 4);
		memcpy(packet_rest, ip, data_size);
		icmp->checksum = icmp_checksum(icmp, data_size+ICMP_HDR_SIZE);
	}
	ip_send_packet(packet, packet_len);
}
