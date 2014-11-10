#ifndef TOKEN_POLICY_MGR_H

#include "PolicyMgr.h"
#include "Address.h"
#include "Thread.h"

#include <map>
#include <vector>

class TokenPolicyMgr : public PolicyMgr, public Thread
{
public:
	TokenPolicyMgr(const Address & SelfAddress, const Address & TargetAddress);
	bool Init();
	void Run();
	uint32_t GetProtect(const std::string & user, Subnet * items);
	bool AllocAddr(const std::string & user, uint32_t * address, uint32_t * mask);
	void FreeAddr(uint32_t);
private:
	bool DoPolicy(char szBuffer[], int iLen);
	bool DoLogin(char szBuffer[], int iLen);
	bool DoLogout(char szBuffer[], int iLen);
private:
	int m_Sock;
private:
#define MAX_USERNAME_LEN 255
	struct RoleInfo {
		std::vector<uint32_t> AppList;
		char szRoleName[MAX_USERNAME_LEN];
	};
	struct UserInfo {
	};
	struct LoginInfo {
		uint32_t roleID;
		time_t loginTime;
		uint32_t virtualIP;
	};
private:
	std::map<int, Subnet> m_Policy;
	std::map<uint32_t, RoleInfo> m_Roles;
	std::map<uint32_t, UserInfo> m_Users;
	std::map<uint32_t, LoginInfo> m_LoginInfo;
};

#endif //TOKEN_POLIC_MGR_H
