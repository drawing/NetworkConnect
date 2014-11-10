#include "Config.h"
#include "RouteSockThread.h"
#include "SessionMgr.h"

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	
	#include <sys/epoll.h>
	
	#include <unistd.h>
	#include <fcntl.h>
#endif
#include "Util.h"
#include "Options.h"

RouteSockThread::RouteSockThread() : m_Running(true)
{}

void RouteSockThread::Run()
{
	if (Options::Instance()->UDPMode)
	{
		ExecUDP();
	}
	else
	{
		if (Options::Instance()->ServerMode)
		{
#ifdef LINUX
			ExecEpoll();
#else
			ExecIOCP();
#endif
		}
		else
		{
			ExecTcp();
		}
	}
}

void RouteSockThread::Stop()
{
	m_Running = false;
}

void RouteSockThread::SetSock(int Sock)
{
	m_Sock = Sock;
}

void RouteSockThread::ExecUDP()
{
	SessionMgr * Mgr = SessionMgr::Instance();

	sockaddr_in from;
	socklen_t fromlen;
	Address address;
	Packet * p;
	
	while (m_Running)
	{
		p = new Packet;
		// 读真实网卡数据
		fromlen = sizeof from;
		p->Len = recvfrom(m_Sock, (char*)p->Buffer, MAX_PACKET_BUFFER, 0,
				(sockaddr*)&from, &fromlen);
		if (p->Len > 0)
		{
			address.m_IPv4 = from.sin_addr.s_addr;
			address.m_Port = from.sin_port;
			// Dispatch 到 Session
			if (Mgr->DispatchPacket(p, address, Address::ADDR_REAL))
			{
				continue;
			}
		}
		// test *
		else
		{
			// printf("%d\n", GetLastError());
			m_Running = false;
#ifdef WIN32
			printf("recvfrom failed with errno : %d\n", GetLastError());
#else
			perror("recvfrom");
#endif
			assert(0);
		}
		//*/
		delete p;
	}
}

#ifdef LINUX
bool RouteSockThread::setNonblocking(int sock)
{
	int opt = fcntl(sock, F_GETFL);
	opt |= O_NONBLOCK;

	if (fcntl(sock, F_SETFL, opt) < 0)
	{
		return false;
	}
	return true;
}

void RouteSockThread::ExecEpoll()
{
	int epfd = epoll_create(MAX_EPOLL_SIZE);
	
	struct epoll_event eventCache;
	
	Address * userData = new Address;
	
	setNonblocking(m_Sock);
	userData->m_Sock = m_Sock;
	eventCache.data.ptr = userData;
	eventCache.events = EPOLLIN | EPOLLET;
	epoll_ctl(epfd, EPOLL_CTL_ADD, m_Sock, &eventCache);
	
	struct epoll_event eventQueue[MAX_EPOLL_SIZE + 1];
	int clientNum = 1;
	
	Packet * p;
	SessionMgr * Mgr = SessionMgr::Instance();
	
	while (m_Running)
	{
		int ActiveSockNum = epoll_wait(epfd, eventQueue, MAX_EPOLL_SIZE, -1);
		for (int i = 0; i < ActiveSockNum; ++i)
		{
			userData = (Address *)eventQueue[i].data.ptr;
			int ActiveSock = userData->m_Sock;
			if (ActiveSock == m_Sock)
			{
				// 有新连接
				sockaddr_in from;
				socklen_t fromlen = sizeof from;
				ActiveSock = accept(m_Sock, (sockaddr *)&from, &fromlen);
				
				eventCache.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
				userData = new Address;
				
				setNonblocking(ActiveSock);
				userData->m_Sock = ActiveSock;
				userData->m_IPv4 = from.sin_addr.s_addr;
				userData->m_Port = from.sin_port;
				eventCache.data.ptr = userData;
				
				epoll_ctl(epfd, EPOLL_CTL_ADD, ActiveSock, &eventCache);
				clientNum ++;
				
				// 测试打印
				printf("new socket %d\n", ActiveSock);
			}
			else
			{
				// 连接有数据接收
				p = new Packet;
				// ET 问题
				p->Len = read(ActiveSock, p->Buffer, MAX_PACKET_BUFFER);
				// 测试打印
				// printf("socket read %d len %d\n", ActiveSock, p->Len);
				// 结束测试
				if (p->Len <= 0)
				{
					// 连接断开，清理资源
					epoll_ctl(epfd, EPOLL_CTL_DEL, ActiveSock, 0);
					delete userData;
					clientNum --;
					// 测试打印
					printf("delete user %d\n", ActiveSock);
					// 结束测试
				}
				else
				{
					if (Mgr->DispatchPacket(p, *userData, Address::ADDR_REAL))
					{
						continue;
					}
				}
				delete p;
			}
		}
	}
	
	close(epfd);
}
#endif // LINUX

