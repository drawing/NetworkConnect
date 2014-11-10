#include "Config.h"
#include "Options.h"

// 改头文件只存在 MinGW 和 GCC 中，若需要VC编译，需增加相同功能模块
#include <getopt.h>
#include <stdio.h>

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

struct option opts[] = 
{
{"udp", no_argument, NULL, 'u'},
{"tcp", no_argument, NULL, 't'},
{"server", no_argument, NULL, 's'},
{"client", no_argument, NULL, 'c'},
{"address", required_argument, NULL, 'a'},
{"port", required_argument, NULL, 'p'},
{"control", required_argument, NULL, 'n'},
{"id", required_argument, NULL, 'i'},
{"help", no_argument, NULL, 'h'},
{0, 0, 0, 0}
};



Options::Options() : UDPMode(true), ServerMode(false)
{}

void Options::Usage()
{
	printf("Usage: %s <options>\n", ExecName.c_str());
	printf("\tThe options are:\n");
	printf("\t\t--udp				UDP Mode (default)\n");
	printf("\t\t--tcp				TCP Mode\n");
	printf("\t\t--server			Server Mode\n");
	printf("\t\t--client			Client Mode (default)\n");
	printf("\t\t--address *.*.*.*		IP Address\n");
	printf("\t\t--port *			Port\n");
	printf("\t\t--control *.*.*.*:*		Control Address\n");
	printf("\t\t--id *				Login ID (client)\n");
	printf("\t\t--help				Print Usage\n");
}

void Options::Show()
{
	printf("UDPMode			%d\n", UDPMode);
	printf("ServerMode		%d\n", ServerMode);
	printf("ServerAddress	%s\n", ServerAddress.c_str());
	printf("ServerPort		%s\n", ServerPort.c_str());
}

bool Options::ParseCommandLine(int argc, char ** argv)
{
	assert(argc > 0);
	assert(argv);
	assert(*argv);
	
	int oc;
	ExecName = *argv;
	bool bRetValue = true;
	while ((oc = getopt_long(argc, argv, "", opts, 0)) != -1)
	{
		switch (oc)
		{
		case 'u':
			UDPMode = true;
			break;
		case 't':
			UDPMode = false;
			break;
		case 's':
			ServerMode = true;
			break;
		case 'c':
			ServerMode = false;
			break;
		case 'n':
			ControlAddress = optarg;
			ControlPort = ControlAddress.substr(ControlAddress.find(':')+1);
			ControlAddress = ControlAddress.substr(0, ControlAddress.find(':'));
			// printf("control %s:%s", ControlAddress.c_str(), ControlPort.c_str());
			break;
		case 'a':
			if (inet_addr(optarg) == INADDR_NONE)
			{
				bRetValue = false;
			}
			ServerAddress = optarg;
			break;
		case 'p':
			// 应该校验 Port 是否为正常端口范围
			ServerPort = optarg;
			break;
		case 'i':
			LoginID = optarg;
			break;
		case 'h':
			bRetValue = false;
			break;
		case '?':
			// 未知选项
			bRetValue = false;
			break;
		}
	}
	
	if (!bRetValue)
	{
		Usage();
	}
	else if (ServerAddress == "" || ServerPort == "")
	{
		printf("must set address and port\n");
		return false;
	}
	else if (ServerMode && (ControlAddress == "" || ControlPort == ""))
	{
		printf("must set control address\n");
		return false;
	}
	return bRetValue;
}
