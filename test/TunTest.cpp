#include "Config.h"
#include "TunHandler.h"
#include "Proto.h"

#include <iostream>

/**
tun 操作函数测试
功能为读取tun网卡icmp包修改后回应
测试 ： ping.exe 192.168.33.1 可接受响应包
*/
int main()
{
	TunHandler * Handle = TunHandler::Instance();
	
	Handle->Start("10.33.33.34", "255.255.255.0");

	Handle->ConfigDHCP("10.33.33.34", "255.255.255.0", "10.33.33.33");

	Handle->AddRoute("192.168.33.0", "255.255.255.0", "10.33.33.241");
	
	Packet packet;
	while (Handle->Read(packet))
	{
		// 交换 IP 头源目的地址
		IPHDR * IpHdr = reinterpret_cast<IPHDR*>(packet.Buffer);
		
		/*
		// 输出源目的地址
		// 加入打印地址，ping 包由 <1ms 响应时间变为 3ms
		in_addr s;
		in_addr d;
		s.S_un.S_addr = IpHdr->saddr;
		d.S_un.S_addr = IpHdr->daddr;
		std::cout << inet_ntoa(s) << " -> ";
		std::cout << inet_ntoa(d) << std::endl;
		*/
		
		uint32_t Address = IpHdr->saddr;
		IpHdr->saddr = IpHdr->daddr;
		IpHdr->daddr = Address;
		
		// 修改 ICMP 为应答包
		ICMPHDR * IcmpHdr = reinterpret_cast<ICMPHDR*>(packet.Buffer);
		IcmpHdr->type = 0;
		IcmpHdr->check_sum += 8;
		Handle->Write(packet);
	}
	
	Handle->Stop();
	
	return 0;
}
