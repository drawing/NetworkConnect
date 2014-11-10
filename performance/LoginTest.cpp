
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

int main()
{
	for (int i = 0; i < 3000; ++i)
	{
		SessionMgr * mgr = SessionMgr::Instance();
		mgr->InitSSL();
		
		Address Server;
		Server.m_IPv4 = inet_addr("192.168.100.38");
		Server.m_Port = htons(9988);
		
		SSL * TempSSL = const_cast<SSL*>(mgr->CreateGateway(Server));
		
		Packet packet;
		char buffer[1024];
		sprintf(buffer, "login %d", i);
		if (!BuildLoginRequest(&packet, buffer, "123", 12))
		{
			printf("Error for build request\n");
			exit(1);
		}
		
		printf("write pre\n");
		SSL_write(TempSSL, packet.Buffer, packet.Len);
		printf("read pre\n");
		packet.Len = SSL_read(TempSSL, packet.Buffer, sizeof packet.Buffer);
		
		LoginRespond * respond = (LoginRespond *) (packet.Buffer + sizeof (InstPacket));
		
		// 测试日志
		in_addr addr;
		addr.s_addr = respond->TunAddr;
		// 测试日志 End
		
		printf("login %d : %s\n", i, inet_ntoa(addr));
	}
	return 0;
}
