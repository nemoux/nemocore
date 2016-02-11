#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <wayland-server.h>

#include <tuionode.h>
#include <touch.h>
#include <compz.h>
#include <screen.h>
#include <oschelper.h>
#include <xmlparser.h>
#include <oshelper.h>
#include <nemomisc.h>
#include <nemolog.h>

static struct tuiotap *tuio_find_tap(struct tuionode *node, int id)
{
	int i;

	for (i = 0; i < node->alive.index; i++) {
		if (node->taps[i].id == id)
			return &node->taps[i];
	}

	return NULL;
}

static void tuio_handle_xml_argument(struct tuionode *node, const char *type, const char *value)
{
	if (type[0] == 's') {
		if (value[0] == 'a') {
			node->state = NEMO_TUIO_ALIVE_STATE;
			node->alive.index = 0;
		} else if (value[0] == 's') {
			node->state = NEMO_TUIO_SET_STATE;
		} else if (value[0] == 'f') {
			node->state = NEMO_TUIO_FSEQ_STATE;
		}
	}

	if (type[0] == 'i') {
		if (node->state == NEMO_TUIO_ALIVE_STATE) {
			node->taps[node->alive.index].id = strtoul(value, NULL, 10);
			node->alive.index++;
		} else if (node->state == NEMO_TUIO_SET_STATE) {
			node->set.id = strtoul(value, NULL, 10);
			node->set.tap = tuio_find_tap(node, node->set.id);
			node->set.index = 0;
		} else if (node->state == NEMO_TUIO_FSEQ_STATE) {
			node->fseq.id = strtoul(value, NULL, 10);

			nemotouch_flush_tuio(node);
		}
	}

	if (type[0] == 'f') {
		if (node->state == NEMO_TUIO_SET_STATE) {
			if (node->set.index < 5) {
				node->set.tap->f[node->set.index] = strtof(value, NULL);
				node->set.index++;
			}
		}
	}
}

static int tuio_handle_xml_event(struct tuionode *tuio, char *tag, char (*attrs)[XML_TOKEN_SIZE], int nattrs)
{
	char *type, *value, *name;
	int i;

	for (i = 0; i < nattrs; i++) {
		if (attrs[i*2+0][0] == 'T') {
			type = attrs[i*2+1];
		} else if (attrs[i*2+0][0] == 'V') {
			value = attrs[i*2+1];

			tuio_handle_xml_argument(tuio, type, value);
		} else if (attrs[i*2+0][0] == 'N') {
			name = attrs[i*2+1];
		}
	}
}

static int tuio_dispatch_xml_event(int fd, uint32_t mask, void *data)
{
	struct xmlparser *parser = (struct xmlparser *)data;
	struct tuionode *node;
	char msg[512];
	char contents[XML_CONTENTS_SIZE];
	int len, clen;

	node = xmlparser_get_userdata(parser);

	len = read(fd, msg, sizeof(msg));
	if (len < 0) {
		if (errno != EAGAIN && errno != EINTR) {
			wl_event_source_remove(node->source);
			node->source = NULL;
		}

		return 1;
	}

	xmlparser_push(parser, msg, len);

	while (xmlparser_parse(parser, contents, &clen)) {
		int type = xmlparser_get_type(contents, clen);
		if (type == XML_START_TAG_ELEMENT || type == XML_STARTEND_TAG_ELEMENT) {
			char tag[XML_TOKEN_SIZE];
			char attrs[16][XML_TOKEN_SIZE];
			int nattrs;

			xmlparser_get_tag(contents, clen, tag);
			nattrs = xmlparser_get_attrs(contents, clen, attrs);

			tuio_handle_xml_event(node, tag, attrs, nattrs);
		}
	}

	return 1;
}

