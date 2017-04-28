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

#include <tuio.h>
#include <touch.h>
#include <compz.h>
#include <screen.h>
#include <oschelper.h>
#include <xmlhelper.h>
#include <nemomisc.h>
#include <nemolog.h>

static struct tuiotap *tuio_find_tap(struct tuio *tuio, int id)
{
	int i;

	for (i = 0; i < tuio->alive.index; i++) {
		if (tuio->taps[i].id == id)
			return &tuio->taps[i];
	}

	return NULL;
}

static void tuio_handle_xml_argument(struct tuio *tuio, const char *type, const char *value)
{
	if (type[0] == 's') {
		if (value[0] == 'a') {
			tuio->state = NEMOTUIO_ALIVE_STATE;
			tuio->alive.index = 0;
		} else if (value[0] == 's') {
			tuio->state = NEMOTUIO_SET_STATE;
		} else if (value[0] == 'f') {
			tuio->state = NEMOTUIO_FSEQ_STATE;
		}
	}

	if (type[0] == 'i') {
		if (tuio->state == NEMOTUIO_ALIVE_STATE) {
			tuio->taps[tuio->alive.index].id = strtoul(value, NULL, 10);
			tuio->alive.index++;
		} else if (tuio->state == NEMOTUIO_SET_STATE) {
			tuio->set.id = strtoul(value, NULL, 10);
			tuio->set.tap = tuio_find_tap(tuio, tuio->set.id);
			tuio->set.index = 0;
		} else if (tuio->state == NEMOTUIO_FSEQ_STATE) {
			tuio->fseq.id = strtoul(value, NULL, 10);

			nemotouch_flush_tuio(tuio);
		}
	}

	if (type[0] == 'f') {
		if (tuio->state == NEMOTUIO_SET_STATE) {
			if (tuio->set.index < 5) {
				tuio->set.tap->f[tuio->set.index] = strtof(value, NULL);
				tuio->set.index++;
			}
		}
	}
}

static int tuio_handle_xml_event(struct tuio *tuio, char *tag, char (*attrs)[XML_TOKEN_SIZE], int nattrs)
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
	struct tuio *tuio;
	char msg[512];
	char contents[XML_CONTENTS_SIZE];
	int len, clen;

	tuio = xmlparser_get_userdata(parser);

	len = read(fd, msg, sizeof(msg));
	if (len < 0) {
		if (errno != EAGAIN && errno != EINTR) {
			wl_event_source_remove(tuio->source);
			tuio->source = NULL;
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

			tuio_handle_xml_event(tuio, tag, attrs, nattrs);
		}
	}

	return 1;
}

static int tuio_handle_osc_message(struct tuio *tuio, const char *msg, int len)
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
				if (tuio->state == NEMOTUIO_ALIVE_STATE) {
					tuio->taps[tuio->alive.index].id = osc_int32(arg);
					tuio->alive.index++;
				} else if (tuio->state == NEMOTUIO_SET_STATE) {
					tuio->set.id = osc_int32(arg);
					tuio->set.tap = tuio_find_tap(tuio, tuio->set.id);
					tuio->set.index = 0;
				} else if (tuio->state == NEMOTUIO_FSEQ_STATE) {
					tuio->fseq.id = osc_int32(arg);

					nemotouch_flush_tuio(tuio);
				}

				arg += 4;
				break;

			case OSC_FLOAT_TAG:
				if (tuio->state == NEMOTUIO_SET_STATE) {
					if (tuio->set.index < 5) {
						tuio->set.tap->f[tuio->set.index] = osc_float(arg);
						tuio->set.index++;
					}
				}

				arg += 4;
				break;

			case OSC_STRING_TAG:
				if (arg[0] == 'a') {
					tuio->state = NEMOTUIO_ALIVE_STATE;
					tuio->alive.index = 0;
				} else if (arg[0] == 's') {
					tuio->state = NEMOTUIO_SET_STATE;
				} else if (arg[0] == 'f') {
					tuio->state = NEMOTUIO_FSEQ_STATE;
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

static int tuio_handle_osc_event(struct tuio *tuio, const char *msg, int len)
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

		tuio_handle_osc_message(tuio, p + 4, size);

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
	struct tuio *tuio = (struct tuio *)data;
	char msg[512];
	int len;

	do {
		len = read(fd, msg, sizeof(msg));
		if (len < 0) {
			if (errno != EAGAIN && errno != EINTR) {
				wl_event_source_remove(tuio->source);
				tuio->source = NULL;
			}

			return 1;
		}

		if (len < 16 || (len & 0x3) != 0)
			break;

		if (msg[0] != '#' || msg[1] != 'b' || msg[2] != 'u' ||
				msg[3] != 'n' || msg[4] != 'd' || msg[5] != 'l' ||
				msg[6] != 'e' || msg[7] != '\0')
			break;

		tuio_handle_osc_event(tuio, msg, len);
	} while (len > 0);

	return 1;
}

