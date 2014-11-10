
#ifndef TUN_HANDLER
#define TUN_HANDLER

#include <string>
#ifdef WIN32
	#include <windows.h>
#endif

#include "Packet.h"

class TunHandler
{
public:
	bool Read(Packet & packet);
	bool Write(Packet & packet);
public:
	static TunHandler * Instance()
	{
		// C++ 不保证静态局部变量线程安全
		static TunHandler Handler;
		return &Handler;
	}
	
private:
	TunHandler();
	TunHandler(const TunHandler&);
	TunHandler & operator = (const TunHandler&);
	~TunHandler();
public:
	// 激活网卡
	bool Start(const std::string & InnerIP = "", const std::string & Mask = "");
	// 停用网卡
	bool Stop();

	// 设置 DHCP
	bool ConfigDHCP(const std::string & IPAddress, const std::string & Mask, const std::string & DHCPServer);
	// 添加路由项  Gate 不可与本地地址相同
	bool AddRoute(const std::string & Destination, const std::string & Mask, const std::string & Gate);
	// 添加虚拟网卡 IP
	bool AddIP(const std::string & IPAddress, const std::string & Mask);

private:
#ifdef WIN32
	HANDLE m_Handler;
	
	OVERLAPPED m_ReadOverlapped;
	OVERLAPPED m_WriteOverlapped;
#else
	int m_Handler;
#endif
};

#endif // TUN_HANDLER
