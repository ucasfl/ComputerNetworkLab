#include "include/mospf_daemon.h"
#include "include/mospf_proto.h"
#include "include/mospf_nbr.h"
#include "include/mospf_database.h"
#include "include/packet.h"
#include "include/rtable.h"

#include "include/ip.h"

#include "include/list.h"
#include "include/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

extern ustack_t *instance;

pthread_mutex_t mospf_lock;

u32 Dist[MAX_NUM] = {0};
u32 Prev[MAX_NUM] = {0};
u32 Node[MAX_NUM] = {0};
bool Visited[MAX_NUM] = {0};
u32 Graph[MAX_NUM][MAX_NUM] = {0};


void mospf_init()
{
	pthread_mutex_init(&mospf_lock, NULL);

	instance->area_id = 0;
	// get the ip address of the first interface
	iface_info_t *iface = list_entry(instance->iface_list.next, iface_info_t, list);
	instance->router_id = iface->ip;
	instance->sequence_num = 0;
	instance->lsuint = MOSPF_DEFAULT_LSUINT;

	iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		iface->helloint = MOSPF_DEFAULT_HELLOINT;
		init_list_head(&iface->nbr_list);
	}

	init_mospf_db();
}

void *sending_mospf_hello_thread(void *param);
void *sending_mospf_lsu_thread(void *param);
void *checking_nbr_thread(void *param);
void *caculate_rtable_thread(void *param);
void *checking_database_thread(void *param);

void mospf_run()
{
	pthread_t hello, lsu, nbr, db, rtable;
	pthread_create(&hello, NULL, sending_mospf_hello_thread, NULL);
	pthread_create(&lsu, NULL, sending_mospf_lsu_thread, NULL);
	pthread_create(&nbr, NULL, checking_nbr_thread, NULL);
	pthread_create(&db, NULL, checking_database_thread, NULL);
	pthread_create(&rtable, NULL, caculate_rtable_thread, NULL);
}

void *sending_mospf_hello_thread(void *param)
{
	fprintf(stdout, "TODO: send mOSPF Hello message periodically.\n");
	while (1){
		printf("In sending hello thread.\n");
		sleep(MOSPF_DEFAULT_HELLOINT);
		pthread_mutex_lock(&mospf_lock);
		iface_info_t *iface;
		list_for_each_entry(iface, &instance->iface_list, list){
			int len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE;
			char *hello_packet = (char *)malloc(len);
			struct ether_header *eth = (struct ether_header *)hello_packet;
			memcpy(eth->ether_shost, iface->mac, ETH_ALEN);
			u8 dhost[ETH_ALEN] = {0x01, 0x00, 0x5e, 0x00, 0x00, 0x05};
			memcpy(eth->ether_dhost, dhost, ETH_ALEN);
			eth->ether_type = htons(ETH_P_IP);

			struct iphdr *iph = packet_to_ip_hdr(hello_packet);
			ip_init_hdr(iph, iface->ip, 0xe0000005, len - ETHER_HDR_SIZE, 90);

			struct mospf_hdr *mohdr = (struct mospf_hdr *)(hello_packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);
			mospf_init_hdr(mohdr, 0x1, MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE, instance->router_id, 0);

			struct mospf_hello *hello = (struct mospf_hello *)(hello_packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE);
			mospf_init_hello(hello, iface->mask);
			mohdr->checksum = mospf_checksum(mohdr);
			iface_send_packet(iface, hello_packet, len);
		}
		pthread_mutex_unlock(&mospf_lock);
	}
	return NULL;
}

void *checking_nbr_thread(void *param)
{
	fprintf(stdout, "TODO: neighbor list timeout operation.\n");
	while (1){
		sleep(1);
		pthread_mutex_lock(&mospf_lock);
		printf("In checking nbr thread.\n");
		iface_info_t *iface;
		list_for_each_entry(iface, &instance->iface_list, list){
			mospf_nbr_t *mos_pos, *mos_q;
			list_for_each_entry_safe(mos_pos, mos_q, &iface->nbr_list, list){
				if (mos_pos->alive > 3 * MOSPF_DEFAULT_HELLOINT){
					list_delete_entry(&mos_pos->list);
					free(mos_pos);
					--iface->num_nbr;
				}
				else {
					++mos_pos->alive;
				}
			}
		}
		pthread_mutex_unlock(&mospf_lock);
	}
	return NULL;
}

