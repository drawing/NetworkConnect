
#include "Config.h"
#include "WorkThread.h"
#include "SessionMgr.h"
#include "TunHandler.h"
#include "ThreadMgr.h"
#include "PolicyMgr.h"

#include <openssl/err.h>

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include "Util.h"
#include "Options.h"

void WorkThread::Run()
{
	if (Options::Instance()->UDPMode)
	{
		RunUDPMode();
	}
	else
	{
		RunTCPMode();
	}
}

WorkThread::WorkThread() : m_Running(true)
{}

void WorkThread::Stop()
{
	m_Running = false;
}


bool WorkThread::ExecInst(InstPacket * inst, Session * session)
{
	switch (inst->Type)
	{
	case LOGIN_REQUEST:
		return ExecLoginRequest(inst, session);
	case LOGIN_RESPOND:
		// 登陆响应，设置虚拟IP和策略
		return ExecLoginRespond(inst);
	default:
		break;
	}
	return true;
}

bool WorkThread::ExecLoginRespond(InstPacket * inst)
{
	LoginRespond * respond = (LoginRespond *)DATA_PTR(inst);
	bool bRetValue = true;
	TunHandler * Handle = TunHandler::Instance();
	ThreadMgr * thread_mgr = ThreadMgr::Instance();
	Options * options = Options::Instance();
	
	// 取消指令
	if (!thread_mgr->m_Inst.Cannel(inst->Identify))
	{
		return false;
	}
	
	printf("Login:Success\n");
	in_addr addr;
	//S_addr(addr) = respond->TunAddr;
	addr.s_addr = respond->TunAddr;
	std::string Address = inet_ntoa(addr);
	printf("TunIP:%s\n", Address.c_str());
	
	addr.s_addr = respond->Mask;
	std::string Mask = inet_ntoa(addr);
	printf("TunMask:%s\n", Mask.c_str());
	
	addr.s_addr = respond->Gateway;
	std::string Gateway = inet_ntoa(addr);
	printf("TunGw:%s\n", Gateway.c_str());
	
	Handle->Start(Address, Mask);

	// 添加策略
	for (uint32_t i = 0; i < respond->ProtectCount; ++i)
	{
		addr.s_addr = respond->Protect[i].Address;
		std::string ProtectAddress = inet_ntoa(addr);
		
		addr.s_addr = respond->Protect[i].Mask;
		std::string ProtectMask = inet_ntoa(addr);

		Handle->AddRoute(
			ProtectAddress,
			ProtectMask,
			Gateway
			);
		printf("Policy:%s %s %s\n", ProtectAddress.c_str(), ProtectMask.c_str(), Gateway.c_str());
	}
	
	if (!thread_mgr->StartNewThread(ThreadMgr::TUN_THREAD))
	{
		// 启动线程失败
		bRetValue = false;
	}
	
	// 发送心跳包
	class Address Server;
	Server.m_IPv4 = inet_addr(options->ServerAddress.c_str());
	Server.m_Port = htons(atoi(options->ServerPort.c_str()));
	
	if (!thread_mgr->m_Inst.StartHeartbeat(Server))
	{
		// 错误，没有心跳
	}

	return bRetValue;
}

