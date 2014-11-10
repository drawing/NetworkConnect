#include "Config.h"
#include "Thread.h"

#include <iostream>

class ThreadTest : public Thread
{
public:
	virtual void Run()
	{
		for (int i = 0; i < 100; ++i)
		{
			std::cout << "Test " << m_Num << std::endl;
		}
	}
public:
	uint32_t m_Num;
};

int main()
{
	ThreadTest t1, t2, t3, t4;
	t1.m_Num = 1;
	t2.m_Num = 2;
	t3.m_Num = 3;
	t4.m_Num = 4;
	
	t1.Start();
	t2.Start();
	t3.Start();
	t4.Start();
	
	t1.WaitForShutdown();
	t2.WaitForShutdown();
	t3.WaitForShutdown();
	t4.WaitForShutdown();
	
	return 0;
}
