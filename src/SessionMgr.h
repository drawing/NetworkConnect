
#include "Session.h"

#include <openssl/ssl.h>

class SessionMgr
{
public:
	// 分发数据包
	bool DispatchPacket(Packet * packet, Address address, Address::AddressType type);
	
	// pop 运行队列元素
	Session * FetchRunningItem();
	
	// push 运行队列元素
	bool PushRunningItem(Session *);
	
	// 销毁Session
	bool DestroySession(Session *);
	
	// 关联虚拟地址
	bool AssociateVirtual(Session *, Address &);
	
	// 初始化 SSL
	bool InitSSL();
	
	// 创建网关 Session
	const SSL * CreateGateway(const Address &);
	
	// 设置UDP socket
	void SetSock(int sock);
	
	// 设置 Session 为 Sleep 状态
	bool PushSleepingItem(Session *);
	
	// 清理不活动的 Session
	bool CleanSessions(time_t past);

private:
	SessionMgr();
	~SessionMgr();
	SessionMgr(const SessionMgr&);
	SessionMgr & operator = (const SessionMgr&);
public:
	static SessionMgr * Instance();
	
private:
	Context m_Context;
	Mutex m_Mutex;
	Semaphore m_RunningSemaphore;
	SSL_CTX * m_SSLCTX;
	
	Session * m_Gateway;
	
	int m_SelfSock;
};
