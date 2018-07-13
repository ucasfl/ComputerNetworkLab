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
	char wbuf[1024];
	int rlen = 0;
	while (1) {
		rlen = tcp_sock_read(csk, rbuf, 1000);
		if (rlen < 0) {
			log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
	        printf("tcp_sock read %d bytes of data.\n", rlen);
			rbuf[rlen] = '\0';
			sprintf(wbuf, "server echoes: %s", rbuf);
			if (tcp_sock_write(csk, wbuf, strlen(wbuf)) < 0) {
				log(DEBUG, "tcp_sock_write return negative value, finish transmission.");
				break;
			}
		}
	}

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

	char *wbuf = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int wlen = strlen(wbuf);
	char rbuf[1001];
	int rlen = 0;

	int n = 10;
	for (int i = 0; i < n; i++) {
		if (tcp_sock_write(tsk, wbuf + i, wlen - n) < 0)
			break;

		rlen = tcp_sock_read(tsk, rbuf, 1000);
		if (rlen < 0) {
			log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
			break;
		}
		else if (rlen > 0) {
	        printf("tcp_sock read %d bytes of data.\n", rlen);
			rbuf[rlen] = '\0';
			fprintf(stdout, "%s\n", rbuf);
		}
		else {
			fprintf(stdout, "*** read data == 0.\n");
		}
		sleep(1);
	}

	tcp_sock_close(tsk);

	return NULL;
}

int tcp_sock_write(struct tcp_sock *tsk, char *buf, int len){

	int data_len = min(len, 1500 - ETHER_HDR_SIZE - 
		  IP_BASE_HDR_SIZE - TCP_BASE_HDR_SIZE);
	int pkt_size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + 
	  TCP_BASE_HDR_SIZE + data_len;

	char *packet = (char *) malloc (pkt_size);
	memset(packet, 0, pkt_size);
	memcpy(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + 
		  TCP_BASE_HDR_SIZE, buf, data_len);

	tcp_send_packet(tsk, packet, pkt_size);

	sleep_on(tsk->wait_send);
	return data_len;
}

int tcp_sock_read(struct tcp_sock *tsk, char *buf, int len){
	pthread_mutex_lock(&tsk->rcv_buf->rw_lock);
	int not_sleep = 1;

	if (ring_buffer_empty(tsk->rcv_buf)){
		pthread_mutex_unlock(&tsk->rcv_buf->rw_lock);
		not_sleep = 0;
		sleep_on(tsk->wait_recv);
	}
	if (!not_sleep){
		pthread_mutex_lock(&tsk->rcv_buf->rw_lock);
	}
	int read_len = read_ring_buffer(tsk->rcv_buf, buf, len);
	pthread_mutex_unlock(&tsk->rcv_buf->rw_lock);
	return read_len;
}
