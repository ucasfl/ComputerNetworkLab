#include "include/tcp.h"
#include "include/tcp_timer.h"
#include "include/tcp_sock.h"

#include <unistd.h>

static struct list_head timer_list;

// scan the timer_list, find the tcp sock which stays for at 2*MSL, release it
void tcp_scan_timer_list()
{
	struct tcp_sock *tsk;
	struct tcp_timer *t, *q;
	list_for_each_entry_safe(t, q, &timer_list, list) {
		t->timeout -= TCP_TIMER_SCAN_INTERVAL;
		if (t->timeout <= 0 && t->type == 0) {
			list_delete_entry(&t->list);

			// only support time wait now
			tsk = timewait_to_tcp_sock(t);
			if (! tsk->parent)
				tcp_bind_unhash(tsk);
			tcp_set_state(tsk, TCP_CLOSED);
			free_tcp_sock(tsk);
		}
		else if(t->timeout <= 0 && (t->type == 1)){
			tsk = retranstimer_to_tcp_sock(t);
            if(t->retrans_number == 3){
            	tcp_sock_close(tsk);
                //free_tcp_sock(tsk);
                return ;
            }
            struct send_packet *buf_pac = list_entry(tsk->send_buf.next, struct send_packet, list);

            if(!list_empty(&tsk->send_buf)){
                char *packet = (char *)malloc(buf_pac->len);
                memcpy(packet, buf_pac->packet, buf_pac->len);
                /*struct tcphdr *tcp = packet_to_tcp_hdr(packet);*/
                /*struct iphdr *ip = packet_to_ip_hdr(packet);*/

                ip_send_packet(packet, buf_pac->len);
                t->timeout = TCP_RETRANS_INTERVAL;

			    for(int i = 0; i <= t->retrans_number; i++){
			    	t->timeout *= 2;
			    }
			    t->retrans_number ++;
			}
		}
	}
}

// set the timewait timer of a tcp sock, by adding the timer into timer_list
void tcp_set_timewait_timer(struct tcp_sock *tsk)
{
	struct tcp_timer *timer = &tsk->timewait;

	timer->type = 0;
	timer->timeout = TCP_TIMEWAIT_TIMEOUT;
	list_add_tail(&timer->list, &timer_list);

	tcp_sock_inc_ref_cnt(tsk);
}

// scan the timer_list periodically by calling tcp_scan_timer_list
void *tcp_timer_thread(void *arg)
{
	init_list_head(&timer_list);
	while (1) {
		usleep(TCP_TIMER_SCAN_INTERVAL);
		tcp_scan_timer_list();
	}

	return NULL;
}

void tcp_set_retrans_timer(struct tcp_sock *tsk){
	struct tcp_timer *timer = &tsk->retrans_timer;

	timer->type = 1;
	timer->timeout = TCP_RETRANS_INTERVAL;
	timer->retrans_number = 0;

	list_add_tail(&timer->list, &timer_list);
}

void tcp_remove_retrans_timer(struct tcp_sock *tsk){
	list_delete_entry(&tsk->retrans_timer.list);
}
