#include "include/arpcache.h"
#include "include/arp.h"
#include "include/ether.h"
#include "include/packet.h"
#include "include/icmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static arpcache_t arpcache;

// initialize IP->mac mapping, request list, lock and sweeping thread
void arpcache_init()
{
	bzero(&arpcache, sizeof(arpcache_t));

	init_list_head(&(arpcache.req_list));

	pthread_mutex_init(&arpcache.lock, NULL);

	pthread_create(&arpcache.thread, NULL, arpcache_sweep, NULL);
}

// release all the resources when exiting
void arpcache_destroy()
{
	pthread_mutex_lock(&arpcache.lock);

	struct arp_req *req_entry = NULL, *req_q;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
		struct cached_pkt *pkt_entry = NULL, *pkt_q;
		list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
			list_delete_entry(&(pkt_entry->list));
			free(pkt_entry->packet);
			free(pkt_entry);
		}

		list_delete_entry(&(req_entry->list));
		free(req_entry);
	}

	pthread_kill(arpcache.thread, SIGTERM);

	pthread_mutex_unlock(&arpcache.lock);
}

// lookup the IP->mac mapping
//
// traverse the hash table to find whether there is an entry with the same IP
// and mac address with the given arguments
int arpcache_lookup(u32 ip4, u8 mac[ETH_ALEN])
{
	/*fprintf(stderr, "TODO: lookup ip address in arp cache.\n");*/
	pthread_mutex_lock(&arpcache.lock);
	for ( int i = 0; i != MAX_ARP_SIZE; ++i ){
		if (arpcache.entries[i].ip4 == ip4 && arpcache.entries[i].valid){
			memcpy(mac, arpcache.entries[i].mac, ETH_ALEN);
			pthread_mutex_unlock(&arpcache.lock);
			return 1;
		}
	}
	pthread_mutex_unlock(&arpcache.lock);
	return 0;
}

// append the packet to arpcache
//
// Lookup in the hash table which stores pending packets, if there is already an
// entry with the same IP address and iface (which means the corresponding arp
// request has been sent out), just append this packet at the tail of that entry
// (the entry may contain more than one packet); otherwise, malloc a new entry
// with the given IP address and iface, append the packet, and send arp request.
void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len)
{
	/*fprintf(stderr, "TODO: append the ip address if lookup failed, and send arp request if necessary.\n");*/
	/*printf ("\n In append packet\n");*/
	pthread_mutex_lock(&arpcache.lock);
	struct cached_pkt *new_pkt = (struct cached_pkt *)malloc(sizeof(struct cached_pkt));
	init_list_head(&new_pkt->list);
	if ( !new_pkt ){
		printf ("Allocate memory(new_pkt) failed.\n");
		pthread_mutex_unlock(&arpcache.lock);
		exit(0);
	}
	new_pkt->len = len;
	new_pkt->packet = packet;
	struct arp_req *pos, *q;
	/*printf ("after malloc new_pkt\n");*/
	list_for_each_entry_safe(pos, q, &arpcache.req_list, list){
		if(pos->iface == iface && pos->ip4 == ip4){
			list_add_tail(&new_pkt->list, &(pos->cached_packets));
			pthread_mutex_unlock(&arpcache.lock);
			return;
		}
	}
	/*printf (" before malloc new_rep\n");*/
	struct arp_req *new_rep = (struct arp_req *)malloc(sizeof(struct arp_req));
	init_list_head(&new_rep->list);
	init_list_head(&new_rep->cached_packets);
	if (!new_rep){
		printf("Allocate memory(new_rep) falied.\n");
		pthread_mutex_unlock(&arpcache.lock);
		exit(0);
	}
	/*printf(" after malloc new_rep\n");*/
	/*init_list_head(&(new_pkt->list));*/
	new_rep->iface = iface;
	new_rep->ip4 = ip4;
	new_rep->sent = time(NULL);
	new_rep->retries = 0;
	/*printf(" Before add to list\n");*/
	list_add_head(&new_pkt->list, &new_rep->cached_packets);
	/*printf(" Before add req list\n");*/
	list_add_tail(&(new_rep->list), &(arpcache.req_list));
	/*printf (" before send arp request\n");*/
	arp_send_request(iface, ip4);
	pthread_mutex_unlock(&arpcache.lock);
}

