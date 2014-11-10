
#include "Config.h"
#include "SessionMgr.h"

#include <assert.h>

int main()
{
	SessionMgr * mgr = SessionMgr::Instance();
	
	Packet * packet = new Packet;
	packet->Len = 100;
	
	Address address;
	address.m_IPv4 = 0x010000cf;
	address.m_Port = 9999;
	mgr->DispatchPacket(packet, address, Address::ADDR_REAL);
	
	Session * session = mgr->FetchRunningItem();
	assert(session);
	assert(!mgr->FetchRunningItem());
	
	assert(session->m_ReadQueue.Size());
	
	packet = NULL;
	assert(session->m_ReadQueue.Pop(packet));
	
	assert(packet->Len == 100);
	
	return 0;
}