static int tuio_handle_osc_message(struct tuionode *node, const char *msg, int len)
{
	const char *end = msg + len;
	const char *p, *arg;

	p = osc_find_str4_end(msg, end);
	if (p[0] != ',')
		return -1;
	if (p[1] == '\0')
		return 0;

	arg = osc_find_str4_end(p, end);
	if (arg == NULL)
		return -1;

	for (p++; *p != '\0'; p++) {
		switch (*p) {
			case OSC_TRUE_TAG:
			case OSC_FALSE_TAG:
			case OSC_NIL_TAG:
			case OSC_INFINITUM_TAG:
				break;

			case OSC_INT32_TAG:
				if (node->state == NEMO_TUIO_ALIVE_STATE) {
					node->taps[node->alive.index].id = osc_int32(arg);
					node->alive.index++;
				} else if (node->state == NEMO_TUIO_SET_STATE) {
					node->set.id = osc_int32(arg);
					node->set.tap = tuio_find_tap(node, node->set.id);
					node->set.index = 0;
				} else if (node->state == NEMO_TUIO_FSEQ_STATE) {
					node->fseq.id = osc_int32(arg);

					nemotouch_flush_tuio(node);
				}

				arg += 4;
				break;

			case OSC_FLOAT_TAG:
				if (node->state == NEMO_TUIO_SET_STATE) {
					if (node->set.index < 5) {
						node->set.tap->f[node->set.index] = osc_float(arg);
						node->set.index++;
					}
				}

				arg += 4;
				break;

			case OSC_STRING_TAG:
				if (arg[0] == 'a') {
					node->state = NEMO_TUIO_ALIVE_STATE;
					node->alive.index = 0;
				} else if (arg[0] == 's') {
					node->state = NEMO_TUIO_SET_STATE;
				} else if (arg[0] == 'f') {
					node->state = NEMO_TUIO_FSEQ_STATE;
				}

				arg = osc_find_str4_end(arg, end);
				break;

			default:
				nemolog_warning("TUIO", "unknown type tag (%c)\n", *p);
				break;
		}
	}

	return 0;
}

static int tuio_handle_osc_event(struct tuionode *node, const char *msg, int len)
{
	const char *timetag;
	const char *end = msg + len;
	const char *p;
	uint32_t size;
	int count = 0;

	timetag = msg + 8;

	p = timetag + 8;

	while (p < end) {
		if (p + 4 > end)
			return -1;

		size = osc_uint32(p);
		if ((size & 0x3) != 0)
			break;

		tuio_handle_osc_message(node, p + 4, size);

		p += 4 + size;
		if (p > end)
			break;

		count++;
	}

	if (p != end)
		return -1;

	return 0;
}

static int tuio_dispatch_osc_event(int fd, uint32_t mask, void *data)
{
	struct tuionode *node = (struct tuionode *)data;
	char msg[512];
	int len;

	do {
		len = read(fd, msg, sizeof(msg));
		if (len < 0) {
			if (errno != EAGAIN && errno != EINTR) {
				wl_event_source_remove(node->source);
				node->source = NULL;
			}

			return 1;
		}

		if (len < 16 || (len & 0x3) != 0)
			break;

		if (msg[0] != '#' || msg[1] != 'b' || msg[2] != 'u' ||
				msg[3] != 'n' || msg[4] != 'd' || msg[5] != 'l' ||
				msg[6] != 'e' || msg[7] != '\0')
			break;

		tuio_handle_osc_event(node, msg, len);
	} while (len > 0);

	return 1;
}

