#include "include/arp.h"
#include "include/base.h"
#include "include/types.h"
#include "include/packet.h"
#include "include/ether.h"
#include "include/arpcache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "log.h"

// send an arp request: encapsulate an arp request packet, send it out through
// iface_send_packet
void arp_send_request(iface_info_t *iface, u32 dst_ip)
{
	/*fprintf(stderr, "TODO: send arp request when lookup failed in arpcache.\n");*/
	/*printf ("in arp arp_send_request\n");*/
	char *packet = (char *) malloc (sizeof(struct ether_arp)+sizeof(struct ether_header));
	struct ether_arp *eth_arp = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
	struct ether_header *eth_h = (struct ether_header *)(packet);
	memcpy(eth_h->ether_shost, iface->mac, ETH_ALEN);
	for (int i = 0; i != ETH_ALEN; ++i){
		eth_h->ether_dhost[i] = 0xff;
	}
	eth_arp->arp_hln = 6;
	eth_arp->arp_pln = 4;
	eth_arp->arp_hrd = htons(ARPHRD_ETHER);
	eth_arp->arp_pro = htons(ETH_P_IP);
	eth_h->ether_type = htons(ETH_P_ARP);
	eth_arp->arp_op = htons(0x0001);
	eth_arp->arp_spa = htonl(iface->ip);
	memcpy(eth_arp->arp_sha, iface->mac, ETH_ALEN);
	eth_arp->arp_tpa = htonl(dst_ip);
	/*printf("before iface_send_packet(in arp_send_request)\n");*/
	iface_send_packet(iface, packet, sizeof(struct ether_arp) + sizeof(struct ether_header));
}

// send an arp reply packet: encapsulate an arp reply packet, send it out
// through iface_send_packet
void arp_send_reply(iface_info_t *iface, struct ether_arp *req_hdr)
{
	/*fprintf(stderr, "TODO: send arp reply when receiving arp request.\n");*/
	char *packet = (char *) malloc(sizeof(struct ether_arp) + sizeof(struct ether_header));
	struct ether_arp *eth_arp = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
	memcpy(eth_arp , req_hdr, sizeof(struct ether_arp));
	memcpy(eth_arp->arp_sha, iface->mac, ETH_ALEN);
	eth_arp->arp_spa = htonl(iface->ip);
	memcpy(eth_arp->arp_tha, req_hdr->arp_sha, ETH_ALEN);
	eth_arp->arp_tpa = req_hdr->arp_spa;
	eth_arp->arp_op = htons(0x0002);

	struct ether_header *eth_h = (struct ether_header *) (packet);
	eth_h->ether_type = htons(ETH_P_ARP);
	memcpy(eth_h->ether_dhost, req_hdr->arp_sha, ETH_ALEN);
	memcpy(eth_h->ether_shost, iface->mac, ETH_ALEN);
	iface_send_packet(iface, packet, sizeof(struct ether_arp) + sizeof(struct ether_header));
}

void handle_arp_packet(iface_info_t *iface, char *packet, int len)
{
	/*fprintf(stderr, "TODO: process arp packet: arp request & arp reply.\n");*/
	struct ether_arp *eth_arp = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
	if (ntohs(eth_arp->arp_op) == 0x0001){
		if (ntohl(eth_arp->arp_tpa) == iface->ip){
			arpcache_insert(ntohl(eth_arp->arp_spa), eth_arp->arp_sha);
			arp_send_reply(iface, eth_arp);
		}
		else {
			iface_send_packet(iface, packet, len);
		}
	}
	if (ntohs(eth_arp->arp_op) == 0x0002){
		/*if(iface->ip == eth_arp->arp_tpa){*/
		arpcache_insert(ntohl(eth_arp->arp_spa), eth_arp->arp_sha);
		/*}*/
		/*else {*/
		/*iface_send_packet_by_arp(iface, ntohl(eth_arp->arp_tpa), packet, len);*/
		/*}*/
	}

}

// send (IP) packet through arpcache lookup 
//
// Lookup the mac address of dst_ip in arpcache. If it is found, fill the
// ethernet header and emit the packet by iface_send_packet, otherwise, pending 
// this packet into arpcache, and send arp request.
void iface_send_packet_by_arp(iface_info_t *iface, u32 dst_ip, char *packet, int len)
{
	/*printf ("In sent packet by arp\n");*/
	struct ether_header *eh = (struct ether_header *)packet;
	memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
	eh->ether_type = htons(ETH_P_IP);

	u8 dst_mac[ETH_ALEN];
	int found = arpcache_lookup(dst_ip, dst_mac);
	if (found) {
		/*log(DEBUG, "found the mac of %x, send this packet", dst_ip);*/
		/*printf ("\n In sent packet\n");*/
		memcpy(eh->ether_dhost, dst_mac, ETH_ALEN);
		iface_send_packet(iface, packet, len);
	}
	else {
		// log(DEBUG, "lookup %x failed, pend this packet", dst_ip);
		/*printf ("\nIn arp append packet\n");*/
		arpcache_append_packet(iface, dst_ip, packet, len);
	}
}
