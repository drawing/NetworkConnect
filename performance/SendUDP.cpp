#include "Config.h"
#include "Proto.h"


#include <stdint.h>
#include <SessionMgr.h>
#include <Instruction.h>
#include <Options.h>

#include "Util.h"

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#define MAX_DATA 2048


bool SendUDP(
	SSL * ssl,
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
	ip->id = base ++;
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
	
	SSL_write(ssl, data, 20 + 8 + dataLen);
	
	return true;
}


DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	SessionMgr * mgr = SessionMgr::Instance();
	
	Options * options = Options::Instance();
	options->UDPMode = true;
	options->ServerMode = false;
	
	Address Server;
	Server.m_IPv4 = inet_addr("192.168.100.38");
	Server.m_Port = htons(9988);
	
	mgr->InitSSL();
	
	SSL * TempSSL = const_cast<SSL*>(mgr->CreateGateway(Server));
	
	Packet packet;
	if (!BuildLoginRequest(&packet, "test", "123", 12))
	{
		printf("Error for build request\n");
		exit(1);
	}
	
	SSL_write(TempSSL, packet.Buffer, packet.Len);
	packet.Len = SSL_read(TempSSL, packet.Buffer, sizeof packet.Buffer);
	
	LoginRespond * respond = (LoginRespond *) (packet.Buffer + sizeof (InstPacket));
	
	while (true)
	{
		// SecondSleep(1);
		// Sleep(100);
		SendUDP(TempSSL, inet_addr("192.168.10.33"), htons(9999), respond->TunAddr, htons(9999), 1000);
	}
	
	return 0;
}


int main()
{
	ThreadProc(0);
	for (int i = 0; i < 1; ++i)
	{
		CreateThread(0, 0, ThreadProc, 0, 0, 0);
	}
	SecondSleep(0xFFFF);
}
