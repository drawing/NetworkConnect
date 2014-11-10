#include "Config.h"

#include "Instruction.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "Packet.h"

void BuildHeader(Instruction * inst,
		uint16_t flag,
		uint32_t addr,
		uint16_t port,
		uint16_t timeout,
		uint16_t timeoutMaxCount)
{
	static uint16_t IdentifyBase = static_cast<uint16_t>(rand()); 
	
	inst->Flag = flag;
	inst->Status = 0;
	inst->IPAddr = addr;
	inst->Port = port;
	inst->Identify = IdentifyBase++;
	inst->Timeout = timeout;
	inst->TimeoutCount = 0;
	inst->MaxTimeoutCount = timeoutMaxCount;
	inst->LastSendTime = time(0);
}

void BuildPacket(Packet * packet, uint16_t flag,
	uint16_t packet_len, uint16_t identify, uint32_t type)
{
	assert(packet);
	InstPacket * instPacket = (InstPacket *)packet->Buffer;
	
	instPacket->Opposite = 0;
	instPacket->Version = 0;
	instPacket->Flag = flag;
	instPacket->Length = packet_len;
	instPacket->Identify = identify;
	instPacket->Timestamp = 0;
	instPacket->Type = type;
	
	packet->Len = packet_len;
}

bool BuildLoginRequest(Packet * packet,
	const std::string & user,
	const std::string & passwd,
	uint16_t identify)
{
	assert(packet);
	
	LoginRequest request;
	
	if (user.size() < MAX_USERNAME_LEN &&
		passwd.size() < MAX_USERNAME_LEN)
	{
		BuildPacket(packet, PACK_SYN, sizeof (LoginRequest), identify, LOGIN_REQUEST);
		
		strcpy((char*)request.User, user.c_str());
		strcpy((char*)request.Passwd, passwd.c_str());
		memcpy(packet->Buffer + sizeof (InstPacket),
				&request, sizeof request);
		
		packet->Len = sizeof (InstPacket) + sizeof request;
		return true;
	}
	else
	{
		return false;
	}
}

bool BuildLoginRespond(Packet * packet,
	uint32_t addr,
	uint32_t mask,
	uint32_t gate,
	uint32_t count,
	Subnet * item,
	uint16_t identify)
{
	assert(packet);
	assert(item);

	uint32_t dataLen = sizeof (LoginRespond) + (count - 1) * sizeof (Subnet);
	BuildPacket(packet, PACK_ACK, dataLen, identify, LOGIN_RESPOND);
	
	LoginRespond * respond = (LoginRespond *)(packet->Buffer + sizeof (InstPacket));
	respond->TunAddr = addr;
	respond->Mask = mask;
	respond->Gateway = gate;
	respond->ProtectCount = count;
	memcpy(respond->Protect, item, count * sizeof (Subnet));

	packet->Len = sizeof (InstPacket) + dataLen;
	
	return true;
}

bool BuildHeartbeat(Packet * packet,
	uint16_t identify)
{
	assert(packet);
	
	// 一字节心跳包（防止0字节包出现）
	BuildPacket(packet, 0, 1, identify, HEARTBEAT);
	
	return true;
}

