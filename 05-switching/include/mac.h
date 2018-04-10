#ifndef __MAC_H__
#define __MAC_H__

#include "base.h"
#include "hash.h"
#include "headers.h"

#define MAC_PORT_TIMEOUT 30

struct mac_port_entry {
	uint8_t mac[ETH_ALEN];
	iface_info_t *iface;
	time_t visited;
	struct mac_port_entry *next;
};

typedef struct mac_port_entry mac_port_entry_t;

typedef struct {
	mac_port_entry_t *hash_table[HASH_8BITS];
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	pthread_t tid;
} mac_port_map_t;

void *sweeping_mac_port_thread(void *);
void init_mac_hash_table();
void destory_mac_hash_table();
void dump_mac_port_table();
iface_info_t *lookup_port(uint8_t mac[ETH_ALEN], int search_des);
void insert_mac_port(uint8_t mac[ETH_ALEN], iface_info_t *iface);
int sweep_aged_mac_port_entry();

#endif
