#include "Config.h"
#include "Address.h"

bool Address::operator < (const Address & other) const
{
	return m_IPv4 < other.m_IPv4 ||
		(m_IPv4 == other.m_IPv4 && m_Port < other.m_Port);
}

Address::Address() {}
