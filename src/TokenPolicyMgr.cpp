#include "TokenPolicyMgr.h"
#include <tinyxml.h>
#include "Options.h"
#include "Util.h"

#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>

	#include <netinet/tcp.h>
#endif

TokenPolicyMgr::TokenPolicyMgr(const Address & SelfAddress, const Address & TargetAddress)
{
}

bool TokenPolicyMgr::Init()
{
	m_Sock = (int)socket(AF_INET, SOCK_STREAM, 0);
	this->Start();
	return true;
}

void TokenPolicyMgr::Run()
{
	Options * options = Options::Instance();

	sockaddr_in dst;
	dst.sin_family = AF_INET;
	dst.sin_port = htons(atoi(options->ControlPort.c_str()));
	dst.sin_addr.s_addr = inet_addr(options->ControlAddress.c_str());

	while (true)
	{
		int err = connect(m_Sock, (sockaddr*)&dst, sizeof dst);
		if (err == -1) {
			SecondSleep(2);
			continue;
		}

		while (true) {
			uint8_t cType = 0;
			int err = recv(m_Sock, (char*)&cType, 1, 0);
			if (err <= 0) {
				break;
			}

			uint16_t iLen = 0;
			err = recv(m_Sock, (char*)&iLen, sizeof iLen, 0);
			iLen = ntohs(iLen);
			printf("cType=%d, iLen=%d\n", cType, iLen);
			char Buffer[1024];
			err = recv(m_Sock, Buffer, iLen, 0);
			switch (cType) {
			case 1:
				// 登入命令
				DoLogin(Buffer, iLen);
				break;
			case 2:
				// 登出命令
				DoLogout(Buffer, iLen);
				break;
			case 3:
				// 策略下发
				DoPolicy(Buffer, iLen);
				break;
			default:
				break;
			}
		}
	}
}

bool TokenPolicyMgr::DoLogout(char szBuffer[], int iLen)
{
	return true;
}

bool TokenPolicyMgr::DoLogin(char szBuffer[], int iLen)
{
	TiXmlDocument doc;
	doc.Parse(szBuffer);

	TiXmlElement * root = doc.RootElement();
	if (root == NULL)
		return false;

	TiXmlElement * userAuth = root->FirstChildElement("USER_AUTH_CONFIG");

	if (userAuth == NULL)
		return false;

	TiXmlElement * userInfo = userAuth->FirstChildElement("USER_INFO");
	while (userInfo) {
		const char * userId = userInfo->Attribute("userId");
		const char * roleId = userInfo->Attribute("roleId");
		const char * userName = userInfo->Attribute("userName");
		const char * loginTime = userInfo->Attribute("loginTime");
		const char * virtualIP = userInfo->Attribute("virtualIP");

		if (userId == NULL || roleId == NULL || userName == NULL ||
				loginTime == NULL || virtualIP == NULL) {
			userInfo = userInfo->NextSiblingElement();
			continue;
		}
		uint32_t id = (uint32_t)atoll(userId);
		uint32_t roleID = (uint32_t)atoll(roleId);
		time_t tm = (time_t)atoll(loginTime);
		uint32_t ip = inet_addr(virtualIP);
		LoginInfo info = {roleID, tm, ip};
		m_LoginInfo[id] = info;

		printf("login command:\t%s, %s, %s, %s\n", userId, roleId, userName, virtualIP);
		userInfo = userInfo->NextSiblingElement();
	}

	return true;
}


bool TokenPolicyMgr::DoPolicy(char szBuffer[], int iLen)
{
	TiXmlDocument doc;
	doc.Parse(szBuffer);

	TiXmlElement * root = doc.RootElement();
	if (root == NULL)
		return false;

	// TiXmlElement * appList = root->FirstChildElement();
	TiXmlElement * appList = root->FirstChildElement("APP_LIST");
	TiXmlElement * roleList = root->FirstChildElement("ROLE_LIST");

	// printf("%s, %s\n", root->Value(), appList->Value());
	// printf("appList=%x, roleList=%x\n", appList->NextSiblingElement(), roleList);
	if (appList == NULL || roleList == NULL)
		return false;

	m_Policy.clear();
	TiXmlElement * appInfo = appList->FirstChildElement("APP_INFO");
	while (appInfo) {
		const char * id = appInfo->Attribute("appId");
		const char * addr = appInfo->Attribute("appAddr");
		const char * mask = appInfo->Attribute("appMask");
		const char * port = appInfo->Attribute("appPort");

		if (id == NULL || addr == NULL || mask == NULL || port == NULL) {
			appInfo = appInfo->NextSiblingElement();
			continue;
		}
		uint32_t appid = (uint32_t)atoll(id);
		Subnet item = {inet_addr(addr), inet_addr(mask)};
		m_Policy[appid] = item;

		printf("app:\t%s, %s, %s, %s\n", id, addr, mask, port);
		appInfo = appInfo->NextSiblingElement();
	}

	TiXmlElement * roleInfo = roleList->FirstChildElement("ROLE_INFO");
	while (roleInfo) {
		const char * id = roleInfo->Attribute("roleId");
		const char * name = roleInfo->Attribute("roleName");
		if (id == NULL || name == NULL) {
			roleInfo = appInfo->NextSiblingElement();
			continue;
		}
		RoleInfo & role = m_Roles[(uint32_t)atoll(id)];
		if (strlen(name) < MAX_USERNAME_LEN) {
			strcpy(role.szRoleName, name);
		} else {
			memcpy(role.szRoleName, name, MAX_USERNAME_LEN - 1);
			role.szRoleName[MAX_USERNAME_LEN - 1] = '\0';
		}
		printf("role:\t%s, %s\n", id, name);
		role.AppList.clear();
		TiXmlElement * elem = roleInfo->FirstChildElement("ROLE_APP_TABLE_INFO");
		while (elem) {
			const char * id = elem->Attribute("appId");
			if (id == NULL) {
				elem = elem->NextSiblingElement();
				break;
			}
			printf("\trole->id %s\n", id);
			role.AppList.push_back((uint32_t)atoll(id));
			elem = elem->NextSiblingElement();
		}
		roleInfo = roleInfo->NextSiblingElement();
	}

	return true;
}

uint32_t TokenPolicyMgr::GetProtect(const std::string & user, Subnet * items)
{
	uint32_t UserId = (uint32_t)atoll(user.c_str());
	std::map<uint32_t, LoginInfo>::iterator k = m_LoginInfo.find(UserId);
	if (k == m_LoginInfo.end())
		return 0;

	std::map<uint32_t, RoleInfo>::iterator i = m_Roles.find(m_LoginInfo[UserId].roleID);
	if (i == m_Roles.end())
		return 0;

	for (unsigned k = 0; k < i->second.AppList.size(); k++) {
		items[k] = m_Policy[i->second.AppList[k]];
	}
	return i->second.AppList.size();
}

bool TokenPolicyMgr::AllocAddr(const std::string & user, uint32_t * address, uint32_t * mask)
{
	uint32_t UserId = (uint32_t)atoll(user.c_str());
	std::map<uint32_t, LoginInfo>::iterator i = m_LoginInfo.find(UserId);
	if (i == m_LoginInfo.end())
		return false;
	*address = i->second.virtualIP;
	// 掩码表示方式
	*mask = ntohl(0xffffff00);
	return true;
}

void TokenPolicyMgr::FreeAddr(uint32_t)
{
}
