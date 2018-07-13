#include "include/tcp.h"
#include "include/tcp_sock.h"
#include "include/tcp_timer.h"
#include "include/list.h"
#include "include/ip.h"

#include "include/log.h"
#include "include/ring_buffer.h"

#include <stdlib.h>

// handling incoming packet for TCP_LISTEN state
//
// 1. malloc a child tcp sock to serve this connection request; 
// 2. send TCP_SYN | TCP_ACK by child tcp sock;
// 3. hash the child tcp sock into established_table (because the 4-tuple 
//    is determined).
void tcp_state_listen(struct tcp_sock *tsk, 
	  struct tcp_cb *cb, char *packet)
{	
	if (cb->flags & TCP_SYN){
		struct tcp_sock *child_sock = alloc_tcp_sock();
		child_sock->local.ip = cb->daddr;
		child_sock->local.port = cb->dport;
		child_sock->peer.ip = cb->saddr;
		child_sock->peer.port = cb->sport;
		child_sock->parent = tsk;
		child_sock->rcv_nxt = cb->seq_end;
		child_sock->iss = tcp_new_iss();
		child_sock->snd_nxt = child_sock->iss;
		struct sock_addr skaddr = {htonl(child_sock->local.ip), htons(child_sock->local.port)};
		tcp_sock_bind(child_sock, &skaddr);
		tcp_set_state(child_sock, TCP_SYN_RECV);
		list_add_tail(&child_sock->list,
			  &tsk->listen_queue);
		tcp_send_control_packet(child_sock, 
			  TCP_SYN | TCP_ACK);
		tcp_hash(child_sock);
	}
	else {
		tcp_send_reset(cb);
	}
}

// handling incoming packet for TCP_CLOSED state, by replying TCP_RST
void tcp_state_closed(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	tcp_send_reset(cb);
}

// handling incoming packet for TCP_SYN_SENT state
//
// If everything goes well (the incoming packet is TCP_SYN|TCP_ACK), reply with 
// TCP_ACK, and enter TCP_ESTABLISHED state, notify tcp_sock_connect; otherwise, 
// reply with TCP_RST.
void tcp_state_syn_sent(struct tcp_sock *tsk, struct tcp_cb *cb, 
	  char *packet)
{
	if (cb->flags & (TCP_SYN | TCP_ACK)){
		tsk->rcv_nxt = cb->seq_end;
		tsk->snd_una = cb->ack;
		tcp_send_control_packet(tsk, TCP_ACK);
		tcp_set_state(tsk, TCP_ESTABLISHED);
		wake_up(tsk->wait_connect);
	}
	else {
		tcp_send_reset(cb);
	}
}

// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u16 old_snd_wnd = tsk->snd_wnd;
	tsk->snd_wnd = cb->rwnd;
	if (old_snd_wnd == 0)
	  wake_up(tsk->wait_send);
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	if (less_or_equal_32b(tsk->snd_una, cb->ack) && less_or_equal_32b(cb->ack, tsk->snd_nxt))
	  tcp_update_window(tsk, cb);
}

// handling incoming ack packet for tcp sock in TCP_SYN_RECV state
//
// 1. remove itself from parent's listen queue;
// 2. add itself to parent's accept queue;
// 3. wake up parent (wait_accept) since there is established connection in the
//    queue.
void tcp_state_syn_recv(struct tcp_sock *tsk, struct tcp_cb *cb, 
	  char *packet)
{
	if (cb->flags & TCP_ACK){
		tcp_sock_accept_enqueue(tsk);
		wake_up(tsk->parent->wait_accept);
	}
	else {
		tcp_send_reset(cb);
	}
}

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
	if (less_than_32b(cb->seq, rcv_end) && less_or_equal_32b(tsk->rcv_nxt, cb->seq_end)) {
		return 1;
	}
	else {
		log(ERROR, "received packet with invalid seq, drop it.");
		return 0;
	}
}