void *checking_database_thread(void *param){
	time_t now;
	rt_entry_t *rt_pos, *rt_q;
	mospf_db_entry_t *db_pos, *db_q;
	while(1){
		sleep(1);
		pthread_mutex_lock(&mospf_lock);
		printf ("In check database thread.\n");
		if (!list_empty(&mospf_db)){
			now = time(NULL);
			list_for_each_entry_safe(db_pos, db_q, &mospf_db, list){
				if (now - db_pos->add_time >= 35){
					list_for_each_entry_safe(rt_pos, rt_q, &rtable, list){
						if (rt_pos->gw != 0)
						  remove_rt_entry(rt_pos);
					}
					list_delete_entry(&db_pos->list);
					free(db_pos);
				}
			}
		}
		pthread_mutex_unlock(&mospf_lock);
	}
	return NULL;
}

void handle_mospf_hello(iface_info_t *iface, const char *packet, int len)
{
	/*fprintf(stdout, "TODO: handle mOSPF Hello message.\n");*/
	printf("In handle hello.\n");
	pthread_mutex_lock(&mospf_lock);
	struct iphdr *iph = packet_to_ip_hdr(packet);
	struct mospf_hdr *moph = (struct mospf_hdr *)(packet + IP_BASE_HDR_SIZE + ETHER_HDR_SIZE);
	struct mospf_hello *hello = (struct mospf_hello *)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE);

	u32 hello_ip = ntohl(iph->saddr);
	u32 hello_rid = ntohl(moph->rid);
	u32 hello_mask = ntohl(hello->mask);
	mospf_nbr_t *mos_nbr;
	int flag = 0;
	list_for_each_entry(mos_nbr, &iface->nbr_list, list){
		if (mos_nbr->nbr_ip == hello_ip){
			flag = 1;
			mos_nbr->alive = 0;
		}
	}
	if (!flag){
		mospf_nbr_t *new_nbr = (mospf_nbr_t *)malloc(sizeof(mospf_nbr_t));
		new_nbr->nbr_id = hello_rid;
		new_nbr->nbr_ip = hello_ip;
		new_nbr->nbr_mask = hello_mask;
		new_nbr->alive = 0;
		list_add_tail(&new_nbr->list, &iface->nbr_list);
		++iface->num_nbr;
	}
	pthread_mutex_unlock(&mospf_lock);
	printf ("Leave handle hello.\n");
}

void *sending_mospf_lsu_thread(void *param)
{
	fprintf(stdout, "TODO: send mOSPF LSU message periodically.\n");
	while(1){
		sleep(MOSPF_DEFAULT_LSUINT);

		pthread_mutex_lock(&mospf_lock);
		printf ("In sending lsu thread.\n");
		int num_adv = 0;
		iface_info_t *iface;
		list_for_each_entry(iface, &(instance->iface_list), list){
			if(iface->num_nbr == 0)
			  num_adv ++;
			else
			  num_adv += iface->num_nbr;
		}
		int pac_len = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + MOSPF_HDR_SIZE + MOSPF_LSU_SIZE + num_adv * MOSPF_LSA_SIZE;
		char *packet = (char *)malloc(pac_len);
		mospf_nbr_t *mos_nbr;
		struct mospf_lsa *mos_lsa;
		int i = 0;
		list_for_each_entry(iface, &(instance->iface_list), list){
			if(iface->num_nbr == 0){
				mos_lsa = (struct mospf_lsa *)(packet + pac_len - (num_adv - i) * MOSPF_LSA_SIZE);
				++i;
				mos_lsa->subnet = htonl(iface->ip & iface->mask);
				mos_lsa->mask = htonl(iface->mask);
				mos_lsa->rid = 0;
				continue;
			}
			list_for_each_entry(mos_nbr, &(iface->nbr_list), list){
				mos_lsa = (struct mospf_lsa *)(packet + pac_len - (num_adv - i) * MOSPF_LSA_SIZE);
				++i;
				mos_lsa->subnet = ntohl(mos_nbr->nbr_ip & mos_nbr->nbr_mask);
				mos_lsa->mask = ntohl(mos_nbr->nbr_mask);
				mos_lsa->rid = ntohl(mos_nbr->nbr_id);
			}
		}
		list_for_each_entry(iface, &(instance->iface_list), list){
			list_for_each_entry(mos_nbr, &(iface->nbr_list), list){
				char *packet_t = (char *)malloc(pac_len);
				memcpy(packet_t, packet, pac_len);
				struct ether_header *eth = (struct ether_header *)packet_t;
				eth->ether_type = htons(ETH_P_IP);       
				memcpy(eth->ether_shost, iface->mac, ETH_ALEN);     

				struct iphdr *iph = (struct iphdr *)(packet_t + ETHER_HDR_SIZE);
				ip_init_hdr(iph, iface->ip, mos_nbr->nbr_ip, pac_len - ETHER_HDR_SIZE, 90);

				struct mospf_hdr * mospf = (struct mospf_hdr *)(packet_t + IP_BASE_HDR_SIZE + ETHER_HDR_SIZE);
				mospf_init_hdr(mospf, MOSPF_TYPE_LSU, pac_len - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE, instance->router_id, instance->area_id);  


				struct mospf_lsu *mos_lsu = (struct mospf_lsu *)((char *)mospf + MOSPF_HDR_SIZE);
				mospf_init_lsu(mos_lsu, num_adv);
				mospf->checksum = mospf_checksum(mospf);
				ip_send_packet(packet_t, pac_len);
			}

		}
		instance->sequence_num ++;
		free(packet);
		pthread_mutex_unlock(&mospf_lock);
	}
	return NULL;
}

