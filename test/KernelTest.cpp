#include "Config.h"

#include "ThreadMgr.h"
#include "Options.h"
#include "Util.h"
#include "TokenPolicyMgr.h"

#include "SessionMgr.h"

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

/**
 * 启动方式：
 * sudo ./kernel --server --udp --port 30000 --address 0.0.0.0 --control 127.0.0.1:9900
 * kernel --client --udp --port 30000 --address 192.168.1.6 --id 2211
 */

int main(int argc, char ** argv)
{
	Options * options = Options::Instance();
	
	options->ServerAddress = "0.0.0.0";
	options->ServerPort = "9988";
	options->ServerMode = true;
	options->UDPMode = false;
	
	if (!options->ParseCommandLine(argc, argv))
	{
		return EXIT_FAILURE;
	}

	ThreadMgr * thread_mgr = ThreadMgr::Instance();
	
	if (!thread_mgr->StartNewThread(ThreadMgr::INSTRUCT_THREAD))
	{
		return EXIT_FAILURE;
	}
	
	for (int i = 0; i < WORK_THREAD_NUMS; ++i)
	{
		thread_mgr->StartNewThread(ThreadMgr::WORK_THREAD);
	}

	Address address;
	address.m_IPv4 = inet_addr(options->ServerAddress.c_str());
	address.m_Port = htons(atoi(options->ServerPort.c_str()));
	if (options->ServerMode)
	{
		Address PolicySAddress;
		Address PolicyTAddress;
		PolicyMgr * pPolicy = new TokenPolicyMgr(PolicySAddress, PolicyTAddress);
		if (!pPolicy->Init()) {
			printf("PolicyMgr Init Failed\n");
			exit(-2);
		}
		PolicyMgr::SetPolicyMgr(pPolicy);

		if (!thread_mgr->Listen(address))
		{
			printf("Listen Failed\n");
			exit(1);
		}
	}
	else
	{
		if (!thread_mgr->Login(address, options->LoginID, "pass"))
		{
			printf("Login Failed\n");
			exit(1);
		}
	}

	while (true)
	{
		char op;
		scanf("%c", &op);
		if (!isspace(op))
		{
			printf("clean\n");
			SessionMgr * Mgr = SessionMgr::Instance();
			Mgr->CleanSessions(5);
		}
	}

	// TestEnd
	SecondSleep(0xFFFFF);
	
	return EXIT_SUCCESS;
}
