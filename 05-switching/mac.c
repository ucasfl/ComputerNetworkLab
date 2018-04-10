#include "mac.h"
#include "headers.h"
#include "log.h"

mac_port_map_t mac_port_map;

void init_mac_hash_table()
{
	bzero(&mac_port_map, sizeof(mac_port_map_t));

	pthread_mutexattr_init(&mac_port_map.attr);
	pthread_mutexattr_settype(&mac_port_map.attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mac_port_map.lock, &mac_port_map.attr);

	pthread_create(&mac_port_map.tid, NULL, sweeping_mac_port_thread, NULL);
}

void destory_mac_hash_table()
{
	pthread_mutex_lock(&mac_port_map.lock);
	mac_port_entry_t *tmp, *entry;
	for (int i = 0; i < HASH_8BITS; i++) {
		entry = mac_port_map.hash_table[i];
		if (!entry) 
		  continue;

		tmp = entry->next;
		while (tmp) {
			entry->next = tmp->next;
			free(tmp);
			tmp = entry->next;
		}
		free(entry);
	}
	pthread_mutex_unlock(&mac_port_map.lock);
}

iface_info_t *lookup_port(u8 mac[ETH_ALEN], int search_des)
{
	// TODO: implement the lookup process here
	/*fprintf(stdout, "TODO: implement the lookup process here.\n");*/
	pthread_mutex_lock(&mac_port_map.lock);
	// count key value
	u8 key = hash8(mac, ETH_ALEN);
	//find port
	/*printf ("in loop up port\n");*/
	mac_port_entry_t *entry = mac_port_map.hash_table[key];
	if (entry){
		//update visited time
		if (search_des){
			entry->visited = time(NULL);
		}
		pthread_mutex_unlock(&mac_port_map.lock);
		return entry->iface;
	}
	pthread_mutex_unlock(&mac_port_map.lock);
	return NULL;
}

void insert_mac_port(u8 mac[ETH_ALEN], iface_info_t *iface)
{
	// TODO: implement the insertion process here
	/*fprintf(stdout, "TODO: implement the insertion process here.\n");*/
	u8 key = hash8(mac, ETH_ALEN);
	//find port
	pthread_mutex_lock(&mac_port_map.lock);
	/*printf ("in insert entry\n");*/
	mac_port_entry_t **entry = &mac_port_map.hash_table[key];
	if (!*entry){
		//malloc new entry
		*entry = (mac_port_entry_t *) malloc (sizeof(mac_port_entry_t));
		(*entry)->next = NULL;
		memcpy((*entry)->mac, mac, ETH_ALEN);
		(*entry)->iface = iface;
	}
	(*entry)->visited = time (NULL);
	mac_port_entry_t *tmp = (*entry)->next;
	mac_port_entry_t *pre = NULL;
	// insert entry into link list
	while (tmp){
		pre = tmp;
		tmp = tmp->next;
	}
	if ( pre ){
		pre->next = *entry;
		(*entry)->next = NULL;
	}
	pthread_mutex_unlock(&mac_port_map.lock);
}

void dump_mac_port_table()
{
	mac_port_entry_t *entry = NULL;
	time_t now = time(NULL);

	fprintf(stdout, "dumping the mac_port table:\n");
	pthread_mutex_lock(&mac_port_map.lock);
	for (int i = 0; i < HASH_8BITS; i++) {
		entry = mac_port_map.hash_table[i];
		while (entry) {
			fprintf(stdout, ETHER_STRING " -> %s, %d\n", ETHER_FMT(entry->mac), \
						entry->iface->name, (int)(now - entry->visited));

			entry = entry->next;
		}
	}

	pthread_mutex_unlock(&mac_port_map.lock);
}

int sweep_aged_mac_port_entry()
{
	// TODO: implement the sweeping process here
	/*fprintf(stdout, "TODO: implement the sweeping process here.\n");*/
	pthread_mutex_lock(&mac_port_map.lock);
	/*printf ("in sweep function\n");*/
	int n = 0;
	mac_port_entry_t *entry = NULL;
	time_t now = time(NULL);
	// scan all port
	for (int i = 0; i < HASH_8BITS; i++){
		entry = mac_port_map.hash_table[i];
		if ( !entry ){
			continue;
		}
		// judge whether TIMEOUT or not
		if ( entry->visited - now > MAC_PORT_TIMEOUT ){
			/*printf ("in if \n");*/
			n++;
			mac_port_entry_t *tmp = entry->next;
			// free all entry
			while (tmp){
				entry->next = tmp->next;
				free(tmp);
				tmp = entry->next;
			}
			free(entry);
		}
	}
	pthread_mutex_unlock(&mac_port_map.lock);
	return n;
}

void *sweeping_mac_port_thread(void *nil)
{
	while (1) {
		sleep(1);
		int n = sweep_aged_mac_port_entry();
		/*printf (" seep port num = %d\n", n);*/

		if (n > 0)
		  log(DEBUG, "%d aged entries in mac_port table are removed.\n", n);
	}

	return NULL;
}
