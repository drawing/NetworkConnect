#include "Config.h"
#include "Thread.h"

#ifdef WIN32
DWORD Thread::StartRoutine(LPVOID lpParameter)
#else
void * Thread::StartRoutine(void * lpParameter)
#endif
{
	Thread * pThread = (Thread *)lpParameter;
	pThread->Run();
	return 0;
}

void Thread::Start()
{
#ifdef WIN32
	m_Thread = CreateThread(NULL, NULL, StartRoutine, this, 0, 0);
#else
	pthread_create(&m_Thread, 0, StartRoutine, this);
#endif
}

void Thread::WaitForShutdown()
{
#ifdef WIN32
	WaitForSingleObject(m_Thread, INFINITE);
#else
	void * ExitValue;
	pthread_join(m_Thread, &ExitValue);
#endif
}

void Thread::Stop()
{
}
