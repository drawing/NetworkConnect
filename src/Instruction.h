#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <list>
#include <string>

#include "Packet.h"

#pragma pack(1)

struct Instruction
{
#define READ_ADDRESS 0x1
#define VIRTUAL_ADDRESS 0x2
#define IPV4 0x4
#define IPV6 0x8
#define LOCAL_ADDRESS 0x10
	uint16_t Flag;
#define DESTROY 0x1
	uint16_t Status;
	uint32_t IPAddr;
	uint32_t IPv6Reverse;
	uint16_t Port;
	uint16_t Identify;
	uint16_t Timeout;
	uint16_t Reversed;
	uint16_t TimeoutCount;
	uint16_t MaxTimeoutCount;
	uint32_t LastSendTime;
	Packet Data;
};

struct InstPacket
{
	uint8_t Opposite;
	uint8_t Version;
	uint16_t Length;
#define PACK_ACK 0x1
#define PACK_SYN 0x2
	uint16_t Flag;
	uint16_t Identify;
	uint32_t Timestamp;
#define LOGIN_REQUEST	1
#define LOGIN_RESPOND	2
#define HEARTBEAT		3
	uint32_t Type;
};

#define DATA_PTR(inst) \
	((uint8_t *)(inst) + sizeof(InstPacket))

struct LoginRequest
{
	uint8_t User[MAX_USERNAME_LEN];
	uint8_t Passwd[MAX_USERNAME_LEN];
};


struct Subnet
{
	uint32_t Address;
	uint32_t Mask;
};

struct LoginRespond
{
	uint32_t TunAddr;
	uint32_t Mask;
	uint32_t Gateway;
	uint32_t ProtectCount;
	Subnet Protect[1];
};

// 心跳包
struct Heartbeat
{
};

#pragma pack()

struct Packet;

void BuildHeader(Instruction * inst,
		uint16_t flag,
		uint32_t addr,
		uint16_t port,
		uint16_t timeout,
		uint16_t timeoutMaxCount);

bool BuildLoginRequest(Packet * packet,
	const std::string & user,
	const std::string & passwd,
	uint16_t identify);

bool BuildLoginRespond(Packet * packet,
	uint32_t addr,
	uint32_t mask,
	uint32_t gate,
	uint32_t count,
	Subnet * item,
	uint16_t identify);

bool BuildHeartbeat(Packet * packet,
	uint16_t identify);

#endif // INSTRUCTION_H
