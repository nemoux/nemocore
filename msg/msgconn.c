#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <msgconn.h>
#include <udphelper.h>
#include <nemomisc.h>

struct msgconn *nemomsg_conn_create(void)
{
	struct msgconn *conn;

	conn = (struct msgconn *)malloc(sizeof(struct msgconn));
	if (conn == NULL)
		return NULL;
	memset(conn, 0, sizeof(struct msgconn));

	nemolist_init(&conn->client_list);

	return conn;
}

void nemomsg_conn_destroy(struct msgconn *conn)
{
	struct msgclient *client, *next;

	nemolist_for_each_safe(client, next, &conn->client_list, link) {
		nemolist_remove(&client->link);

		free(client->name);
		free(client->ip);
		free(client);
	}

	free(conn);
}

int nemomsg_conn_set_client(struct msgconn *conn, const char *name, const char *ip, int port)
{
	struct msgclient *client;

	client = (struct msgclient *)malloc(sizeof(struct msgclient));
	if (client == NULL)
		return -1;

	client->name = strdup(name);
	client->ip = strdup(ip);
	client->port = port;

	nemolist_insert_tail(&conn->client_list, &client->link);

	return 0;
}

int nemomsg_conn_put_client(struct msgconn *conn, const char *name, const char *ip, int port)
{
	struct msgclient *client;

	nemolist_for_each(client, &conn->client_list, link) {
		if (strcmp(client->name, name) == 0 &&
				strcmp(client->ip, ip) == 0 &&
				client->port == port) {
			nemolist_remove(&client->link);

			free(client->name);
			free(client->ip);
			free(client);

			break;
		}
	}

	return 0;
}

int nemomsg_conn_send_msg(struct msgconn *conn, const char *name, const char *msg, int size)
{
	struct msgclient *client;

	nemolist_for_each(client, &conn->client_list, link) {
		if (strcmp(client->name, name) == 0) {
			udp_send_to(conn->soc, client->ip, client->port, msg, size);
		}
	}

	return 0;
}
