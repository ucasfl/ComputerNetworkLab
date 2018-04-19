#ifndef __UTILS_H__
#define __UTILS_H__

#include "types.h"
#include <arpa/inet.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define htonll(x) ((1==htonl(1)) ? (x) : ((u64)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) htonll(x)

#endif
