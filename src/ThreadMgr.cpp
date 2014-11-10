#include "Config.h"
#include "ThreadMgr.h"

#include "SessionMgr.h"
#include "Instruction.h"
#include "TunHandler.h"
#include "PolicyMgr.h"
#include "Options.h"

#include "Util.h"

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	
	#include <netinet/tcp.h>
#endif

ThreadMgr::ThreadMgr()
{}

ThreadMgr::~ThreadMgr()
{
	m_Tun.Stop();
	m_Sock.Stop();
	for (std::list<WorkThread *>::iterator i = m_WorkList.begin();
		i != m_WorkList.end();
		++i)
	{
		(*i)->Stop();
		delete *i;
	}
}

bool ThreadMgr::StartNewThread(ThreadType type, void * Parameter)
{
	WorkThread * work;
	switch (type)
	{
	case TUN_THREAD:
		m_Tun.Start();
		break;
	case SOCK_THREAD:
		m_Sock.Start();
		break;
	case WORK_THREAD:
		work = new WorkThread;
		work->Start();
		m_WorkList.push_back(work);
		break;
	case INSTRUCT_THREAD:
		m_Inst.Start();
		break;
	default:
		assert(0);
		break;
	}
	return true;
}

bool ThreadMgr::Login(const Address & Server, const std::string & Username,
		const std::string & Passwd)
{
	Packet * packet = new Packet;
	SessionMgr * Mgr = SessionMgr::Instance();
	// 构造登陆指令包

	if (!Mgr->InitSSL())
	{
		return false;
	}
	
	const SSL * TempSSL = Mgr->CreateGateway(Server);
	if (TempSSL == NULL)
	{
		goto Error_Exit;
	}
	
	if (!m_Inst.Login(Server, Username, Passwd))
	{
		goto Error_Exit;
	}

	// 启动 SockRouter 线程
	m_Sock.SetSock(SSL_get_fd(TempSSL));
	m_Sock.Start();
	
	return true;

Error_Exit:
	if (packet)
	{
		delete packet;
	}
	return false;
}

bool ThreadMgr::Listen(const Address & Server)
{
	int err;
	SessionMgr * Mgr = SessionMgr::Instance();
	TunHandler * Handle = TunHandler::Instance();
	PolicyMgr * policy = (PolicyMgr *)PolicyMgr::Instance();
	Options * options = Options::Instance();
	
	if (!Mgr->InitSSL())
	{
		return false;
	}
	
	int sock;
	if (options->UDPMode)
	{
		sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
	}
	else
	{
		sock = (int)socket(AF_INET, SOCK_STREAM, 0);
		
		// 设置 socket 为不延时方式，否则效率极低
		uint32_t flag = 1;
		if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) == -1)
		{
			perror("setsockopt TCP_NODELAY");
		}
		/*
		flag = 2048;
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&flag, sizeof(flag)))
		{
			perror("setsockopt");
		}
		*/
	}

	if (sock == -1)
	{
		return false;
	}
	
	sockaddr_in dst;
	dst.sin_family = AF_INET;
	dst.sin_port = Server.m_Port;
	dst.sin_addr.s_addr = Server.m_IPv4;
	
	err = bind(sock, (sockaddr*)&dst, sizeof dst);
	if (err == -1)
	{
		closesocket(sock);
		return false;
	}
	
	if (!options->UDPMode)
	{
		listen(sock, MAX_LISTEN_SIZE);
	}
	
	// 启动 SockRouter 线程
	m_Sock.SetSock(sock);
	m_Sock.Start();
	
	Mgr->SetSock(sock);
	
	// 得到网关策略
	Subnet gateways[MAX_GATEWAY_COUNT];
	uint32_t count = policy->GetGateway("", gateways);
	// 检测count值
	
	in_addr addr;
	
	addr.s_addr = gateways[0].Address;
	std::string Address = inet_ntoa(addr);
	
	addr.s_addr = gateways[0].Mask;
	std::string Mask = inet_ntoa(addr);
	
	// 启动Tun网卡
	if (!Handle->Start(Address, Mask))
	{
		fprintf(stderr, "start tun/tap driver failed\n");
		return false;
	}
	
	for (unsigned i = 1; i < count; ++i)
	{
		addr.s_addr = gateways[i].Address;
		Address = inet_ntoa(addr);
		
		addr.s_addr = gateways[i].Mask;
		Mask = inet_ntoa(addr);
	
		Handle->AddIP(Address, Mask);
	}
	
	// 启动Tun监视线程
	if (!StartNewThread(ThreadMgr::TUN_THREAD))
	{
		// 启动线程失败
		closesocket(sock);
		return false;
	}

	return true;
}
