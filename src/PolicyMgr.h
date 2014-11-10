#ifndef POLICY_MGR_H
#define POLICY_MGR_H

#include "Mutex.h"
#include "Instruction.h"
#include <string>
#include <vector>

class PolicyMgr
{
public:
	static PolicyMgr * Instance()
	{
		// C++ 不保证静态局部变量线程安全
		if (m_pPolicyMgr == NULL) {
			// 暂时没有默认管理器，需调用SetPolicyMgr设置
			assert(0);
			// return default_mgr;
			return NULL;
		} else {
			return m_pPolicyMgr;
		}

	}
	static void SetPolicyMgr(PolicyMgr * pPolicyMgr);
private:
	static PolicyMgr * m_pPolicyMgr;
public:
	virtual bool AllocAddr(const std::string & user, uint32_t * address, uint32_t * mask) = 0;
	virtual void FreeAddr(uint32_t) = 0;
	
	virtual bool Init() = 0;

	virtual uint32_t GetProtect(const std::string & user, Subnet * items) = 0;
	uint32_t GetGateway(const std::string & user, Subnet * items);
private:
	Mutex m_Mutex;
	
private:
	std::vector<bool> m_AddrPool;
public:
	virtual ~PolicyMgr() {}
};

#endif // POLICY_MGR_H