void RouteSockThread::ExecTcp()
{
	/**
	* TCP 只做客户端使用
	* 不考虑TCP打洞情况，这里仅实现单连接情况
	*/
	SessionMgr * Mgr = SessionMgr::Instance();
	Options * options = Options::Instance();

	Address server;
	server.m_IPv4 = inet_addr(options->ServerAddress.c_str());
	server.m_Port = htons(atoi(options->ServerPort.c_str()));
	server.m_Sock = m_Sock;
	
	Packet * p;

	while (m_Running)
	{
		p = new Packet;
		// 读真实网卡数据
		p->Len = recv(m_Sock, (char*)p->Buffer, MAX_PACKET_BUFFER, 0);
		if (p->Len > 0)
		{
			// Dispatch 到服务器 Session
			if (Mgr->DispatchPacket(p, server, Address::ADDR_REAL))
			{
				continue;
			}
		}
		// test *
		else
		{
			// printf("%d\n", GetLastError());
			m_Running = false;
			assert(0);
		}
		//*/
		delete p;
	}
}

#ifdef WIN32

struct IOCPData
{
	Address target;
	Packet * packet;
	WSABUF buffer;
	OVERLAPPED overlapped;
};
	
DWORD WINAPI IoCompletionRouter(LPVOID lpParameter)
{
	SessionMgr * Mgr = SessionMgr::Instance();
	HANDLE CompletionPort = lpParameter;
	OVERLAPPED * pOverlapped;
	DWORD Flags = 0;

	IOCPData * pUser;
	DWORD BytesTransferred;

	while (true)
	{
		pOverlapped = NULL;
		if (!GetQueuedCompletionStatus(
			CompletionPort,
			&BytesTransferred,
			(PULONG_PTR)&pUser,
			&pOverlapped,
			INFINITE))
		{
			if (pOverlapped != NULL)
			{
				// 释放客户端
				printf("free client %d\n", pUser->target.m_Sock);
				closesocket(pUser->target.m_Sock);
				delete pUser->packet;
				delete pUser;
				continue;
			}
			else
			{
				printf("GetQueuedCompletionStatus failed with error %d\n", GetLastError());
				return -1;
			}
		}
		if (BytesTransferred <= 0)
		{
			printf("client is closed %d\n", pUser->target.m_Sock);
			closesocket(pUser->target.m_Sock);
			delete pUser->packet;
			delete pUser;
		}
		else
		{
			// 获取到数据
			pUser->packet->Len = BytesTransferred;
			
			// Dispatch 到服务器 Session
			if (Mgr->DispatchPacket(pUser->packet, pUser->target, Address::ADDR_REAL))
			{
				pUser->packet = new Packet;
			}
			
			ZeroMemory(&pUser->overlapped, sizeof(OVERLAPPED));
			Flags = 0;
			pUser->buffer.len = MAX_PACKET_BUFFER;
			pUser->buffer.buf = (char*)pUser->packet->Buffer;
			WSARecv(pUser->target.m_Sock, &pUser->buffer, 1, 0, &Flags, &pUser->overlapped, 0);
		}
	}

	return 0;
}

void RouteSockThread::ExecIOCP()
{
	HANDLE CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (CompletionPort == NULL)
	{
		printf("CreateIoCompletionPort: %d\n", GetLastError());
		return;
	}
	
	// 创建线程
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	for (int i = 0; i < SystemInfo.dwNumberOfProcessors * 2; ++i)
	{
		HANDLE ThreadHandle = CreateThread(NULL, 0, IoCompletionRouter, CompletionPort, 0, 0);
		CloseHandle(ThreadHandle);
	}
	
	while (m_Running)
	{
		sockaddr_in from;
		socklen_t fromlen = sizeof from;
		SOCKET ActiveSock = accept(m_Sock, (sockaddr *)&from, &fromlen);

		IOCPData * pUser = new IOCPData;
		pUser->target.m_Sock = ActiveSock;
		pUser->target.m_IPv4 = from.sin_addr.s_addr;
		pUser->target.m_Port = from.sin_port;
		pUser->packet = new Packet;
		pUser->buffer.len = MAX_PACKET_BUFFER;
		pUser->buffer.buf = (char*)pUser->packet->Buffer;
		
		ZeroMemory(&pUser->overlapped, sizeof(OVERLAPPED));

		if (CreateIoCompletionPort((HANDLE)ActiveSock, CompletionPort, (ULONG_PTR)pUser, 0) == NULL)
		{
			printf("CreateIoCompletionPort: %d\n", GetLastError());
		}
		
		DWORD Flags = 0;
		WSARecv(ActiveSock, &pUser->buffer, 1, 0, &Flags, &pUser->overlapped, 0);
	}
}
#endif //WIN32