static int tuio_prepare_xml(struct tuio *tuio, int port)
{
	struct xmlparser *parser;
	struct sockaddr_in addr;
	int r;

	tuio->fd = socket(AF_INET, SOCK_STREAM, 0);
	if (tuio->fd < 0)
		return -1;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(port);

	r = connect(tuio->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (r < 0)
		goto err1;

	os_fd_set_nonblocking_mode(tuio->fd);

	parser = xmlparser_create(8192);
	if (parser == NULL)
		goto err1;

	tuio->source = wl_event_loop_add_fd(tuio->compz->loop,
			tuio->fd,
			WL_EVENT_READABLE,
			tuio_dispatch_xml_event,
			parser);
	if (tuio->source == NULL)
		goto err2;

	xmlparser_set_userdata(parser, tuio);
	tuio->xmlparser = parser;

	return 0;

err2:
	xmlparser_destroy(parser);

err1:
	close(tuio->fd);
	tuio->fd = 0;

	return -1;
}

static void tuio_finish_xml(struct tuio *tuio)
{
	if (tuio->xmlparser != NULL)
		xmlparser_destroy(tuio->xmlparser);
	if (tuio->source != NULL)
		wl_event_source_remove(tuio->source);
	if (tuio->fd > 0)
		close(tuio->fd);
}

static int tuio_prepare_osc(struct tuio *tuio, int port)
{
	struct sockaddr_in addr;
	socklen_t len;
	int r;

	tuio->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (tuio->fd < 0)
		return -1;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(port);

	r = bind(tuio->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (r < 0)
		goto err1;

	os_fd_set_nonblocking_mode(tuio->fd);

	tuio->source = wl_event_loop_add_fd(tuio->compz->loop,
			tuio->fd,
			WL_EVENT_READABLE,
			tuio_dispatch_osc_event,
			tuio);
	if (tuio->source == NULL)
		goto err1;

	return 0;

err1:
	close(tuio->fd);
	tuio->fd = 0;

	return -1;
}

static void tuio_finish_osc(struct tuio *tuio)
{
	if (tuio->source != NULL)
		wl_event_source_remove(tuio->source);
	if (tuio->fd > 0)
		close(tuio->fd);
}

struct tuio *tuio_create(struct nemocompz *compz, int protocol, int port, int max)
{
	struct tuio *tuio;
	uint32_t tuioid, screenid;

	tuio = (struct tuio *)malloc(sizeof(struct tuio));
	if (tuio == NULL)
		return NULL;
	memset(tuio, 0, sizeof(struct tuio));

	tuio->compz = compz;

	wl_list_init(&tuio->link);
	wl_list_init(&tuio->base.link);

	tuio->taps = (struct tuiotap *)malloc(sizeof(struct tuiotap) * max);
	if (tuio->taps == NULL)
		goto err1;

	tuio->touch = nemotouch_create(compz->seat, &tuio->base);
	if (tuio->touch == NULL)
		goto err2;

	if (protocol == NEMOTUIO_XML_PROTOCOL) {
		asprintf(&tuio->base.devnode, "xml:%d", port);
	} else if (protocol == NEMOTUIO_OSC_PROTOCOL) {
		asprintf(&tuio->base.devnode, "osc:%d", port);
	}

	tuio->base.type |= NEMOINPUT_TOUCH_TYPE;

	nemoinput_set_size(&tuio->base,
			nemocompz_get_scene_width(compz),
			nemocompz_get_scene_height(compz));

	if (protocol == NEMOTUIO_XML_PROTOCOL) {
		if (tuio_prepare_xml(tuio, port) < 0)
			goto err3;
	} else if (protocol == NEMOTUIO_OSC_PROTOCOL) {
		if (tuio_prepare_osc(tuio, port) < 0)
			goto err3;
	}

	wl_list_insert(compz->tuio_list.prev, &tuio->link);
	wl_list_insert(compz->input_list.prev, &tuio->base.link);

	return tuio;

err3:
	nemotouch_destroy(tuio->touch);

err2:
	free(tuio->taps);

err1:
	free(tuio);

	return NULL;
}

void tuio_destroy(struct tuio *tuio)
{
	wl_list_remove(&tuio->link);
	wl_list_remove(&tuio->base.link);

	if (tuio->protocol == NEMOTUIO_XML_PROTOCOL) {
		tuio_finish_xml(tuio);
	} else if (tuio->protocol == NEMOTUIO_OSC_PROTOCOL) {
		tuio_finish_osc(tuio);
	}

	if (tuio->base.screen != NULL)
		nemoinput_put_screen(&tuio->base);

	if (tuio->touch != NULL)
		nemotouch_destroy(tuio->touch);

	if (tuio->taps != NULL)
		free(tuio->taps);

	if (tuio->base.devnode != NULL)
		free(tuio->base.devnode);

	free(tuio);
}
