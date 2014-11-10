#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#ifdef WIN32
	#include <windows.h>
#else
	#include <pthread.h>
#endif

/**
* 线程基类，实现类仅需实现Run函数，调用Start启动线程
*/
class Thread
{
private:
#ifdef WIN32
	static DWORD __stdcall StartRoutine(LPVOID lpParameter);
#else
	static void * StartRoutine(void * lpParameter);
#endif
public:
	// 开启线程
	void Start();
	// 等待线程结束
	void WaitForShutdown();
	// 子类实现操作
	virtual void Run() = 0;
	// 停止，默认杀线程，子类可更为优雅的实现
	virtual void Stop();
	
	virtual ~Thread() {}
private:
	// 线程句柄
#ifdef WIN32
	HANDLE m_Thread;
#else
	pthread_t m_Thread;
#endif
};

#endif // THREAD_H
