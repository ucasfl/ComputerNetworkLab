#include "hash.h"
#include "types.h"

u8 hash8(unsigned char *addr, int len)
{
	u8 result = 0;
	while (len > 0) {
		result ^= addr[--len];
	}

	return result;
}

u16 hash16(unsigned char *addr, int len)
{
	u16 result = 0;
	while (len >= 2) {
		result ^= *(u8 *)(addr + len - 2);
		len -= 2;
	}

	if (len) 
		result ^= (u8)(*addr);
	
	return result;
}
