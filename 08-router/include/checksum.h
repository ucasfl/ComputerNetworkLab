#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include "types.h"

#include <assert.h>
// #include <arpa/inet.h>

static inline u16 checksum(u16 *ptr, int nbytes, u32 sum)
{
	assert(nbytes % 2 == 0);
 
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
 
    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);

    return (u16)~sum;
}

#endif
