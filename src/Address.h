#ifndef ADDRESS_H
#define ADDRESS_H

#include <stdint.h>

class Address
{
public:
	enum AddressType {ADDR_VIRTUAL, ADDR_REAL, ADDR_LOCAL, ADDR_UNDEFINED};
public:
	Address();
	
public:
	bool operator < (const Address & other) const;

public:
	uint32_t m_IPv4;
	uint16_t m_Port;
	
	int m_Sock;
};

#endif // ADDRESS_H
