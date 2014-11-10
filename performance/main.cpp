#include <stdio.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib") 

int main()
{
	WSADATA wsaData;
	WORD wSockVersion = MAKEWORD(2,0);
	::WSAStartup(wSockVersion,&wsaData);

	SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)
	{
		printf("socket fail\n");
		::WSACleanup();
		return 0;
	}

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(9999);
	sin.sin_addr.S_un.S_addr = inet_addr("192.168.10.33");
	if (::bind(s, (SOCKADDR*)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("bind fail\n");
		::WSACleanup();
		return 0;
	}

	sockaddr caddr;
	int clen = sizeof(caddr);
	memset(&caddr, 0, sizeof(caddr));

	char buf[1000] = {};
	unsigned cnt = 0;
	float res = 0.0;
	time_t last = time(0);
	int datalen = 0;
	while (true)
	{
		datalen = ::recvfrom(s, buf, 1000, 0, (SOCKADDR*)&caddr, &clen);
		if (datalen == SOCKET_ERROR)
		{
			printf("recvfrom error\n");
			int i = ::GetLastError();
			printf("%d\n", i);
			break;
		}
		cnt += datalen;
			
		time_t now = time(0);
		if (now != last)
		{
			last = now;
			res = cnt/1024.0/1024.0;
			printf("%0.3f M/S\n", res);
			cnt = 0;
		}
	}

	return 0;
}