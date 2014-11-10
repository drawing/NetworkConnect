#include "Config.h"
#include "Proto.h"


#include <stdint.h>
#include <SessionMgr.h>
#include <Instruction.h>

#include "Util.h"

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#define MAX_DATA 2048
#include <ws2tcpip.h>

bool SendUDP(
	SOCKET s,
	uint32_t dest, uint16_t dport,
	uint32_t src, uint16_t sport,
	uint16_t dataLen)
{
	static uint16_t base = 0;
	uint8_t data[MAX_DATA];
	IPHDR * ip = (IPHDR *)data;
	ip->version_len = 0x45;
	ip->tos = 0;
	ip->tot_len = htons(20 + 8 + dataLen);
	ip->id = 0;//base ++;
	ip->frag_off = 0;
	ip->ttl = 0x7F;
	ip->protocol = IP_PROTO_UDP;
	ip->check_sum = 0;
	ip->saddr = src;
	ip->daddr = dest;
	
	// 计算 IP 校验和
	uint16_t * p = (uint16_t *)ip;
	uint32_t check = 0;
	for (int i = 0; i < 10; ++i)
	{
		check += p[i];
	}
	check = (check >> 16) + (check & 0xFFFF);
	check += (check >> 16);

	ip->check_sum = ~check;
	
	UDPHDR * udp = (UDPHDR *)(data + 20);
	udp->source = sport;
	udp->dest = dport;
	udp->len = htons(8 + dataLen);
	udp->check = 0;
	
	memset(data + 20 + 8, 'T', dataLen);
	memset(data + 20 + 8 + dataLen, 0, 3);
	
	// 计算 UDP 校验和
	p = (uint16_t *)udp;
	check = 0;
	for (int i = 0; i < ((8 + dataLen) >> 1) + 1; ++i)
	{
		check += p[i];
	}
	BOGUS bogus;
	bogus.saddr = src;
	bogus.daddr = dest;
	bogus.zero = 0;
	bogus.protocol = IP_PROTO_UDP;
	bogus.len = udp->len;
	p = (uint16_t *)&bogus;
	for (int i = 0; i < 6; ++i)
	{
		check += p[i];
	}
	
	check = (check >> 16) + (check & 0xFFFF);
	check += (check >> 16);
	udp->check = ~check;
	
	sockaddr_in target;
	target.sin_family = AF_INET;
	target.sin_port = dport;
	target.sin_addr.s_addr = dest;
	
	sendto(s, (char*)data, 20 + 8 + dataLen, 0, (sockaddr*)&target, sizeof(target));
	int i = GetLastError();
	return true;
}


int main()
{
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD( 2, 2 );

	if (WSAStartup(wVersionRequested, &wsaData))
	{
		return false;
	}
	
	SOCKET ListenSocket_RAW = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
	int opt = 1;
	setsockopt(ListenSocket_RAW, IPPROTO_IP, IP_HDRINCL, (LPCSTR)&opt, sizeof(opt));
	
	int i = GetLastError();
	
	SendUDP(ListenSocket_RAW, inet_addr("192.168.100.87"), htons(9999), inet_addr("192.168.100.33"), htons(53), 80);
	// SendUDP(ListenSocket_RAW, inet_addr("202.106.0.20"), htons(53), inet_addr("192.168.100.33"), htons(53), 80);
	return 0;
}