static int tuio_prepare_xml(struct tuionode *node, int port)
{
	struct xmlparser *parser;
	struct sockaddr_in addr;
	int r;

	node->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (node->fd < 0)
		return -1;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(port);

	r = connect(node->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (r < 0)
		goto err1;

	os_set_nonblocking_mode(node->fd);

	parser = xmlparser_create(8192);
	if (parser == NULL)
		goto err1;

	node->source = wl_event_loop_add_fd(node->compz->loop,
			node->fd,
			WL_EVENT_READABLE,
			tuio_dispatch_xml_event,
			parser);
	if (node->source == NULL)
		goto err2;

	xmlparser_set_userdata(parser, node);
	node->xmlparser = parser;

	return 0;

err2:
	xmlparser_destroy(parser);

err1:
	close(node->fd);
	node->fd = 0;

	return -1;
}

static void tuio_finish_xml(struct tuionode *node)
{
	if (node->xmlparser != NULL)
		xmlparser_destroy(node->xmlparser);
	if (node->source != NULL)
		wl_event_source_remove(node->source);
	if (node->fd > 0)
		close(node->fd);
}

static int tuio_prepare_osc(struct tuionode *node, int port)
{
	struct sockaddr_in addr;
	socklen_t len;
	int r;

	node->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (node->fd < 0)
		return -1;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(port);

	r = bind(node->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (r < 0)
		goto err1;

	os_set_nonblocking_mode(node->fd);

	node->source = wl_event_loop_add_fd(node->compz->loop,
			node->fd,
			WL_EVENT_READABLE,
			tuio_dispatch_osc_event,
			node);
	if (node->source == NULL)
		goto err1;

	return 0;

err1:
	close(node->fd);
	node->fd = 0;

	return -1;
}

static void tuio_finish_osc(struct tuionode *node)
{
	if (node->source != NULL)
		wl_event_source_remove(node->source);
	if (node->fd > 0)
		close(node->fd);
}

struct tuionode *tuio_create_node(struct nemocompz *compz, int protocol, int port, int max)
{
	struct tuionode *node;
	uint32_t nodeid, screenid;

	node = (struct tuionode *)malloc(sizeof(struct tuionode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct tuionode));

	node->compz = compz;

	node->taps = (struct tuiotap *)malloc(sizeof(struct tuiotap) * max);
	if (node->taps == NULL)
		goto err1;

	node->touch = nemotouch_create(compz->seat, &node->base);
	if (node->touch == NULL)
		goto err2;

	if (protocol == NEMO_TUIO_XML_PROTOCOL) {
		asprintf(&node->base.devnode, "xml:%d", port);
	} else if (protocol == NEMO_TUIO_OSC_PROTOCOL) {
		asprintf(&node->base.devnode, "osc:%d", port);
	}

	if (nemoinput_get_config_screen(compz, node->base.devnode, &nodeid, &screenid) > 0)
		nemoinput_set_screen(&node->base, nemocompz_get_screen(compz, nodeid, screenid));
	else if (nemoinput_get_config_geometry(compz, node->base.devnode, &node->base) <= 0)
		nemoinput_set_geometry(&node->base,
				0, 0,
				nemocompz_get_scene_width(compz),
				nemocompz_get_scene_height(compz));

	if (protocol == NEMO_TUIO_XML_PROTOCOL) {
		if (tuio_prepare_xml(node, port) < 0)
			goto err4;
	} else if (protocol == NEMO_TUIO_OSC_PROTOCOL) {
		if (tuio_prepare_osc(node, port) < 0)
			goto err4;
	}

	wl_list_insert(compz->tuio_list.prev, &node->link);

	return node;

err4:
	if (node->base.screen != NULL)
		nemoinput_put_screen(&node->base);

err3:
	nemotouch_destroy(node->touch);

err2:
	free(node->taps);

err1:
	free(node);

	return NULL;
}

void tuio_destroy_node(struct tuionode *node)
{
	wl_list_remove(&node->link);

	if (node->protocol == NEMO_TUIO_XML_PROTOCOL) {
		tuio_finish_xml(node);
	} else if (node->protocol == NEMO_TUIO_OSC_PROTOCOL) {
		tuio_finish_osc(node);
	}

	if (node->base.screen != NULL)
		nemoinput_put_screen(&node->base);

	if (node->touch != NULL)
		nemotouch_destroy(node->touch);

	if (node->taps != NULL)
		free(node->taps);

	if (node->base.devnode != NULL)
		free(node->base.devnode);

	free(node);
}
