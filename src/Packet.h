#ifndef PACKET_H
#define PACKET_H

#include "Config.h"
#include <stdint.h>

#define MAX_PACKET_BUFFER 1024 * 8

struct Packet
{
	uint8_t Buffer[MAX_PACKET_BUFFER];
	int32_t Len;
};

#endif // PACKET_H
