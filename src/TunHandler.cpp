
#include "Config.h"
#include "TunHandler.h"

#ifdef WIN32

#include <winioctl.h>
#include <Iprtrmib.h>
#include <Iphlpapi.h>
#include "Win32Helper.h"

#define ADAPTER_NAME "sectap"

#define TAP_CONTROL_CODE(request,method) \
  CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

#define TAP_IOCTL_GET_MAC               TAP_CONTROL_CODE (1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION           TAP_CONTROL_CODE (2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU               TAP_CONTROL_CODE (3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO              TAP_CONTROL_CODE (4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE (5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS      TAP_CONTROL_CODE (6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ      TAP_CONTROL_CODE (7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE          TAP_CONTROL_CODE (8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT   TAP_CONTROL_CODE (9, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_TUN            TAP_CONTROL_CODE (10, METHOD_BUFFERED)


TunHandler::TunHandler() : m_Handler(INVALID_HANDLE_VALUE)
{
	memset(&m_ReadOverlapped, 0, sizeof m_ReadOverlapped);
	m_ReadOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	memset(&m_WriteOverlapped, 0, sizeof m_WriteOverlapped);
	m_WriteOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

TunHandler::~TunHandler()
{
	CloseHandle(m_ReadOverlapped.hEvent);
	CloseHandle(m_WriteOverlapped.hEvent);
}

bool TunHandler::Read(Packet & packet)
{
	DWORD dwNumberOfBytesRead = 0;

	if (ReadFile(m_Handler, packet.Buffer, MAX_PACKET_BUFFER, &dwNumberOfBytesRead, &m_ReadOverlapped)
		|| GetLastError() == ERROR_IO_PENDING)
	{
		WaitForSingleObject(m_ReadOverlapped.hEvent, INFINITE);
		GetOverlappedResult(m_Handler, &m_ReadOverlapped, &dwNumberOfBytesRead, 0);
		packet.Len = dwNumberOfBytesRead;

		return true;
	}
	else
		return false;
}

bool TunHandler::Write(Packet & packet)
{
	DWORD dwNumberOfBytesWritten = 0;
	if (WriteFile(m_Handler, packet.Buffer, MAX_PACKET_BUFFER, &dwNumberOfBytesWritten, &m_WriteOverlapped)
		|| GetLastError() == ERROR_IO_PENDING)
	{
		WaitForSingleObject(m_WriteOverlapped.hEvent, INFINITE);
		GetOverlappedResult(m_Handler, &m_WriteOverlapped, &dwNumberOfBytesWritten, 0);
		if (dwNumberOfBytesWritten != packet.Len)
			return true;
	}

	return false;
}

bool TunHandler::Start(const std::string & InnerIP, const std::string & Mask)
{
	std::string AdapterGUID = GetAdapterGUID(ADAPTER_NAME);
	char DeviceName[255];
	sprintf(DeviceName, "\\\\.\\Global\\%s.tap", AdapterGUID.c_str());
	
	BOOL Success;
	int Index;

	// 打开设备
	m_Handler = CreateFile (
		DeviceName,
		GENERIC_READ | GENERIC_WRITE,
		0, /* was: FILE_SHARE_READ */
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
		0
		);
	
	// 设置 Tun 模式
	ULONG ep[3];
	DWORD len = 0;
	ULONG MediaStatus = TRUE;

	ULONG local = inet_addr(InnerIP.c_str());
	ULONG mask = inet_addr(Mask.c_str());
	ep[0] = local;
	ep[1] = local & mask;
	ep[2] = mask;

	Success = DeviceIoControl (m_Handler, TAP_IOCTL_CONFIG_TUN,
			ep, sizeof ep,
			ep, sizeof ep, &len, NULL);
	if (!Success)
		goto Err_Exit;
	
	// 设置网卡启动状态
    Success = DeviceIoControl (m_Handler, TAP_IOCTL_SET_MEDIA_STATUS,
			&MediaStatus, sizeof (MediaStatus),
			&MediaStatus, sizeof (MediaStatus), &len, NULL);
	if (!Success)
		goto Err_Exit;
	
	Index = GetAdapterIndex(AdapterGUID.c_str(), true);
	if (Index >= 0)
	{
		// 重新获得IP
		ULONG IPAPI_Instance;
		ULONG IPAPI_Context;
		return AddIPAddress(inet_addr(InnerIP.c_str()),
			inet_addr(Mask.c_str()),
			Index,
			&IPAPI_Context,
			&IPAPI_Instance) == NO_ERROR;
	}
	else
	{
		return false;
	}
	
	return true;
		
Err_Exit:
	if (m_Handler != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_Handler);
		m_Handler = INVALID_HANDLE_VALUE;
	}
	return false;
}

