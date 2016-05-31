#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomsg.h>
#include <msgqueue.h>
#include <udphelper.h>
#include <nemomisc.h>

struct nemomsg *nemomsg_create(const char *ip, int port)
{
	struct nemomsg *msg;

	msg = (struct nemomsg *)malloc(sizeof(struct nemomsg));
	if (msg == NULL)
		return NULL;
	memset(msg, 0, sizeof(struct nemomsg));

	msg->soc = udp_create_socket(ip, port);
	if (msg->soc < 0)
		goto err1;

	msg->queue = nemomsg_queue_create();
	if (msg->queue == NULL)
		goto err2;

	msg->conn = nemomsg_conn_create();
	if (msg->conn == NULL)
		goto err3;
	nemomsg_conn_set_socket(msg->conn, msg->soc);

	return msg;

err3:
	nemomsg_queue_destroy(msg->queue);

err2:
	close(msg->soc);

err1:
	free(msg);

	return NULL;
}

void nemomsg_destroy(struct nemomsg *msg)
{
	nemomsg_queue_destroy(msg->queue);
	nemomsg_conn_destroy(msg->conn);

	close(msg->soc);

	free(msg);
}
