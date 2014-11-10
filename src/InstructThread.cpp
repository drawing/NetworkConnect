
#include "Config.h"
#include "InstructThread.h"
#include "Util.h"

#include "SessionMgr.h"
#include "Address.h"

void InstructThread::Run()
{
	SessionMgr * Mgr = SessionMgr::Instance();
	while (m_Running)
	{
		m_Mutex.Lock();
		std::list <Instruction *>::iterator i = m_InstList.begin();
		uint32_t now = time(0);
		while (i != m_InstList.end())
		{
			// 检查指令队列
			// p->MaxTimeoutCount 为 0 表示没有最大超时限制
			Instruction * p = *i;
			if (p->Status == DESTROY ||
				(p->MaxTimeoutCount != 0 && p->TimeoutCount >= p->MaxTimeoutCount))
			{
				m_InstList.erase(i++);
				delete p;
				continue;
			}
			if (now - p->LastSendTime > p->Timeout)
			{
				++ p->TimeoutCount;
				p->LastSendTime = now;
				
				Packet * FixedPacket = &p->Data;
				
				// 分发失败需要释放
				Packet * packet = new Packet;
				packet->Len = FixedPacket->Len;
				memcpy(packet->Buffer, FixedPacket->Buffer, FixedPacket->Len);
				
				Address address;
				address.m_IPv4 = p->IPAddr;
				address.m_Port = p->Port;
				
				if (p->Flag & LOCAL_ADDRESS)
				{
					Mgr->DispatchPacket(packet, address, Address::ADDR_LOCAL);
				}
				else if (p->Flag & READ_ADDRESS)
				{
					Mgr->DispatchPacket(packet, address, Address::ADDR_REAL);
				}
				else if (p->Flag & VIRTUAL_ADDRESS)
				{
					Mgr->DispatchPacket(packet, address, Address::ADDR_VIRTUAL);
				}
			}
			++ i;
		}
		m_Mutex.Unlock();
		SecondSleep(INST_SCAN_INTERVAL);
	}
}

InstructThread::InstructThread() : m_Running(true)
{}

void InstructThread::Stop()
{
	m_Running = false;
}

// 指令功能
bool InstructThread::PushInst(Instruction * inst)
{
	m_Mutex.Lock();
	m_InstList.push_back(inst);
	m_Mutex.Unlock();
	return true;
}

bool InstructThread::Cannel(uint16_t Identify)
{
	bool retValue = false;
	m_Mutex.Lock();
	std::list <Instruction *>::iterator i = m_InstList.begin();
	while (i != m_InstList.end())
	{
		Instruction * p = *i ++;
		if (p->Identify == Identify)
		{
			if (p->Status != DESTROY)
			{
				p->Status = DESTROY;
				retValue = true;
			}
			break;
		}
	}
	m_Mutex.Unlock();
	return retValue;
}


bool InstructThread::Login(const Address & Server, const std::string & Username,
	const std::string & Passwd)
{
	Instruction * p = new Instruction;
	
	BuildHeader(p, IPV4 | LOCAL_ADDRESS, Server.m_IPv4, Server.m_Port, 1, MAX_LOGIN_RETRY);
	
	Packet * packet = &p->Data;
	BuildLoginRequest(packet, Username, Passwd, p->Identify);
	
	return PushInst(p);
}


bool InstructThread::StartHeartbeat(const Address & Server)
{
	Instruction * p = new Instruction;
	
	BuildHeader(p, IPV4 | LOCAL_ADDRESS, Server.m_IPv4, Server.m_Port, HEARTBEAT_INTERVAL, 0x0);
	
	Packet * packet = &p->Data;
	BuildHeartbeat(packet, p->Identify);
	
	return PushInst(p);
}
