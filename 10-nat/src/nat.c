#include "include/nat.h"
#include "include/ip.h"
#include "include/icmp.h"
#include "include/tcp.h"
#include "include/rtable.h"
#include "include/log.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static struct nat_table nat;

// get the interface from iface name
static iface_info_t *if_name_to_iface(const char *if_name)
{
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		if (strcmp(iface->name, if_name) == 0)
		  return iface;
	}

	log(ERROR, "Could not find the desired interface according to if_name '%s'", if_name);
	return NULL;
}

// determine the direction of the packet, DIR_IN / DIR_OUT / DIR_INVALID
static int get_packet_direction(char *packet)
{
	//fprintf(stdout, "TODO: determine the direction of this packet.\n");
	struct iphdr *ip_hdr = packet_to_ip_hdr(packet);
	rt_entry_t *src_entry = longest_prefix_match(ntohl(ip_hdr->saddr));
	rt_entry_t *dest_entry = longest_prefix_match(ntohl(ip_hdr->daddr));
	if(src_entry->iface == nat.internal_iface && dest_entry->iface == nat.external_iface){
		return DIR_OUT;
	}
	else if(src_entry->iface == nat.external_iface && ntohl(ip_hdr->daddr) == nat.external_iface->ip){
		return DIR_IN;
	}
	return DIR_INVALID;
}

static u32 getport(){
	int i;
	for(i = NAT_PORT_MIN; i < NAT_PORT_MAX; ++i){
		if (!nat.assigned_ports[i]){
			nat.assigned_ports[i] = 1;
			break;
		}
	}
	return i;
}
static struct nat_mapping* insert_nat_entry(u32 internal_ip, u16 sport,u32 dest_ip,struct list_head *head){
	rt_entry_t *rt_entry = longest_prefix_match(dest_ip);
	struct nat_mapping* temp = (struct nat_mapping *)malloc(sizeof(struct nat_mapping));
	memset(temp,0,sizeof(struct nat_mapping));
	temp->external_ip = rt_entry->iface->ip;
	temp->external_port = getport();
	temp->internal_ip = internal_ip;
	temp->internal_port = sport;
	list_add_tail(&temp->list,head);
	return temp;
}
// do translation for the packet: replace the ip/port, recalculate ip & tcp
// checksum, update the statistics of the tcp connection
void do_translation(iface_info_t *iface, char *packet, int len, int dir)
{
	//fprintf(stdout, "TODO: do translation for this packet.\n");
	pthread_mutex_lock(&nat.lock);
	struct iphdr *ip = packet_to_ip_hdr(packet);
	struct tcphdr *tcp = packet_to_tcp_hdr(packet);
	int is_find = 0;
	struct nat_mapping  *nat_entry = NULL;
	u32 dest = ntohl(ip->daddr);
	u32 src = ntohl(ip->saddr);
	u16 sport = ntohs(tcp->sport);
	u16 dport = ntohs(tcp->dport);
	if(dir == DIR_IN){
		struct list_head *nat_entry_list = &nat.nat_mapping_list[hash8((char*)&src,4)];
		if(!list_empty(nat_entry_list)){
			list_for_each_entry(nat_entry, nat_entry_list, list){
				if(nat_entry->external_ip == dest && nat_entry->external_port == dport){
					is_find = 1;
					break;
				}
			}
		}
		if(!is_find){
			icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
			free(packet);
		}
		else{
			ip->daddr = htonl(nat_entry->internal_ip);
			ip->checksum = ip_checksum(ip);
			tcp->dport = htons(nat_entry->internal_port);
			tcp->checksum = tcp_checksum(ip,tcp);
			nat_entry->update_time = time(NULL);
		}
	}
	else{
		struct list_head *nat_entry_list = &nat.nat_mapping_list[hash8((char*)&dest,4)];
		if(!list_empty(nat_entry_list)){
			list_for_each_entry(nat_entry, nat_entry_list, list){
				if(nat_entry->internal_ip == src && nat_entry->internal_port == sport){
					is_find = 1;
					break;
				}
			}
		}
		if(!is_find){
			nat_entry = insert_nat_entry(src,sport,dest,nat_entry_list);
		}
		ip->saddr = htonl(nat_entry->external_ip);
		ip->checksum = ip_checksum(ip);
		tcp->sport = htons(nat_entry->external_port);
		tcp->checksum = tcp_checksum(ip,tcp);
		nat_entry->update_time = time(NULL);
	}
	ip_send_packet(packet,len);
	pthread_mutex_unlock(&nat.lock);
}

void nat_translate_packet(iface_info_t *iface, char *packet, int len)
{
	int dir = get_packet_direction(packet);
	if (dir == DIR_INVALID) {
		log(ERROR, "invalid packet direction, drop it.");
		icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
		free(packet);
		return ;
	}

	struct iphdr *ip = packet_to_ip_hdr(packet);
	if (ip->protocol != IPPROTO_TCP) {
		log(ERROR, "received non-TCP packet (0x%0hhx), drop it", ip->protocol);
		free(packet);
		return ;
	}

	do_translation(iface, packet, len, dir);
}

// nat timeout thread: find the finished flows, remove them and free port
// resource
void *nat_timeout()
{
	while (1) {
		//fprintf(stdout, "TODO: sweep finished flows periodically.\n");
		sleep(1);
		struct nat_mapping  *nat_entry = NULL;
		struct nat_mapping  *nat_entry_next = NULL;
		pthread_mutex_lock(&nat.lock);
		for(int i =0; i < HASH_8BITS;i ++){
			struct list_head *nat_entry_list = &nat.nat_mapping_list[i];
			if(list_empty(nat_entry_list))  
			  continue;
			list_for_each_entry_safe(nat_entry, nat_entry_next, nat_entry_list, list){
				if(nat_entry->conn.external_fin == 1 && 
							nat_entry->conn.internal_fin == 1 && 
							nat_entry->conn.external_seq_end == nat_entry->conn.internal_ack &&
							nat_entry->conn.internal_seq_end == nat_entry->conn.external_ack){
					list_delete_entry(&nat_entry->list);
					free(nat_entry);
				}
				else if(time(NULL) - nat_entry->update_time > TCP_ESTABLISHED_TIMEOUT){
					list_delete_entry(&nat_entry->list);
					free(nat_entry);
				}
			}
		}
		pthread_mutex_unlock(&nat.lock);
	}

	return NULL;
}

// initialize nat table
void nat_table_init()
{
	memset(&nat, 0, sizeof(nat));

	for (int i = 0; i < HASH_8BITS; i++)
	  init_list_head(&nat.nat_mapping_list[i]);

	nat.internal_iface = if_name_to_iface("n1-eth0");
	nat.external_iface = if_name_to_iface("n1-eth1");
	if (!nat.internal_iface || !nat.external_iface) {
		log(ERROR, "Could not find the desired interfaces for nat.");
		exit(1);
	}

	memset(nat.assigned_ports, 0, sizeof(nat.assigned_ports));

	pthread_mutex_init(&nat.lock, NULL);

	pthread_create(&nat.thread, NULL, nat_timeout, NULL);
}

// destroy nat table
void nat_table_destroy()
{
	pthread_mutex_lock(&nat.lock);

	for (int i = 0; i < HASH_8BITS; i++) {
		struct list_head *head = &nat.nat_mapping_list[i];
		struct nat_mapping *mapping_entry, *q;
		list_for_each_entry_safe(mapping_entry, q, head, list) {
			list_delete_entry(&mapping_entry->list);
			free(mapping_entry);
		}
	}

	pthread_kill(nat.thread, SIGTERM);

	pthread_mutex_unlock(&nat.lock);
}
