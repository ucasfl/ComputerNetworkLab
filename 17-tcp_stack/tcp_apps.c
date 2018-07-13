#include "include/tcp_sock.h"

#include "include/log.h"

#include <unistd.h>
#include <pthread.h>

// tcp server application, listens to port (specified by arg) and serves only one
// connection request

void *tcp_server(void *arg)
{
	u16 port = *(u16 *)arg;
	struct tcp_sock *tsk = alloc_tcp_sock();

	struct sock_addr addr;
	addr.ip = htonl(0);
	addr.port = port;
	if (tcp_sock_bind(tsk, &addr) < 0) {
		log(ERROR, "tcp_sock bind to port %hu failed", ntohs(port));
		exit(1);
	}

	if (tcp_sock_listen(tsk, 3) < 0) {
		log(ERROR, "tcp_sock listen failed");
		exit(1);
	}

	log(DEBUG, "listen to port %hu.", ntohs(port));

	struct tcp_sock *csk = tcp_sock_accept(tsk);

	log(DEBUG, "accept a connection.");

	char rbuf[1001];
	FILE *fp = fopen("server-output.dat", "w+");
    if(fp == NULL){
        printf("Open Error\n");
        exit(0);
    }
	int rlen = 0;
	while (1) {
		rlen = tcp_sock_read(csk, rbuf, 1000);
		if (rlen < 0) {
			log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
			rbuf[rlen] = '\0';
           // printf("%s\n", rbuf);
			fprintf(fp, "%s", rbuf);
		}
	}

    fclose(fp);
	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}

    FILE *fp = fopen("1.dat", "r");
    if(fp == NULL){
        printf("Open Error!\n");
        exit(0);
    }
    char rbuf[1001];
    int i = 0;
    while(!feof(fp)){
        int j = fread(rbuf, 1, 1000, fp);
        printf("debug j : \t%d\n", j);
        if(tcp_sock_write(tsk, rbuf, 1000) < 0)
            break;
        sleep(1);
        printf("transmission %d success\n", i++);
    }
    while(!tcp_trans_finish(tsk))
        sleep(1);
    fclose(fp);
	tcp_sock_close(tsk);

	return NULL;
}

int tcp_sock_write(struct tcp_sock *tsk, char *buf, int len){

	int sent_len = 0;
	while(sent_len < len){
		if (tsk->snd_wnd == 0){
			sleep_on(tsk->wait_send);
		}
		int data_len = min(tsk->snd_wnd, min(len, 1500 - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE - TCP_BASE_HDR_SIZE));
		int pkt_size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE + data_len;

		char *packet = (char *) malloc (pkt_size);
		memset(packet, 0, pkt_size);
		memcpy(packet + ETHER_HDR_SIZE+IP_BASE_HDR_SIZE+TCP_BASE_HDR_SIZE, buf + sent_len, data_len);

		tcp_send_packet(tsk, packet, pkt_size);
		sent_len += data_len;
	}
	return 1;
}

int tcp_sock_read(struct tcp_sock *tsk, char *buf, int len){
	if (ring_buffer_empty(tsk->rcv_buf)){
		sleep_on(tsk->wait_recv);
	}
	pthread_mutex_lock(&tsk->rcv_buf->rw_lock);
	int read_len = read_ring_buffer(tsk->rcv_buf, buf, len);
	pthread_mutex_unlock(&tsk->rcv_buf->rw_lock);
	return read_len;
}

void remove_ack_data(struct tcp_sock *tsk, int ack_num){
	tcp_remove_retrans_timer(tsk);
	struct send_packet *pos, *q;
	list_for_each_entry_safe(pos, q, &tsk->send_buf, list){
		struct tcphdr *tcp = packet_to_tcp_hdr(pos->packet);
		struct iphdr *ip = packet_to_ip_hdr(pos->packet);
		if (ack_num >= ntohl(tcp->seq)){
			tsk->snd_wnd += (ntohs(ip->tot_len) - IP_HDR_SIZE(ip) - TCP_HDR_SIZE(tcp));
			free(pos->packet);
			list_delete_entry(&pos->list);
		}
	}
	if (!list_empty(&tsk->send_buf)){
		tcp_set_retrans_timer(tsk);
	}
}
