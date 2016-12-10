#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <udphelper.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

int udp_create_socket(const char *ip, int port)
{
	int soc;

	soc = socket(PF_INET, SOCK_DGRAM, 0);
	if (soc < 0)
		return -1;

	if (ip != NULL && port > 0) {
		struct sockaddr_in addr;

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);

		if (inet_aton(ip, &addr.sin_addr) == 0)
			goto err1;

		if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			goto err1;
	}

	return soc;

err1:
	close(soc);

	return -1;
}

int udp_send_to(int soc, const char *ip, int port, const char *msg, int size)
{
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (inet_aton(ip, &addr.sin_addr) == 0)
		return -1;

	return sendto(soc, msg, size, 0, (struct sockaddr *)&addr, sizeof(addr));
}

int udp_recv_from(int soc, char *ip, int *port, char *msg, int size)
{
	struct sockaddr_in addr;
	int addrsize = sizeof(addr);
	int r;

	r = recvfrom(soc, msg, size, 0, (struct sockaddr *)&addr, &addrsize);
	if (r > 0) {
		char *_ip;

		_ip = inet_ntoa(addr.sin_addr);
		if (_ip == NULL)
			return -1;
		strcpy(ip, _ip);

		*port = ntohs(addr.sin_port);
	}

	return r;
}
