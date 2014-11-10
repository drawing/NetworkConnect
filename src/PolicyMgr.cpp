#include "Config.h"

#include "PolicyMgr.h"

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

PolicyMgr * PolicyMgr::m_pPolicyMgr = 0;

void PolicyMgr::SetPolicyMgr(PolicyMgr * pPolicyMgr)
{
	// 暂时不考虑线程安全，以后有需要需设置线程锁
	if (m_pPolicyMgr != NULL) {
		delete m_pPolicyMgr;
	}
	m_pPolicyMgr = pPolicyMgr;
}

bool PolicyMgr::AllocAddr(const std::string & user, uint32_t * address, uint32_t * mask)
{
	uint32_t addr = 0;
	
	static uint32_t base = 0x0A212102;
	
	m_Mutex.Lock();
	
	size_t i;
	for (i = 0; i < m_AddrPool.size(); ++i)
	{
		if (!m_AddrPool[i])
		{
			m_AddrPool[i] = true;
			break;
		}
	}
	if (i == m_AddrPool.size())
	{
		m_AddrPool.push_back(true);
	}
	addr = base + i;
	
	m_Mutex.Unlock();
	
	*address = htonl(addr);
	*mask = inet_addr("255.255.255.0");
	
	return true;
}

void PolicyMgr::FreeAddr(uint32_t addr)
{
	static uint32_t base = 0x0A212102;
	
	addr = ntohl(addr);
	m_Mutex.Lock();
	if (addr - base < m_AddrPool.size())
	{
		m_AddrPool[addr - base] = false;
	}
	m_Mutex.Unlock();
}

uint32_t PolicyMgr::GetProtect(const std::string & user, Subnet * items)
{
	uint32_t count = 2;

	items[0].Address = inet_addr("192.168.33.0");
	items[0].Mask = inet_addr("255.255.255.0");

	items[1].Address = inet_addr("192.168.10.0");
	items[1].Mask = inet_addr("255.255.255.0");

	return count;
}

uint32_t PolicyMgr::GetGateway(const std::string & user, Subnet * items)
{
	uint32_t count = 1;
	if (user == "")
	{
		// 得到所有的网关地址
		items[0].Address = inet_addr("10.33.33.1");
		items[0].Mask = inet_addr("255.255.255.0");
	}
	else
	{
		// 得到相关用户名的网关
		items[0].Address = inet_addr("10.0.0.2");
		items[0].Mask = inet_addr("255.255.255.0");
	}
	return count;
}

