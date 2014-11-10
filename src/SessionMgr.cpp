#include "Config.h"
#include "SessionMgr.h"

#include "reentrant.h"
#include "Options.h"
#include "Util.h"
#include "PolicyMgr.h"

#include <openssl/err.h>

#ifdef WIN32
	// Win32 环境若不加此文件
	// 可能出现 no OPENSSL_Applink 错误
	// #include <openssl/applink.c>
#endif

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	
	#include <netinet/tcp.h>
#endif

SessionMgr::SessionMgr() : m_RunningSemaphore(0, 0x7FFFFFFF),
	m_SSLCTX(NULL), m_Gateway(NULL), m_SelfSock(-1)
{}

SessionMgr::~SessionMgr()
{
	if (m_SSLCTX)
	{
		SSL_CTX_free(m_SSLCTX);
	}
}

SessionMgr * SessionMgr::Instance()
{
	// C++ 不保证静态局部变量线程安全
	static SessionMgr manager;
	return &manager;
}

// 分发数据包
bool SessionMgr::DispatchPacket(Packet * packet, Address address, Address::AddressType type)
{
	// 查找目标连接
	Session * pItem = NULL;
	bool bRetValue = true;
	BIO * wbio;
	sockaddr_in srcAddr;
	Options * optinos = Options::Instance();
	
	m_Mutex.Lock();
	
	switch (type)
	{
	case Address::ADDR_VIRTUAL:
		pItem = m_Context.m_VirtualMap[address];
		if (pItem)
		{
			pItem->m_WriteQueue.Push(packet);
		}
		else if (m_Gateway)
		{
			// 没有所对应的 Session，分发给网关Session
			pItem = m_Gateway;
			pItem->m_WriteQueue.Push(packet);
		}
		break;
	case Address::ADDR_REAL:
		pItem = m_Context.m_RealMap[address];
		// 如果没有连接则创建Session
		if (pItem == NULL)
		{
			pItem = new Session;
			pItem->m_SSL = SSL_new(m_SSLCTX);
			SSL_set_accept_state(pItem->m_SSL);
			
			// 创建 BIO
			if (optinos->UDPMode)
			{
				srcAddr.sin_family = AF_INET;
				srcAddr.sin_addr.s_addr = address.m_IPv4;
				srcAddr.sin_port = address.m_Port;
				
				wbio = BIO_new_dgram(m_SelfSock, BIO_NOCLOSE);
				BIO_dgram_set_peer(wbio, &srcAddr);
			}
			else
			{
				wbio = BIO_new_socket(address.m_Sock, BIO_NOCLOSE);
			}
			// 创建 BIO 结束

			SSL_set_bio(pItem->m_SSL, NULL, wbio);
			
			pItem->m_RealAddress = address;
			m_Context.m_RealMap[address] = pItem;
		}
		pItem->m_ReadQueue.Push(packet);
		break;
	case Address::ADDR_LOCAL:
		pItem = m_Context.m_RealMap[address];
		// 如果没有连接则返回错误
		if (pItem == NULL)
		{
			bRetValue = false;
		}
		else
		{
			pItem->m_WriteQueue.Push(packet);
		}
		break;
	default:
		assert(0);
		break;
	}

	if (pItem == NULL)
		goto Err_Exit;
	else
	{
		pItem->m_LastDataTime = time(0);
	}

	// Session 插入运行队列
	// PushRunningItem(pItem);
	if (pItem->m_Status == Session::SLEEPED)
	{
		// printf("dispatch size %d, %d, %x\n", m_Context.m_Running.size(), pItem->m_Status, pItem);
		pItem->m_Status = Session::RUNNING;
		m_Context.m_Running.push(pItem);
		m_RunningSemaphore.Up();
	}
	
Err_Exit:
	m_Mutex.Unlock();
	
	return bRetValue;
}

// pop 运行队列元素
Session * SessionMgr::FetchRunningItem()
{
	// 从运行队列中获取Session，并删除
	Session * pItem = NULL;
	m_RunningSemaphore.Down();
	
	m_Mutex.Lock();
	if (!m_Context.m_Running.empty())
	{
		// printf("fetch size %d\n", m_Context.m_Running.size());
		pItem = m_Context.m_Running.front();
		m_Context.m_Running.pop();
		pItem->m_Status = Session::WORKING;
	}
	else
	{
		m_RunningSemaphore.Up();
	}
	m_Mutex.Unlock();
	
	// 信号不可放入加锁的范围内
	
	return pItem;
}

bool SessionMgr::PushSleepingItem(Session * pItem)
{
	bool bRetValue = true;
	m_Mutex.Lock();
	if (pItem->m_Status == Session::WORKING)
	{
		pItem->m_Status = Session::SLEEPED;
	}
	m_Mutex.Unlock();
	return bRetValue;
}

