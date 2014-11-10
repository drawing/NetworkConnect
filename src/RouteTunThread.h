#ifndef ROUTE_TUN_THREAD_H
#define ROUTE_TUN_THREAD_H

#include "Thread.h"

/**
* 读取Tun驱动数据包，存入队列
*/
class RouteTunThread : public Thread
{
public:
	RouteTunThread();
	void Run();
	void Stop();
private:
	bool m_Running;
};

#endif // ROUTE_TUN_THREAD_H
