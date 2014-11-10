#ifndef WORK_THREAD_H
#define WORK_THREAD_H

#include "Thread.h"
#include "Instruction.h"
#include "Session.h"

class WorkThread : public Thread
{
public:
	WorkThread();
	void Run();
	void Stop();
	
private:
	void RunUDPMode();
	void RunTCPMode();
	
private:
	// 处理指令
	bool ExecInst(InstPacket * inst, Session *);
	
private:
	bool ExecLoginRespond(InstPacket *);
	bool ExecLoginRequest(InstPacket *, Session *);
	
private:
	bool m_Running;
};

#endif // WORK_THREAD_H