bool WorkThread::ExecLoginRequest(InstPacket * inst, Session * session)
{
	SessionMgr * Mgr = SessionMgr::Instance();
	LoginRequest * request = (LoginRequest *)DATA_PTR(inst);
	PolicyMgr * policy = (PolicyMgr *)PolicyMgr::Instance();
	Options * options = Options::Instance();
	Packet packet;
	
	bool bRet = true;
	// 判断用户名密码
	printf("Login a uesr: (%s, %s)\n", request->User, request->Passwd);
	
	// 分配地址
	Address address;
	address.m_Port = 0;
	uint32_t mask;
	bRet = policy->AllocAddr(reinterpret_cast<char*>(request->User), &address.m_IPv4, &mask);
	if (!bRet) {
		printf("%s alloc address failed\n", request->User);
	}
	
	// 关联地址
	Mgr->AssociateVirtual(session, address);
	
	// 得到保护策略
	Subnet items[MAX_PROTECT_COUNT];
	uint32_t count = policy->GetProtect(reinterpret_cast<char*>(request->User), items);
	printf("%s protect count %d\n", request->User, count);
	for (int i = 0; i < count; ++i) {
		printf("%s protect %d %x %x\n", request->User, i, items[i].Address, items[i].Mask);
	}
	
	// 得到用户的网关地址
	Subnet Gateway;
	policy->GetGateway(reinterpret_cast<char*>(request->User), &Gateway);
	// 检测 返回值，为 1

	// 组装 Response 包并返回
	if (BuildLoginRespond(&packet, address.m_IPv4, mask, Gateway.Address, count, items, inst->Identify))
	{
		if (options->UDPMode)
		{
			/// 删除 包跟踪
			/*
			*(int*)&packet.Buffer[packet.Len] = rand();
			printf("write --> %02x%02x%02x		num %d --> len : %d\n", packet.Buffer[0], packet.Buffer[1], packet.Buffer[2],
				*(int*)&packet.Buffer[packet.Len], packet.Len);
			packet.Len += 4;
			// end*/
			
			return SSL_write(session->m_SSL, packet.Buffer, packet.Len) > 0;
		}
		else
		{
			if (SSL_write(session->m_SSL, &packet.Len, sizeof packet.Len) > 0)
			{
				return SSL_write(session->m_SSL, packet.Buffer, packet.Len) > 0;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}


void WorkThread::RunUDPMode()
{
	SessionMgr * Mgr = SessionMgr::Instance();
	TunHandler * Tun = TunHandler::Instance();
	
	Session * session;
	Packet * packet;
	
	struct Packet TempData;
	
	bool IsDestroyed;
	
	while (m_Running)
	{
		// 获取运行队列中 Session
		session = Mgr->FetchRunningItem();
		if (!session)
		{
			continue;
		}
		
		IsDestroyed = false;
		uint8_t TransPacket = 0;
		
		// 处理写队列数据包
		for (int i = 0; i < 20 && session->m_WriteQueue.Pop(packet); ++i)
		{
			/// 删除 包跟踪
			/*
			*(int*)&packet->Buffer[packet->Len] = rand();
			printf("write --> %02x%02x%02x		num %d --> len : %d\n", packet->Buffer[0], packet->Buffer[1], packet->Buffer[2],
				*(int*)&packet->Buffer[packet->Len], packet->Len);
			packet->Len += 4;
			*/
			// end
			
			SSL_write(session->m_SSL, packet->Buffer, packet->Len);
			
			delete packet;
			TransPacket ++;
		}
		
		// 处理读队列数据包
		for (int i = 0; i < 20 && session->m_ReadQueue.Pop(packet); ++i)
		{
			BIO * rbio = BIO_new_mem_buf(packet->Buffer, packet->Len);
			BIO_set_mem_eof_return(rbio, -1);
			
			session->m_SSL->rbio = rbio;

			TempData.Len = SSL_read(session->m_SSL, TempData.Buffer, sizeof TempData.Buffer);
			
			/// 删除 包跟踪
			/*
			printf("read --> %02x%02x%02x		num %d --> len : %d\n", TempData.Buffer[0], TempData.Buffer[1], TempData.Buffer[2],
				*(int*)&TempData.Buffer[TempData.Len - 4], TempData.Len - 4);
			TempData.Len -= 4;
			// end*/

			session->m_SSL->rbio = NULL;
			BIO_free(rbio);
			
			if (TempData.Len > 0)
			{
				session->m_bConnected = true;
				
				// 检测 TempData 是否指令包
				if (TempData.Buffer[0] == 0)
				{
					ExecInst((InstPacket *)TempData.Buffer, session);
				}
				else
				{
					Tun->Write(TempData);
				}
			}
			else
			{
				ERR_print_errors_fp(stderr);
				// 如果已经连接，则断开连接
				if (session->m_bConnected)
				{
					session->m_bConnected = false;
					Mgr->DestroySession(session);
					IsDestroyed = true;
				}
			}
			
			delete packet;
			TransPacket ++;
		}
		
		if (IsDestroyed)
			continue;
			
		// 设置连接状态（Running 或者 Sleep）
		if (TransPacket)
		{
			Mgr->PushRunningItem(session);
		}
		else
		{
			Mgr->PushSleepingItem(session);
		}
	}
}

void WorkThread::RunTCPMode()
{
	Session * session;
	Packet * packet;
	
	SessionMgr * Mgr = SessionMgr::Instance();
	TunHandler * Tun = TunHandler::Instance();
	
	bool ServerMode = Options::Instance()->ServerMode;

	while (m_Running)
	{
		// 获取运行队列中 Session
		session = Mgr->FetchRunningItem();
		if (!session)
		{
			continue;
		}

		uint8_t TransPacket = 0;
		
		// 处理写队列数据包
		if (session->m_WriteQueue.Pop(packet))
		{
			// 测试日志
			// printf("write len %d address, %x\n", packet->Len, &packet->Len);
			// 测试日志结束

			// TCP 模式先写长度
			SSL_write(session->m_SSL, &packet->Len, sizeof packet->Len);
			SSL_write(session->m_SSL, packet->Buffer, packet->Len);
			
			delete packet;
			TransPacket ++;
		}
		
		// 处理读队列数据包
		if (session->m_ReadQueue.Pop(packet))
		{
			BIO * rbio = BIO_new_mem_buf(packet->Buffer, packet->Len);
			BIO_set_mem_eof_return(rbio, -1);
			
			session->m_SSL->rbio = rbio;

			Packet & TcpBuffer = session->m_TcpBuffer;
			int TempLen = 0;
			
			// 对于每一个读取包循环
			while (true)
			{
				if (!session->m_bConnected && ServerMode)
				{
					if (SSL_accept(session->m_SSL) > 0)
					{
						session->m_bConnected = true;
					}
					else
					{
						// ERR_print_errors_fp(stderr);
						// 无数据可读，处理下一个包
						break;
					}
				}
				else if (TcpBuffer.Len == 0)
				{
					// 先读取数据长度
					TempLen = SSL_read(session->m_SSL, &TcpBuffer.Len, sizeof TcpBuffer.Len);
					if (TempLen < 0)
					{
						// 无数据可读，处理下一个包
						break;
					}
					
					// 测试日志
					// printf("read len %d, packet len %d\n", TempLen, TcpBuffer.Len);
					// 测试日志结束
			
					session->m_TcpNowLen = 0;
				}
				else
				{
					TempLen = SSL_read(session->m_SSL,
						TcpBuffer.Buffer + session->m_TcpNowLen,
						TcpBuffer.Len - session->m_TcpNowLen);
						
					if (TempLen <= 0)
					{
						// 无数据可读
						break;
					}
					session->m_TcpNowLen += TempLen;
					if (session->m_TcpNowLen == TcpBuffer.Len)
					{
						// 一个包完成
						// 检测 TempData 是否指令包
						if (TcpBuffer.Buffer[0] == 0)
						{
							ExecInst((InstPacket *)TcpBuffer.Buffer, session);
						}
						else
						{
							Tun->Write(TcpBuffer);
						}
						TcpBuffer.Len = 0;
					}
				}
			}
			
			session->m_SSL->rbio = NULL;
			BIO_free(rbio);
			
			delete packet;
			TransPacket ++;
		}

		// 设置连接状态（Running 或者 Sleep）
		if (TransPacket)
		{
			Mgr->PushRunningItem(session);
		}
		else
		{
			Mgr->PushSleepingItem(session);
		}
	}
}

