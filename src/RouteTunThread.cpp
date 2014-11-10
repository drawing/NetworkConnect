#include "Config.h"
#include "RouteTunThread.h"
#include "TunHandler.h"
#include "SessionMgr.h"
#include "Proto.h"

RouteTunThread::RouteTunThread() : m_Running(true)
{}

void RouteTunThread::Run()
{
	TunHandler * Handle = TunHandler::Instance();
	SessionMgr * Mgr = SessionMgr::Instance();
	Address address;
	address.m_Port = 0;
	Packet * p;
	while (m_Running)
	{
		p = new Packet;
		// 读虚拟网卡数据
		if (Handle->Read(*p))
		{
			// 设置目标地址
			IPHDR * IpHdr = reinterpret_cast<IPHDR*>(p->Buffer);
			address.m_IPv4 = IpHdr->daddr;

			// 分发数据
			if (Mgr->DispatchPacket(p, address, Address::ADDR_VIRTUAL))
			{
				continue;
			}
		}
		// 没有分发成功，则释放空间，忽略此数据包
		delete p;
	}
}

void RouteTunThread::Stop()
{
	m_Running = false;
}
