
#ifdef WIN32
	#include <Winsock2.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string>

int main(int argc, char ** argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage:\n\t%s address port\n", argv[0]);
		exit(0);
	}
	sockaddr_in self;
	self.sin_family = AF_INET;
	self.sin_port = htons(atoi(argv[2]));
	self.sin_addr.s_addr = inet_addr(argv[1]);

	int sock = (int)socket(AF_INET, SOCK_STREAM, 0);
	int err = bind(sock, (sockaddr*)&self, sizeof self);
	if (err < 0){
		perror("bind failed");
		exit(0);
	}

	err = listen(sock, 1);
	if (err < 0){
		perror("listen failed");
		exit(0);
	}

	sockaddr_in target;
	socklen_t socklen = sizeof target;
	int client = accept(sock, (sockaddr*)&target, &socklen);
	if (client < 0){
		perror("accept failed");
		exit(0);
	}
	printf("vpn online:\n");

	while (true) {
		printf("input type:");
		int type;
		scanf("%d", &type);
		printf("input xml:");
		char line[255] = {};
		std::string xml = "";
		while (fgets(line, 255, stdin) != NULL) {
			xml += line;
		}
		uint8_t cType = type;
		uint16_t wLen = htons(xml.size());
		write(client, &cType, sizeof cType);
		write(client, &wLen, sizeof wLen);
		int sendBytes = write(client, xml.data(), xml.size());
		printf("sendBytes=%d\n", sendBytes);
	}
	close(sock);

	return 0;
}
