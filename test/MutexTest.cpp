
#include "Mutex.h"
#include "Thread.h"

#include <iostream>

class ThreadTest : public Thread
{
public:
	virtual void Run()
	{
		for (int i = 0; i < 100; ++i)
		{
			m_Lock->Lock();
			std::cout << "Test Start " << m_Num << std::endl;
#ifdef WIN32
			Sleep(1000);
#elif defined( LINUX )
			sleep(1);
#endif
			std::cout << "Test End " << m_Num << std::endl;
			m_Lock->Unlock();
		}
	}
public:
	Mutex * m_Lock;
	uint32_t m_Num;
};

int main()
{
	Mutex lock;
	ThreadTest t1, t2;
	
	t1.m_Num = 1;
	t2.m_Num = 2;
	
	t1.m_Lock = &lock;
	t2.m_Lock = &lock;
	
	t1.Start();
	t2.Start();
	
	t1.WaitForShutdown();
	t2.WaitForShutdown();
	
	return 0;
}
