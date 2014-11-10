#ifdef WIN32

#ifndef WIN32_HELPER_H
#define WIN32_HELPER_H

#include <string>

#define ADAPTER_KEY "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

// 获得网卡索引
// ClearIP 为真删除设备所有IP
int GetAdapterIndex(const char * AdapterGUID, bool ClearIP = false);

// 获得适配器GUID
std::string GetAdapterGUID(const std::string & AdapterName);

// 刷新IP列表
bool RenewAddress(int Index);

#endif // WIN32_HELPER_H

#endif //WIN32
