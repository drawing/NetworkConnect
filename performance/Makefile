
ProgDir = ../


CPPFLAGS = -I$(ProgDir) -g
CXX = g++

udp :
	$(CXX) $(CPPFLAGS) SendUDP.cpp $(ProgDir)*.cpp -o udp -lws2_32 -ID:\svn\DEV_2010_11_17 -LD:\svn\ssl_lib -leay32 -lssl32 -lIphlpapi
	
login :
	$(CXX) $(CPPFLAGS) LoginTest.cpp $(ProgDir)*.cpp -o login -lws2_32 -leay32 -lssl32 -lIphlpapi

raw :
	$(CXX) $(CPPFLAGS) rawipSendUDP.cpp -o raw -lws2_32 -ID:\svn\DEV_2010_11_17 -LD:\svn\ssl_lib -leay32 -lssl32 -lIphlpapi