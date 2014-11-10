#include "Config.h"
#include "Mutex.h"

Mutex::Mutex(void)
{
#ifdef WIN32
	InitializeCriticalSection(&m_Mutex);
#elif defined( LINUX )
	pthread_mutex_init(&m_Mutex, NULL);
#endif
}

Mutex::~Mutex(void)
{
#ifdef WIN32
	DeleteCriticalSection(&m_Mutex);
#elif defined( LINUX )
	pthread_mutex_destroy(&m_Mutex);
#endif
}

void Mutex::Lock()
{
#ifdef WIN32
	EnterCriticalSection(&m_Mutex);
#elif defined( LINUX )
	pthread_mutex_lock(&m_Mutex);
#endif
}

void Mutex::Unlock()
{
#ifdef WIN32
	LeaveCriticalSection(&m_Mutex);
#elif defined( LINUX )
	pthread_mutex_unlock(&m_Mutex);
#endif
}


Semaphore::Semaphore(uint32_t InitialCount, uint32_t MaxCount)
{
#ifdef WIN32
	m_Semaphore = CreateSemaphore(NULL, InitialCount, MaxCount, NULL);
#elif defined( LINUX )
	sem_init(&m_Semaphore, 0, InitialCount);
#endif
}

Semaphore::~Semaphore()
{
#ifdef WIN32
	CloseHandle(m_Semaphore);
	m_Semaphore = NULL;
#elif defined( LINUX )
	sem_destroy(&m_Semaphore);
#endif
}

void Semaphore::Up(void)
{
#ifdef WIN32
	LONG PreCount = 0;
	ReleaseSemaphore(m_Semaphore, 1, &PreCount);
#elif defined( LINUX )
	sem_post(&m_Semaphore);
#endif
}

void Semaphore::Down(void)
{
#ifdef WIN32
	WaitForSingleObject(m_Semaphore, INFINITE);
#elif defined( LINUX )
	sem_wait(&m_Semaphore);
#endif
}