void handle_mospf_lsu(iface_info_t *iface, char *packet, int len)
{
	/*fprintf(stdout, "TODO: handle mOSPF LSU message.\n");*/
	pthread_mutex_lock(&mospf_lock);
	mospf_db_entry_t *mos_db_en;
	int flag = 0;
	struct mospf_hdr * mospf = (struct mospf_hdr *)(packet + IP_BASE_HDR_SIZE + ETHER_HDR_SIZE);
	struct mospf_lsu *mos_lsu = (struct mospf_lsu *)((char *)mospf + MOSPF_HDR_SIZE);
	struct mospf_lsa *mos_lsa = (struct mospf_lsa *)((char *)mos_lsu + MOSPF_LSU_SIZE);
	int mospf_rid = ntohl(mospf->rid);
	fprintf(stdout, IP_FMT"\t\n",
				HOST_IP_FMT_STR(mospf_rid)
		   );
	if(instance->router_id == mospf_rid) return;
	int seq_num = ntohs(mos_lsu->seq);
	int nadv = ntohl(mos_lsu->nadv);
	printf ("In handle lus.\n");
	list_for_each_entry(mos_db_en, &(mospf_db), list){
		if(mospf_rid == mos_db_en->rid && mos_db_en->seq >= seq_num)
		  flag = 1;
		else if(mospf_rid == mos_db_en->rid && mos_db_en->seq < seq_num){
			flag = 1;
			free(mos_db_en->array);
			mos_db_en->array = (struct mospf_lsa *)malloc(nadv * MOSPF_LSA_SIZE);
			mos_db_en->add_time = time(NULL);
			for(int i = 0; i < nadv; i++){
				struct mospf_lsa *lsa = (struct mospf_lsa *)((char *)mos_lsa + i * MOSPF_LSA_SIZE);
				mos_db_en->array[i].rid = ntohl(lsa->rid);
				mos_db_en->array[i].subnet = ntohl(lsa->subnet);
				mos_db_en->array[i].mask = ntohl(lsa->mask);
			}
		}
	}
	if(! flag){
		mospf_db_entry_t * mospf_db_en_t = (mospf_db_entry_t *)malloc(sizeof(mospf_db_entry_t));
		mospf_db_en_t->add_time = time(NULL);
		mospf_db_en_t->rid = mospf_rid;
		mospf_db_en_t->seq = seq_num;
		mospf_db_en_t->nadv = nadv;

		mospf_db_en_t->array = (struct mospf_lsa *)malloc(nadv * MOSPF_LSA_SIZE);
		for(int i = 0; i < nadv; i++){
			struct mospf_lsa *lsa = (struct mospf_lsa *)((char *)mos_lsa + i * MOSPF_LSA_SIZE);
			mospf_db_en_t->array[i].rid = ntohl(lsa->rid);
			mospf_db_en_t->array[i].subnet = ntohl(lsa->subnet);
			mospf_db_en_t->array[i].mask = ntohl(lsa->mask);
		}
		list_add_tail(&(mospf_db_en_t->list), &mospf_db);
	}
	mospf_db_entry_t *mosdb;
	list_for_each_entry(mosdb, &mospf_db, list){
		for(int i = 0;i < mosdb->nadv; i++)
		  fprintf(stdout, IP_FMT"\t"IP_FMT"\t"IP_FMT"\t"IP_FMT"\n",
					  HOST_IP_FMT_STR(mosdb->rid),
					  HOST_IP_FMT_STR(mosdb->array[i].subnet), 
					  HOST_IP_FMT_STR(mosdb->array[i].mask),
					  HOST_IP_FMT_STR(mosdb->array[i].rid)
				 );
	}

	mos_lsu->ttl -= 1;
	if(mos_lsu->ttl > 0){
		iface_info_t *iface_t;
		mospf_nbr_t *mos_nbr;
		list_for_each_entry(iface_t, &(instance->iface_list), list){
			if(iface_t->index == iface->index)
			  continue;
			list_for_each_entry(mos_nbr, &(iface_t->nbr_list), list){
				char *packet_t = (char *)malloc(len);
				memcpy(packet_t, packet, len);
				struct ether_header *eth = (struct ether_header *)packet_t;    
				memcpy(eth->ether_shost, iface_t->mac, ETH_ALEN);
				struct iphdr *iph = (struct iphdr *)(packet_t + ETHER_HDR_SIZE);
				struct mospf_hdr * mospf = (struct mospf_hdr *)(packet_t + IP_BASE_HDR_SIZE + ETHER_HDR_SIZE);
				mospf->checksum = mospf_checksum(mospf);
				ip_init_hdr(iph, iface_t->ip, mos_nbr->nbr_ip, len - ETHER_HDR_SIZE, 90);
				ip_send_packet(packet_t, len);
			}
		}
	}
	pthread_mutex_unlock(&mospf_lock);
	printf ("Leave handle lsu.\n");
}

