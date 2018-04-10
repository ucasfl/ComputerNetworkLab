#ifndef __BASE_H__
#define __BASE_H__

#include "types.h"
#include "ether.h"
#include "list.h"

#include <arpa/inet.h>

#define DYNAMIC_ROUTING

typedef struct {
	struct list_head iface_list;
	int nifs;
	struct pollfd *fds;

#ifdef DYNAMIC_ROUTING
	// used for ospf routing
	u32 area_id;	
	u32 router_id;
	u16 sequence_num;
	int lsuint;
#endif
} ustack_t;

extern ustack_t *instance;

typedef struct {
	struct list_head list;

	int fd;
	int index;
	u8	mac[ETH_ALEN];
	u32 ip;
	u32 mask;
	char name[16];
	char ip_str[16];

#ifdef DYNAMIC_ROUTING
	// list of ospf neighbors
	int helloint;
	int num_nbr;
	struct list_head nbr_list;
#endif
} iface_info_t;

char *ntoa_net(u32 addr);
char *ntoa_local(u32 addr);

#endif
