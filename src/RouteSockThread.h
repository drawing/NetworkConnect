#ifndef ROUTE_SOCK_THREAD_H
#define ROUTE_SOCK_THREAD_H

#include "Thread.h"
#include "Address.h"
#include "Packet.h"

/**
* 读取Socket数据包，存入队列
*/
class RouteSockThread : public Thread
{
public:
	RouteSockThread();
	void Run();
	void Stop();
	void SetSock(int Sock);
	
private:
	void ExecUDP();

	// TCP 网络模型
private:
#ifdef LINUX
	bool setNonblocking(int sock);
	void ExecEpoll();
#else
	// Windows 完成端口服务器
	void ExecIOCP();
#endif

	// TCP 客户端执行
	void ExecTcp();

private:
	bool m_Running;
#ifdef WIN32
	SOCKET m_Sock;
#else
	int m_Sock;
#endif
};

#endif // ROUTE_SOCK_THREAD_H
