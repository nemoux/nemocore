#ifndef	__XML_HELPER_H__
#define	__XML_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define	XML_TOKEN_SIZE			(128)
#define	XML_CONTENTS_SIZE		(1024)

#define	XML_NONE_ELEMENT					(0)
#define	XML_START_TAG_ELEMENT			(1)
#define	XML_END_TAG_ELEMENT				(2)
#define	XML_STARTEND_TAG_ELEMENT	(3)
#define	XML_CHARACTERS_ELEMENT		(4)

struct xmlparser {
	char *buffer;
	int max;
	int head, tail;

	void *userdata;
};

extern struct xmlparser *xmlparser_create(int max);
extern void xmlparser_destroy(struct xmlparser *parser);

extern void xmlparser_push(struct xmlparser *parser, const char *msg, int len);
extern void xmlparser_pop(struct xmlparser *parser, int len);

extern int xmlparser_parse(struct xmlparser *parser, char *contents, int *len);

extern int xmlparser_get_type(const char *contents, int len);
extern int xmlparser_get_tag(const char *contents, int len, char *tag);
extern int xmlparser_get_attrs(const char *contents, int len, char (*attrs)[XML_TOKEN_SIZE]);

static inline void xmlparser_set_userdata(struct xmlparser *parser, void *data)
{
	parser->userdata = data;
}

static inline void *xmlparser_get_userdata(struct xmlparser *parser)
{
	return parser->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