// insert the IP->mac mapping into arpcache, if there are pending packets
// waiting for this mapping, fill the ethernet header for each of them, and send
// them out
void arpcache_insert(u32 ip4, u8 mac[ETH_ALEN])
{
	/*fprintf(stderr, "TODO: insert ip->mac entry, and send all the pending packets.\n");*/
	pthread_mutex_lock(&arpcache.lock);
	int i;
	for (i = 0; i != MAX_ARP_SIZE; ++i){
		if (arpcache.entries[i].valid == 0){
			break;
		}
		if (arpcache.entries[i].ip4 == ip4 && arpcache.entries[i].valid == 1){
			arpcache.entries[i].added = time(NULL);
			pthread_mutex_unlock(&arpcache.lock);
			return ;
		}
	}
	if (i < MAX_ARP_SIZE){
		arpcache.entries[i].ip4 = ip4;
		memcpy(arpcache.entries[i].mac, mac, ETH_ALEN);
		arpcache.entries[i].added = time(NULL);
		arpcache.entries[i].valid = 1;
	}
	else {// replace the map in the first location
		arpcache.entries[0].ip4 = ip4;
		memcpy(arpcache.entries[0].mac, mac, ETH_ALEN);
		arpcache.entries[0].added = time(NULL);
		arpcache.entries[0].valid = 1;
	}
	struct arp_req *req_pos, *req_q;
	struct cached_pkt *pkt_pos, *pkt_q;
	char *packet;
	list_for_each_entry_safe(req_pos, req_q, &arpcache.req_list, list){
		if (req_pos->ip4 == ip4){
			list_for_each_entry_safe(pkt_pos, pkt_q, &req_pos->cached_packets, list){
				packet = pkt_pos->packet;
				struct ether_header *eth = (struct ether_header *)(packet);
				memcpy(eth->ether_dhost, mac, ETH_ALEN);
				iface_send_packet(req_pos->iface, packet, pkt_pos->len);
			}
			list_for_each_entry_safe(pkt_pos, pkt_q, &req_pos->cached_packets, list){
				list_delete_entry(&(pkt_pos->list));
				free(pkt_pos);
			}
			list_delete_entry(&req_pos->list);
			free(req_pos);
		}//if
	}
	pthread_mutex_unlock(&arpcache.lock);
}

// sweep arpcache periodically
//
// For the IP->mac entry, if the entry has been in the table for more than 15
// seconds, remove it from the table.
// For the pending packets, if the arp request is sent out 1 second ago, while 
// the reply has not been received, retransmit the arp request. If the arp
// request has been sent 5 times without receiving arp reply, for each
// pending packet, send icmp packet (DEST_HOST_UNREACHABLE), and drop these
// packets.
void *arpcache_sweep(void *arg) 
{
	while (1) {
		sleep(1);
		/*fprintf(stderr, "TODO: sweep arpcache periodically: remove old entries, resend arp requests .\n");*/
		pthread_mutex_lock(&arpcache.lock);
		time_t now = time(NULL);
		for ( int i = 0; i != MAX_ARP_SIZE; ++i ){
			if ((now - arpcache.entries[i].added) > 15){
				arpcache.entries[i].valid = 0;
			}
		}
		struct arp_req *req_pos, *req_q;
		now = time(NULL);
		list_for_each_entry_safe(req_pos, req_q, &arpcache.req_list, list){
			if (req_pos->retries > 5){
				struct cached_pkt *pkt_pos, *pkt_q;
				list_for_each_entry(pkt_pos, &req_pos->cached_packets, list){
					icmp_send_packet(pkt_pos->packet, pkt_pos->len, 3, 1);
				}
				list_for_each_entry_safe(pkt_pos, pkt_q, &req_pos->cached_packets, list){
					list_delete_entry(&pkt_pos->list);
					free(pkt_pos);
				}
				list_delete_entry(&req_pos->list);
				free(req_pos);
			}//if
			if ((now - req_pos->sent) > 1){
				arp_send_request(req_pos->iface, req_pos->ip4);
				++req_pos->retries;
			}
		}

		pthread_mutex_unlock(&arpcache.lock);
	}

	return NULL;
}
