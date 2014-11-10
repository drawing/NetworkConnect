#ifdef WIN32

#include "Config.h"
#include "Win32Helper.h"

#include <windows.h>
#include <Iphlpapi.h>

std::string GetAdapterGUID(const std::string & AdapterName)
{
	HKEY NetCardKey;
    LONG Status;
    DWORD Len;
	std::string AdatperGUID;

    Status = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        ADAPTER_KEY,
        0,
        KEY_READ,
        &NetCardKey);

    if (Status != ERROR_SUCCESS) {
        return false;
    }

    for (int i = 0; ; i++)
	{
        char EnumName[256];
        char UnitString[256];
        HKEY UnitKey;
        char ComponentIdString[] = "ComponentId";
        char ComponentId[256];
        char NetCfgInstanceIdString[] = "NetCfgInstanceId";
        char NetCfgInstanceId[256];
        DWORD DataType;

        Len = sizeof (EnumName);
        Status = RegEnumKeyEx(
            NetCardKey,
            i,
            EnumName,
            &Len,
            NULL,
            NULL,
            NULL,
            NULL);

        if (Status == ERROR_NO_MORE_ITEMS
			|| Status != ERROR_SUCCESS)
		{
            break;
		}

        sprintf (UnitString, "%s\\%s",
                  ADAPTER_KEY, EnumName);

        Status = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            UnitString,
            0,
            KEY_READ,
            &UnitKey);

        if (Status != ERROR_SUCCESS)
		{
            break;
        }

		Len = sizeof (ComponentId);
		Status = RegQueryValueEx(
			UnitKey,
			ComponentIdString,
			NULL,
			&DataType,
			(LPBYTE)ComponentId,
			&Len);

		if (Status == ERROR_SUCCESS && DataType == REG_SZ)
		{
			Len = sizeof (NetCfgInstanceId);
			Status = RegQueryValueEx(
				UnitKey,
				NetCfgInstanceIdString,
				NULL,
				&DataType,
				(LPBYTE)NetCfgInstanceId,
				&Len);

			if (Status == ERROR_SUCCESS && DataType == REG_SZ)
			{
				if (ComponentId == AdapterName)
				{
					AdatperGUID = NetCfgInstanceId;
					RegCloseKey (UnitKey);
					break;
				}
			}
		}
		RegCloseKey (UnitKey);

    }

    RegCloseKey (NetCardKey);
    return AdatperGUID;
}


int GetAdapterIndex(const char * AdapterGUID, bool ClearIP)
{
	int index = -1;
	ULONG ulOutBufLen = 0;
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	PIP_ADAPTER_INFO pAdapter = NULL;

	if (GetAdaptersInfo (NULL, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);

		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
		{
			pAdapter = pAdapterInfo;
			while (pAdapter)
			{
				if (strcmp(AdapterGUID, pAdapter->AdapterName) == 0)
				{
					index = pAdapter->Index;
					IP_ADDR_STRING * IP = &pAdapter->IpAddressList;
					while (IP && ClearIP)
					{
						ULONG Context = IP->Context;
						DeleteIPAddress(Context);
						IP = IP->Next;
					}
					break;
				}

				pAdapter = pAdapter->Next;
			}
		}
	}

	if (pAdapterInfo)
	{
		free(pAdapterInfo);
	}

	return index;
}

bool RenewAddress(int Index)
{
	PIP_INTERFACE_INFO pInfo = 0;
	ULONG ulOutBufLen = 0;
	bool bRetValue = false;

	if (GetInterfaceInfo(pInfo, &ulOutBufLen) != ERROR_INSUFFICIENT_BUFFER)
	{
		goto clean;
	}

	pInfo = (IP_INTERFACE_INFO *) malloc (ulOutBufLen);
	if (GetInterfaceInfo(pInfo, &ulOutBufLen) != NO_ERROR)
	{
		goto clean;
	}

	for (int i = 0; i < pInfo->NumAdapters; ++i)
	{
		if (Index == pInfo->Adapter[i].Index)
		{
			IpReleaseAddress(&pInfo->Adapter[i]);

			if (IpRenewAddress(&pInfo->Adapter[i]) == NO_ERROR)
			{
				bRetValue = true;
			}
			break;
		}
	}

clean:
	if (pInfo)
	{
		free(pInfo);
	}
	return bRetValue;
}

#endif // WIN32