// Process an incoming packet as follows:
// 	 1. if the state is TCP_CLOSED, hand the packet over to tcp_state_closed;
// 	 2. if the state is TCP_LISTEN, hand it over to tcp_state_listen;
// 	 3. if the state is TCP_SYN_SENT, hand it to tcp_state_syn_sent;
// 	 4. check whether the sequence number of the packet is valid, if not, drop
// 	    it;
// 	 5. if the TCP_RST bit of the packet is set, close this connection, and
// 	    release the resources of this tcp sock;
// 	 6. if the TCP_SYN bit is set, reply with TCP_RST and close this connection,
// 	    as valid TCP_SYN has been processed in step 2 & 3;
// 	 7. check if the TCP_ACK bit is set, since every packet (except the first 
//      SYN) should set this bit;
//   8. process the ack of the packet: if it ACKs the outgoing SYN packet, 
//      establish the connection; (if it ACKs new data, update the window;)
//      if it ACKs the outgoing FIN packet, switch to correpsonding state;
//   9. (process the payload of the packet: call tcp_recv_data to receive data;)
//  10. if the TCP_FIN bit is set, update the TCP_STATE accordingly;
//  11. at last, do not forget to reply with TCP_ACK if the connection is alive.
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	int state = tsk->state;
	// 1.
	if (state == TCP_CLOSED) {
		tcp_state_closed(tsk, cb, packet);
		return ;
	}
	// 2.
	if (state == TCP_LISTEN) {
		tcp_state_listen(tsk, cb, packet);
		return ;
	}
	// 3.
	if (state == TCP_SYN_SENT) {
		tcp_state_syn_sent(tsk, cb, packet);
		return ;
	}
	// 4.
	if (!is_tcp_seq_valid(tsk, cb)) {
		log(ERROR, "tcp_process(): received packet with invalid seq, drop it.");
		return ;
	}
	// 5.
	if (cb->flags & TCP_RST) {
		tcp_set_state(tsk, TCP_CLOSED);
		tcp_unhash(tsk);
		return ;
	}
	// 6.
	if (cb->flags & TCP_SYN) {
		tcp_send_reset(cb);
		tcp_set_state(tsk, TCP_CLOSED);
		return ;
	}
	// 7.
	if (!(cb->flags & TCP_ACK)) {
		tcp_send_reset(cb);
		log(ERROR, "tcp_process(): TCP_ACK is not set.");
		return ;
	}
	// 8. 9. 10
	tsk->snd_una = cb->ack;
	tsk->rcv_nxt = cb->seq_end;
	// SYN_RCVD -> ESTABLISHED
	if ((state == TCP_SYN_RECV) && (cb->ack == tsk->snd_nxt))
		tcp_state_syn_recv(tsk, cb, packet);
	// FIN_WAIT_1 -> FIN_WAIT_2
	if ((state == TCP_FIN_WAIT_1) && (cb->ack == tsk->snd_nxt))
		tcp_set_state(tsk, TCP_FIN_WAIT_2);
	// FIN_WAIT_2 -> TIME_WAIT
	if ((state == TCP_FIN_WAIT_2) && (cb->flags & TCP_FIN)) {
		tcp_set_state(tsk, TCP_TIME_WAIT);
		tcp_set_timewait_timer(tsk);
		tcp_send_control_packet(tsk, TCP_ACK);
	}
	// ESTABLISHED -> CLOSE_WAIT
	if ((state == TCP_ESTABLISHED) && (cb->flags & TCP_FIN)) {
		tcp_set_state(tsk, TCP_CLOSE_WAIT);
		tcp_send_control_packet(tsk, TCP_ACK);
	}
	// LAST_ACK -> CLOSED
	if ((state == TCP_LAST_ACK) && (cb->ack == tsk->snd_nxt))
		tcp_set_state(tsk, TCP_CLOSED);
	// 11.
	if (state == TCP_ESTABLISHED && cb->flags == TCP_ACK){
		tcp_send_control_packet(tsk, TCP_ACK);
	}
}