bool TunHandler::ConfigDHCP(const std::string & IPAddress, const std::string & Mask, const std::string & DHCPServer)
{
	if (m_Handler == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	ULONG DhcpConfig[4];
	DWORD BytesReturned; 
	DhcpConfig[0] = inet_addr(IPAddress.c_str());
	DhcpConfig[1] = inet_addr(Mask.c_str());
	DhcpConfig[2] = inet_addr(DHCPServer.c_str());
	DhcpConfig[3] = 50000; // 租约，秒

	BOOL Success = DeviceIoControl (m_Handler, TAP_IOCTL_CONFIG_DHCP_MASQ,
			DhcpConfig, sizeof DhcpConfig,
			DhcpConfig, sizeof DhcpConfig, &BytesReturned, NULL);
	// RenewAddress(Index);
	return (bool)Success;
}

bool TunHandler::Stop()
{
	if (m_Handler != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_Handler);
		m_Handler = INVALID_HANDLE_VALUE;
	}
	return true;
}

// AddRoute 前必须确保 IP 地址已正确设置
bool TunHandler::AddRoute(const std::string & Destination, const std::string & Mask, const std::string & Gate)
{
	if (m_Handler == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	
	int Index = GetAdapterIndex(GetAdapterGUID(ADAPTER_NAME).c_str());
	if (Index == -1)
	{
		return false;
	}
	
	MIB_IPFORWARDROW fr;
	fr.dwForwardDest = inet_addr(Destination.c_str());
	fr.dwForwardMask = inet_addr(Mask.c_str());
	fr.dwForwardPolicy = 0;
	fr.dwForwardNextHop = inet_addr(Gate.c_str());
	fr.dwForwardIfIndex = Index;
	fr.dwForwardType = 4;  /* the next hop is not the final dest */
	fr.dwForwardProto = 3; /* PROTO_IP_NETMGMT */
	fr.dwForwardAge = 0;
	fr.dwForwardNextHopAS = 0;
	fr.dwForwardMetric1 = 1;
	fr.dwForwardMetric2 = ~0;
	fr.dwForwardMetric3 = ~0;
	fr.dwForwardMetric4 = ~0;
	fr.dwForwardMetric5 = ~0;

	if (CreateIpForwardEntry(&fr) == NO_ERROR)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool TunHandler::AddIP(const std::string & IPAddress, const std::string & Mask)
{
	// 不支持此操作
	return true;
	
	int Index = GetAdapterIndex(GetAdapterGUID(ADAPTER_NAME).c_str(), false);
	if (Index >= 0)
	{
		// 重新获得IP
		ULONG IPAPI_Instance;
		ULONG IPAPI_Context;
		return AddIPAddress(inet_addr(IPAddress.c_str()),
			inet_addr(Mask.c_str()),
			Index,
			&IPAPI_Context,
			&IPAPI_Instance) == NO_ERROR;
	}
	else
	{
		return false;
	}
	return false;
}

#else

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <stdlib.h>
#include <stdio.h>

#define TUN_DEV "/dev/net/tun"

bool TunHandler::Start(const std::string & InnerIP, const std::string & Mask)
{
	struct ifreq ifr = {};
	bool bRet = false;

	if ((m_Handler = open(TUN_DEV, O_RDWR)) != -1)
	{
		ifr.ifr_flags = IFF_NO_PI;
		ifr.ifr_flags |= IFF_TUN;

		if (ioctl (m_Handler, TUNSETIFF, (void *) &ifr) < 0)
		{
			close (m_Handler);
		}
		else
		{
			bRet = true;
		}
	}
	
	if (bRet)
	{
		bRet = AddIP(InnerIP, Mask);
	}
	return bRet;
}

bool TunHandler::Stop()
{
	return close(m_Handler) != -1;
}

bool TunHandler::Read(Packet & packet)
{
	packet.Len = ::read(m_Handler, packet.Buffer, sizeof packet.Buffer);
	return packet.Len > 0;
}

bool TunHandler::Write(Packet & packet)
{
	return ::write(m_Handler, packet.Buffer, packet.Len) > 0;
}

bool TunHandler::AddRoute(const std::string & Destination,
	const std::string & Mask, const std::string & Gate)
{
	std::string cmd = "route add -net " + Destination + " netmask "
		+ Mask + " gw " + Gate;
	return system(cmd.c_str()) != -1;
}

bool TunHandler::ConfigDHCP(const std::string &, const std::string &, const std::string &)
{
	return true;
}

bool TunHandler::AddIP(const std::string & IPAddress, const std::string & Mask)
{
	static int Count = 0;
	char TempData[255] = {};
	
	if (Count == 0)
	{
		sprintf(TempData, "ifconfig tun0 %s netmask %s mtu 1400 up", IPAddress.c_str(), Mask.c_str());
	}
	else
	{
		sprintf(TempData, "ifconfig tun0:%d %s netmask %s mtu 1400 up", Count - 1, IPAddress.c_str(), Mask.c_str());
	}
	Count ++;
	
	return system(TempData) != -1;
}

TunHandler::TunHandler() {}
TunHandler::~TunHandler() {}

#endif
