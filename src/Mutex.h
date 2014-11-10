#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>
#ifdef WIN32
	#include <windows.h>
#elif defined( LINUX )
	#include <pthread.h>
	#include <semaphore.h>
#endif

/**
* 跨平台线程间互斥结构
*/
class Mutex
{
public:
	Mutex(void);
	~Mutex(void);

public:
	void Lock();
	void Unlock();
	
private:
#ifdef WIN32
	CRITICAL_SECTION m_Mutex;
#elif defined( LINUX )
	pthread_mutex_t m_Mutex;
#endif
};

/**
* 跨平台信号量封装，供线程同步之用
*/
class Semaphore
{
public:
	Semaphore(uint32_t InitialCount, uint32_t MaxCount);
	~Semaphore();
public:
	void Up(void);
	void Down(void);

private:
#ifdef WIN32
	HANDLE m_Semaphore;
#elif defined( LINUX )
	sem_t m_Semaphore;
#endif
};


#endif // MUTEX_H