// push 运行队列元素
bool SessionMgr::PushRunningItem(Session * pItem)
{
	// 把 Session 加入运行队列
	bool bRetValue = true;

	m_Mutex.Lock();
	if (pItem->m_Status == Session::WORKING)
	{
		// printf("push size %d, %d, %x\n", m_Context.m_Running.size(), pItem->m_Status, pItem);
		pItem->m_Status = Session::RUNNING;
		m_Context.m_Running.push(pItem);
		m_RunningSemaphore.Up();
	}
	m_Mutex.Unlock();

	return bRetValue;
}

// 销毁Session
bool SessionMgr::DestroySession(Session * pItem)
{
	bool bRetValue = true;

	m_Mutex.Lock();
	m_Context.m_VirtualMap.erase(pItem->m_VirtualAddress);
	m_Context.m_RealMap.erase(pItem->m_RealAddress);
	
	// 销毁 Session 结构
	pItem->m_Status = Session::DESTROYED;
	
	m_Context.m_Destroyed.push_back(pItem);
	PolicyMgr::Instance()->FreeAddr(pItem->m_VirtualAddress.m_IPv4);
	
	m_Mutex.Unlock();

	return bRetValue;
}

// 初始化 SSL
bool SessionMgr::InitSSL()
{
#ifdef WIN32
	// windows 网络初始化
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD( 2, 2 );

	if (WSAStartup(wVersionRequested, &wsaData))
	{
		return false;
	}
#endif //WIN32

	Options * options = Options::Instance();
	bool bRetValue = false;
	m_Mutex.Lock();

    if (!THREAD_setup() || !SSL_library_init())
    {
        goto ExitFun;
    }
    SSL_load_error_strings();
	
	if (options->ServerMode)
	{
		if (options->UDPMode)
		{
			m_SSLCTX = SSL_CTX_new(DTLSv1_server_method());
		}
		else
		{
			m_SSLCTX = SSL_CTX_new(SSLv23_server_method());
		}
	}
	else
	{
		if (options->UDPMode)
		{
			m_SSLCTX = SSL_CTX_new(DTLSv1_client_method());
		}
		else
		{
			m_SSLCTX = SSL_CTX_new(SSLv23_client_method());
		}
	}
	
	if (m_SSLCTX)
	{
		if (options->ServerMode)
		{
			// SSL 初始化 环境
			if (SSL_CTX_use_certificate_chain_file(m_SSLCTX, "server.pem") != 1)
			{
				ERR_print_errors_fp(stderr);
				goto ExitFun;
			}

			if (SSL_CTX_use_PrivateKey_file(m_SSLCTX, "server.pem", SSL_FILETYPE_PEM) != 1)
			{
				ERR_print_errors_fp(stderr);
				goto ExitFun;
			}
			
			//SSL_CTX_set_cipher_list(m_SSLCTX, "NULL-MD5");
			/*
			if (SSL_CTX_set_cipher_list(m_SSLCTX, "HIGH:MEDIUM:aNULL") != 1)
			{
				ERR_print_errors_fp(stderr);
				goto ExitFun;
			}
			*/
		}
		else
		{
			// SSL 初始化 环境
			SSL_CTX_set_read_ahead(m_SSLCTX, 1);
			SSL_CTX_set_cipher_list(m_SSLCTX, "HIGH:MEDIUM:aNULL");
			// SSL_CTX_set_cipher_list(m_SSLCTX, "NULL-MD5");
			/*
			if (SSL_CTX_use_certificate_chain_file(m_SSLCTX, "client.pem") != 1)
			{
				ERR_print_errors_fp(stderr);
				goto ExitFun;
			}

			if (SSL_CTX_use_PrivateKey_file(m_SSLCTX, "client.pem", SSL_FILETYPE_PEM) != 1)
			{
				ERR_print_errors_fp(stderr);
				goto ExitFun;
			}
			*/
		}
		bRetValue = true;
	}

ExitFun:
	m_Mutex.Unlock();
	return bRetValue;
}