void handle_mospf_packet(iface_info_t *iface, char *packet, int len)
{
	struct iphdr *ip = (struct iphdr *)(packet + ETHER_HDR_SIZE);
	struct mospf_hdr *mospf = (struct mospf_hdr *)((char *)ip + IP_HDR_SIZE(ip));

	if (mospf->version != MOSPF_VERSION) {
		log(ERROR, "received mospf packet with incorrect version (%d)", mospf->version);
		return ;
	}
	if (mospf->checksum != mospf_checksum(mospf)) {
		log(ERROR, "received mospf packet with incorrect checksum");
		return ;
	}
	if (ntohl(mospf->aid) != instance->area_id) {
		log(ERROR, "received mospf packet with incorrect area id");
		return ;
	}

	// log(DEBUG, "received mospf packet, type: %d", mospf->type);

	switch (mospf->type) {
		case MOSPF_TYPE_HELLO:
			handle_mospf_hello(iface, packet, len);
			break;
		case MOSPF_TYPE_LSU:
			handle_mospf_lsu(iface, packet, len);
			break;
		default:
			log(ERROR, "received mospf packet with unknown type (%d).", mospf->type);
			break;
	}
}

static void init_Graph(){
	printf ("In init_Graph.\n");
	for( int i = 0; i != MAX_NUM; ++i ){
		for (int j = 0; j != MAX_NUM; ++j){
			Graph[i][j] = (i == j)? 0 : MAX_INT;
		}
	}
	printf("Leave init_Graph.\n");
}

static int get_Node(int rid, int node_num){
	printf("In get_Node.\n");
	for(int i = 0; i != node_num; ++i){
		if (rid == Node[i]){
			return i;
		}
	}
	assert(0);
	return -1;
}

static int caculate_graph(){
	printf("In caculate_graph.\n");
	printf("\nInstance rid = %d\n\n", instance->router_id);
	int node_num =  0;
	int x, y;
	mospf_db_entry_t *db_pos, *db_q;
	init_Graph();
	Node[node_num++] = instance->router_id;
	if (list_empty(&mospf_db)){
		printf("Emmty Database.\n");
		return node_num;
	}
	list_for_each_entry_safe(db_pos, db_q, &mospf_db, list){
		Node[node_num++] = db_pos->rid;
	}
	printf("\n Node_num = %d\n\n", node_num);
	for (int i = 0; i != node_num; ++i){
		printf("%d\t", Node[i]);
	}
	printf("\n");
	list_for_each_entry_safe(db_pos, db_q, &mospf_db, list){
		for(int i = 0; i != db_pos->nadv; ++i){
			printf("array rid = %d\n", db_pos->array[i].rid);
			printf("db rid = %d\n", db_pos->rid);
			if (db_pos->array[i].rid == 0)
			  continue;
			x = get_Node(db_pos->array[i].rid, node_num);
			y = get_Node(db_pos->rid, node_num);
			Graph[x][y] = Graph[y][x] = 1;
		}
	}
	return node_num;
}

