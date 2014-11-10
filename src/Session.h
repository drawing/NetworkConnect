#ifndef SESSION_H
#define SESSION_H

#include <map>
#include <list>

#include "Packet.h"
#include "Address.h"
#include "Queue.h"
#include "time.h"

#include <openssl/ssl.h>

struct Session
{
	enum Status { RUNNING, SLEEPED, WORKING, DESTROYED };
	
	SSL * m_SSL;
	
	// 所谓读写，都是针对网卡
	// 从网卡读数据放入读队列，写入网卡的数据位于写队列
	Queue <Packet *> m_ReadQueue;
	Queue <Packet *> m_WriteQueue;
	
	Packet m_TcpBuffer;
	uint32_t m_TcpNowLen;
	
	Status m_Status;
	bool m_bConnected;
	time_t m_LastDataTime;
	
	Address m_RealAddress;
	Address m_VirtualAddress;
	
public:
	Session();
	~Session();
};

struct Context
{
	std::map <Address, Session *> m_RealMap;
	std::map <Address, Session *> m_VirtualMap;
	std::queue <Session *> m_Running;
	std::list <Session *> m_Destroyed;
};

#endif // SESSION_H