const SSL * SessionMgr::CreateGateway(const Address & target)
{
	Options * options = Options::Instance();
	
	int err;
	sockaddr_in dst;
	int sock = -1;
	
	BIO * TempBIO = NULL;
	SSL * TempSSL = NULL;
	Session * pItem = NULL;
	
	dst.sin_family = AF_INET;
	dst.sin_port = target.m_Port;
	dst.sin_addr.s_addr = target.m_IPv4;

	if (options->UDPMode)
	{
		sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
		TempBIO = BIO_new_dgram(sock, BIO_NOCLOSE);
		if (TempBIO == NULL)
		{
			goto Error_Exit;
		}

		err = BIO_dgram_set_peer(TempBIO, (struct sockaddr*)&dst);
		if (err != 1)
		{
			goto Error_Exit;
		}
	}
	else
	{
		char Server[255] = {};
		sprintf(Server, "%s:%d", inet_ntoa(dst.sin_addr), ntohs(target.m_Port));
		
		TempBIO = BIO_new_connect(Server);
		if (TempBIO == NULL)
		{
			goto Error_Exit;
		}
		if (BIO_do_connect(TempBIO) <= 0)
		{
			// ERR_print_errors_fp(stderr);
			goto Error_Exit;
		}

		sock = BIO_get_fd(TempBIO, 0);
		
		// 设置 socket 为不延时方式，否则对TCP应用效率极低
		uint32_t flag = 1;
		if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) == -1)
		{
			perror("setsockopt");
		}
		
		/*
		flag = 2048;
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&flag, sizeof(flag)))
		{
			perror("setsockopt");
		}
		*/
		
	}
	
	TempSSL = SSL_new(m_SSLCTX);
	if (TempSSL == NULL)
	{
		goto Error_Exit;
	}
	
	SSL_set_bio(TempSSL, TempBIO, TempBIO);
	
	err = SSL_connect(TempSSL);
	if (err != 1)
	{
		ERR_print_errors_fp(stderr);
		goto Error_Exit;
	}
	
	pItem = m_Context.m_RealMap[target];
	// 如果没有连接则创建Session
	if (pItem == NULL)
	{
		pItem = new Session;
		m_Context.m_RealMap[target] = pItem;
		m_Gateway = pItem;
	}
	if (pItem->m_SSL)
	{
		SSL_free(pItem->m_SSL);
		pItem->m_SSL = NULL;
	}
	pItem->m_SSL = TempSSL;
	
	SetSock(sock);

	return TempSSL;
	
Error_Exit:
	if (TempBIO)
	{
		BIO_free(TempBIO);
	}
	if (TempSSL)
	{
		SSL_free(TempSSL);
	}
	if (sock != -1)
	{
		closesocket(sock);
	}
	return NULL;
}

void SessionMgr::SetSock(int sock)
{
	m_SelfSock = sock;
}


// 关联虚拟地址
bool SessionMgr::AssociateVirtual(Session * session, Address & address)
{
	m_Mutex.Lock();
	session->m_VirtualAddress = address;
	m_Context.m_VirtualMap[address] = session;
	m_Mutex.Unlock();
	return true;
}

bool SessionMgr::CleanSessions(time_t past)
{
	time_t now = time(0);
	
	m_Mutex.Lock();
	std::map <Address, Session *>::iterator i = m_Context.m_RealMap.begin();
	for (; i != m_Context.m_RealMap.end(); ++i)
	{
		Session * pItem = i->second;
		if (now - pItem->m_LastDataTime > past &&
			pItem->m_Status == Session::SLEEPED)
		{
			// 间隔 past 没有数据包，则置Session为Destroyed
			m_Context.m_VirtualMap.erase(pItem->m_VirtualAddress);
			m_Context.m_RealMap.erase(pItem->m_RealAddress);
			
			// 销毁 Session 结构
			pItem->m_Status = Session::DESTROYED;
			pItem->m_LastDataTime = now;
			PolicyMgr::Instance()->FreeAddr(pItem->m_VirtualAddress.m_IPv4);
			
			m_Context.m_Destroyed.push_back(pItem);
			
			// 测试日志
			in_addr addr;
			addr.s_addr = pItem->m_VirtualAddress.m_IPv4;
			printf("Disable Session %s\n", inet_ntoa(addr));
			// 测试日志 End
		}
	}
	m_Mutex.Unlock();
	
	m_Mutex.Lock();
	std::list<Session *>::iterator j = m_Context.m_Destroyed.begin();
	while (j != m_Context.m_Destroyed.end())
	{
		Session * pItem = *j;
		if (pItem->m_Status == Session::DESTROYED &&
			now - pItem->m_LastDataTime > FREE_AFTER_DESTROYED)
		{
			// 测试日志
			in_addr addr;
			addr.s_addr = pItem->m_VirtualAddress.m_IPv4;
			printf("Free Session %s\n", inet_ntoa(addr));
			// 测试日志 End
			
			// Destroyed Session 经历一段时间后再销毁
			delete pItem;
			m_Context.m_Destroyed.erase(j++);
		}
		else
		{
			++j;
		}
	}
	m_Mutex.Unlock();
	return true;
}
