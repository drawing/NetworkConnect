#include "PolicyMgr.h"

#ifdef LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif // LINUX
#include <iostream>

int main()
{
	PolicyMgr * policy = (PolicyMgr *)PolicyMgr::Instance();
	for (int i = 0; i < 10; ++i)
	{
		in_addr addr;
		/*
		addr.s_addr = policy->AllocAddr("", "", "");
		std::cout << inet_ntoa(addr) << std::endl;
		*/
	}
	return 0;
}
