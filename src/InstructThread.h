#ifndef INSTRUCT_THREAD_H
#define INSTRUCT_THREAD_H

#include <list>
#include <string>
#include "Thread.h"
#include "Instruction.h"
#include "Address.h"
#include "Mutex.h"

class InstructThread : public Thread
{
public:
	// 线程功能
	InstructThread();
	void Run();
	void Stop();
public:
	// 指令功能
	bool PushInst(Instruction *);
	bool Cannel(uint16_t Identify);
	
public:
	// 登陆
	bool Login(const Address & Server, const std::string & Username,
		const std::string & Passwd);
	
	// 心跳
	bool StartHeartbeat(const Address & Server);
	
private:
	bool m_Running;
private:
	std::list <Instruction *> m_InstList;
	Mutex m_Mutex;
};

#endif // INSTRUCT_THREAD_H
