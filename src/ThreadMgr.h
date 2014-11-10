#ifndef THREAD_MGR_H
#define THREAD_MGR_H

#include <list>
#include <string>

#include "RouteTunThread.h"
#include "RouteSockThread.h"
#include "InstructThread.h"
#include "WorkThread.h"
#include "Address.h"

class ThreadMgr
{
private:
	RouteTunThread m_Tun;
	RouteSockThread m_Sock;
public:
	InstructThread m_Inst;
private:
	std::list<WorkThread *> m_WorkList;
	
public:
	enum ThreadType {TUN_THREAD, SOCK_THREAD, WORK_THREAD, INSTRUCT_THREAD};

public:
	static ThreadMgr * Instance()
	{
		// C++ 不保证静态局部变量线程安全
		static ThreadMgr thread_mgr;
		return &thread_mgr;
	}

public:
	// 线程管理函数
	bool StartNewThread(ThreadType type, void * Parameter = 0);
	bool StopThread(ThreadType type);
	
public:
	// 客户端指令控制函数
	bool Login(const Address & Server, const std::string & Username,
		const std::string & Passwd);
	bool Logout();
	
	// 服务器端控制
	bool Listen(const Address & Server);
	bool Destroy();

private:
	ThreadMgr();
	~ThreadMgr();
	ThreadMgr(const ThreadMgr & );
	ThreadMgr & operator = (const ThreadMgr &);
};

#endif // THREAD_MGR_H