static int Min_dist(int Node_num){
	printf("In Min_dist.\n");
	int rank = -1;
	int Min = MAX_INT;
	for(int i = 0; i != Node_num; ++i){
		if (!Visited[i] && Dist[i] < Min){
			Min = Dist[i];
			rank = i;
		}
	}
	assert(rank != -1);
	return rank;
}

static void Dijkstra(int Node_num){
	printf("In Dijkstra.\n");
	for (int i = 0; i < Node_num; ++i){
		Visited[i] = 0;
		Dist[i] = Graph[0][i];
		if (Dist[i] == MAX_INT || !Dist[i])
		  Prev[i] = -1;
		else 
		  Prev[i] = 0;
	}

	Dist[0] = 0;
	Visited[0] = 1;
	int u = 0;

	for (int i = 0; i != Node_num -1; ++i){
		u = Min_dist(Node_num);
		Visited[u] = 1;
		for(int v = 0; v != Node_num; ++v){
			if (Visited[v] == 0 &&
						Graph[u][v] > 0 &&
						Dist[u] != MAX_INT &&
						Dist[u] + Graph[u][v] < Dist[v]){
				Dist[v] = Dist[u] + Graph[u][v];
				Prev[v] = u;
			}
		}
	}
	printf("Leave Dijkstra.\n");
}

static bool is_in_rtable(u32 subnet){
	printf("In is_in_rtable.\n");
	rt_entry_t *rt_pos, *rt_q;
	list_for_each_entry_safe(rt_pos, rt_q, &rtable, list){
		if ((rt_pos->dest & rt_pos->mask) == subnet){
			return 1;
		}
	}
	return 0;
}

static void 
get_iface_and_gw (u32 rid, iface_info_t **iface_addr, u32 *gw){
	printf("In get_iface_and_gw.\n");
	iface_info_t *iface ;
	mospf_nbr_t *nbr_pos, *nbr_q;
	list_for_each_entry(iface, &instance->iface_list, list){
		list_for_each_entry_safe(nbr_pos, nbr_q, &iface->nbr_list, list){
			if (nbr_pos->nbr_id == rid){
				*iface_addr = iface;
				*gw = nbr_pos->nbr_ip;
				return;
			}
		}
	}
}

static void swap(u32 *a, u32 *b){
	u32 tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

void *caculate_rtable_thread(void *param){
	while(1){
		sleep(10);
		pthread_mutex_lock(&mospf_lock);
		printf("In caculate_rtable.\n");
		int Node_num = caculate_graph();
		printf("Node_num = %d\n", Node_num);
		Dijkstra(Node_num);
		u32 Dist_rank[Node_num];
		for(int i = 0; i != Node_num; ++i)
		  Dist_rank[i] = i;
		for (int i = 0; i != Node_num -1; ++i)
		  for (int j = 0; i != Node_num - i -1; ++j){
			  if (Dist[j] > Dist[j+1]){
				  swap(&Dist_rank[j], &Dist_rank[j+1]);
				  swap(&Dist[j] , &Dist[j+1]);
			  }
		  }
		mospf_db_entry_t *db_pos, *db_q;
		u32 gw, dest;
		int hop = -1;
		iface_info_t **iface_addr = NULL;
		for (int i = 0; i != Node_num; ++i){
			if(Prev[Dist_rank[i]] != -1 && !list_empty(&mospf_db)){
				list_for_each_entry_safe(db_pos, db_q, &mospf_db, list){
					for (int j = 0; j != db_pos->nadv; ++j){
						if (!is_in_rtable(db_pos->array[j].subnet)){
							dest = db_pos->array[j].subnet;
							hop = get_Node(db_pos->rid, Node_num);
							while (Prev[hop])
							  hop = Prev[hop];
							get_iface_and_gw(Node[hop], iface_addr, &gw);
							add_rt_entry(new_rt_entry(dest, (*iface_addr)->mask,gw, *iface_addr));
						}
					}
				}
			} 
		}
		printf("====RTable====\n");
		print_rtable();
		pthread_mutex_unlock(&mospf_lock);
	}
}
