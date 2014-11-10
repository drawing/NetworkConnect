#ifndef QUEUE_H
#define QUEUE_H

#include <queue>
#include <cstddef>
#include "Mutex.h"

/**
* 线程安全 队列
*/
template <typename T>
class Queue
{
public:
	bool Push(const T & Item)
	{
		m_Locker.Lock();
		m_Queue.push(Item);
		m_Locker.Unlock();
		return true;
	}

	bool Pop(T & Item)
	{
		bool RetValue = true;
		m_Locker.Lock();
		if (m_Queue.empty())
		{
			RetValue = false;
		}
		else
		{
			Item = m_Queue.front();
			m_Queue.pop();
		}
		m_Locker.Unlock();
		return RetValue;
	}
	
	size_t Size()
	{
		size_t RetValue = 0;
		m_Locker.Lock();
		RetValue = m_Queue.size();
		m_Locker.Unlock();
		return RetValue;
	}
	
	bool Empty()
	{
		bool RetValue = true;
		m_Locker.Lock();
		RetValue = m_Queue.empty();
		m_Locker.Unlock();
		return RetValue;
	}
	
	bool Clear()
	{
		m_Locker.Lock();
		m_Queue.clear();
		m_Locker.Unlock();
		return true;
	}
private:
	std::queue<T> m_Queue;
	Mutex m_Locker;
};

#endif // QUEUE_H
