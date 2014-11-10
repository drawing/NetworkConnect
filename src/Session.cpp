#include "Config.h"
#include "Session.h"

Session::Session(): m_Status(SLEEPED), m_SSL(NULL), m_bConnected(false)
{
	m_LastDataTime = time(0);
	m_TcpBuffer.Len = 0;
}

Session::~Session()
{
	// 释放资源
	if (m_SSL)
	{
		SSL_free(m_SSL);
	}

	Packet * packet;
	while (!m_ReadQueue.Empty())
	{
		m_ReadQueue.Pop(packet);
		delete packet;
	}
	while (!m_WriteQueue.Empty())
	{
		m_WriteQueue.Pop(packet);
		delete packet;
	}
}
