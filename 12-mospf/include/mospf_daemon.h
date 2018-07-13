#ifndef __MOSPF_DAEMON_H__
#define __MOSPF_DAEMON_H__

#include "base.h"
#include "types.h"
#include "list.h"

#define MAX_NUM 15
#define MAX_INT 255

void mospf_init();
void mospf_run();
void handle_mospf_packet(iface_info_t *iface, char *packet, int len);


#endif
