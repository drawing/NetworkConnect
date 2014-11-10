
#include "Mutex.h"
#include "Thread.h"

#include <iostream>

class ThreadTest : public Thread
{
public:
	virtual void Run()
	{
		for (int i = 0; i < 10; ++i)
		{
			for (int j = 0; j < 5; j++)
			{
				m_Sem->Down();
				std::cout << "Down" << std::endl;
			}
		}
	}
public:
	Semaphore * m_Sem;
	uint32_t m_Num;
};

int main()
{
	Semaphore sem(5, 100);
	ThreadTest t1;
	
	t1.m_Sem = &sem;
	
	t1.Start();
	
	for (int i = 0; i < 50; i++)
	{
#ifdef WIN32
		Sleep(1000);
#elif defined( LINUX )
		sleep(1);
#endif
		sem.Up();
		std::cout << "Up" << std::endl;
	}
	
	t1.WaitForShutdown();
	return 0;
}
