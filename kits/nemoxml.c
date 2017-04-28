#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <expat.h>

#include <nemoxml.h>
#include <nemomisc.h>

struct nemoxml *nemoxml_create(void)
{
	struct nemoxml *xml;

	xml = (struct nemoxml *)malloc(sizeof(struct nemoxml));
	if (xml == NULL)
		return NULL;
	memset(xml, 0, sizeof(struct nemoxml));

	nemolist_init(&xml->children);
	nemolist_init(&xml->nodes);

	return xml;
}

void nemoxml_destroy(struct nemoxml *xml)
{
	struct xmlnode *child, *nchild;

	nemolist_for_each_safe(child, nchild, &xml->children, link) {
		nemoxml_destroy_node(child);
	}

	nemolist_remove(&xml->children);
	nemolist_remove(&xml->nodes);

	free(xml);
}

static void nemoxml_update_children(struct nemoxml *xml, struct nemolist *children, char *path)
{
	struct xmlnode *node;

	nemolist_for_each(node, children, link) {
		nemolist_remove(&node->nodelink);
		nemolist_insert_tail(&xml->nodes, &node->nodelink);

		strcpy(node->path, path);
		strcat(node->path, "/");
		strcat(node->path, node->name);

		nemoxml_update_children(xml, &node->children, node->path);
	}
}

void nemoxml_update(struct nemoxml *xml)
{
	nemolist_remove(&xml->nodes);
	nemolist_init(&xml->nodes);

	nemoxml_update_children(xml, &xml->children, "");
}

struct xmlnode *nemoxml_create_node(void)
{
	struct xmlnode *node;

	node = (struct xmlnode *)malloc(sizeof(struct xmlnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct xmlnode));

	node->parent = NULL;
	node->nattrs = 0;
	node->contents = NULL;
	node->ncontents = 0;

	nemolist_init(&node->link);
	nemolist_init(&node->children);
	nemolist_init(&node->nodelink);

	return node;
}

void nemoxml_destroy_node(struct xmlnode *node)
{
	struct xmlnode *child, *nchild;
	int i;

	for (i = 0; i < node->nattrs; i++) {
		free(node->attrs[i*2+0]);
		free(node->attrs[i*2+1]);
	}

	if (node->contents != NULL) {
		free(node->contents);
	}

	nemolist_for_each_safe(child, nchild, &node->children, link) {
		nemoxml_destroy_node(child);
	}

	nemolist_remove(&node->link);
	nemolist_remove(&node->children);
	nemolist_remove(&node->nodelink);

	free(node);
}

struct xmlcontext {
	XML_Parser parser;

	struct nemoxml *xml;
	struct xmlnode *node;
};

static void nemoxml_handle_sax_start_element(void *data, const char *name, const char **attrs)
{
	struct xmlcontext *context = (struct xmlcontext *)data;
	struct xmlnode *node;
	int i;

	if (context == NULL)
		return;

	node = nemoxml_create_node();
	if (node == NULL)
		return;
	strcpy(node->name, name);

	for (i = 0; attrs[i*2+0] != NULL && i < NEMOXML_ATTRS_MAX; i++) {
		node->attrs[i*2+0] = strdup(attrs[i*2+0]);
		node->attrs[i*2+1] = strdup(attrs[i*2+1]);

		node->nattrs++;
	}

	if (context->node == NULL) {
		node->level = 0;

		nemolist_insert_tail(&context->xml->children, &node->link);
	} else {
		node->parent = context->node;
		node->level = context->node->level + 1;

		nemolist_insert_tail(&context->node->children, &node->link);
	}

	context->node = node;
}

static void nemoxml_handle_sax_end_element(void *data, const char *name)
{
	struct xmlcontext *context = (struct xmlcontext *)data;

	if (context == NULL || context->node == NULL)
		return;

	context->node = context->node->parent;
}

static void nemoxml_handle_sax_characters(void *data, const char *contents, int length)
{
	struct xmlcontext *context = (struct xmlcontext *)data;

	if (context == NULL || context->node == NULL)
		return;

	if (context->node->contents == NULL) {
		context->node->contents = (char *)malloc(length + 1);
		if (context->node->contents == NULL)
			return;
		context->node->ncontents = 0;
		context->node->scontents = length + 1;
	}

	if (context->node->ncontents + length + 1 >= context->node->scontents) {
		context->node->contents = (char *)realloc(context->node->contents, sizeof(char) * (context->node->scontents + length + 1));
		context->node->scontents = context->node->scontents + length + 1;
	}

	memcpy(&context->node->contents[context->node->ncontents], contents, length);
	context->node->contents[context->node->ncontents + length] = '\0';
	context->node->ncontents += length;
}

int nemoxml_load_file(struct nemoxml *xml, const char *filepath)
{
	struct xmlcontext context;
	char *buffer = NULL;
	int length;

	context.parser = XML_ParserCreate(NULL);
	if (context.parser == NULL)
		return -1;
	context.xml = xml;
	context.node = NULL;

	XML_SetUserData(context.parser, &context);

	XML_SetStartElementHandler(context.parser, nemoxml_handle_sax_start_element);
	XML_SetEndElementHandler(context.parser, nemoxml_handle_sax_end_element);
	XML_SetCharacterDataHandler(context.parser, nemoxml_handle_sax_characters);

	if ((length = os_file_load(filepath, &buffer)) >= 0) {
		XML_Parse(context.parser, buffer, length, 0);

		free(buffer);
	}

	XML_ParserFree(context.parser);

	return 0;
}

int nemoxml_load_data(struct nemoxml *xml, const char *buffer, int length)
{
	struct xmlcontext context;

	context.parser = XML_ParserCreate(NULL);
	if (context.parser == NULL)
		return -1;
	context.xml = xml;
	context.node = NULL;

	XML_SetUserData(context.parser, &context);

	XML_SetStartElementHandler(context.parser, nemoxml_handle_sax_start_element);
	XML_SetEndElementHandler(context.parser, nemoxml_handle_sax_end_element);
	XML_SetCharacterDataHandler(context.parser, nemoxml_handle_sax_characters);

	XML_Parse(context.parser, buffer, length, 0);

	XML_ParserFree(context.parser);

	return 0;
}

void nemoxml_node_set_name(struct xmlnode *node, const char *name)
{
	strcpy(node->name, name);
}

int nemoxml_node_set_attr(struct xmlnode *node, const char *attr, const char *value)
{
	int i;

	for (i = 0; i < node->nattrs; i++) {
		if (strcmp(node->attrs[i*2+0], attr) == 0) {
			free(node->attrs[i*2+1]);

			node->attrs[i*2+1] = strdup(value);

			return 1;
		}
	}

	if (node->nattrs < NEMOXML_ATTRS_MAX - 1) {
		node->attrs[node->nattrs*2+0] = strdup(attr);
		node->attrs[node->nattrs*2+1] = strdup(value);

		node->nattrs++;

		return 1;
	}

	return 0;
}

char *nemoxml_node_get_attr(struct xmlnode *node, const char *attr)
{
	int i;

	for (i = 0; i < node->nattrs; i++) {
		if (strcmp(node->attrs[i*2+0], attr) == 0) {
			return node->attrs[i*2+1];
		}
	}

	return NULL;
}

void nemoxml_node_set_contents(struct xmlnode *node, const char *contents, int length)
{
	if (node->contents != NULL) {
		free(node->contents);
	}

	node->contents = (char *)malloc(length + 1);
	if (node->contents == NULL)
		return;

	memcpy(node->contents, contents, length);
	node->contents[length] = '\0';
}

void nemoxml_link_node(struct nemoxml *xml, struct xmlnode *node)
{
	node->level = 0;

	nemolist_insert_tail(&xml->children, &node->link);
}

void nemoxml_unlink_node(struct nemoxml *xml, struct xmlnode *node)
{
	nemolist_remove(&node->link);
	nemolist_init(&node->link);
}

void nemoxml_node_link_node(struct xmlnode *node, struct xmlnode *child)
{
	child->level = node->level + 1;

	nemolist_insert_tail(&node->children, &child->link);
}

void nemoxml_node_unlink_node(struct xmlnode *node, struct xmlnode *child)
{
	nemolist_remove(&child->link);
	nemolist_init(&child->link);
}

int nemoxml_select_node(struct nemoxml *xml, struct xmlnode **nodes, int length, const char *name)
{
	struct xmlnode *node;
	int count = 0;

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if (strcmp(node->name, name) == 0) {
			nodes[count++] = node;

			if (count >= length)
				return count;
		}
	}

	return count;
}

int nemoxml_select_node_by_path(struct nemoxml *xml, struct xmlnode **nodes, int length, const char *path)
{
	struct xmlnode *node;
	int count = 0;

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if (strcmp(node->path, path) == 0) {
			nodes[count++] = node;

			if (count >= length)
				return count;
		}
	}

	return count;
}

int nemoxml_select_node_by_attr(struct nemoxml *xml, struct xmlnode **nodes, int length, const char *name, const char *attr, const char *value)
{
	struct xmlnode *node;
	int count = 0;
	int i;

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if (strcmp(node->name, name) == 0) {
			for (i = 0; i < node->nattrs; i++) {
				if (strcmp(node->attrs[i*2+0], attr) == 0) {
					if (strcmp(node->attrs[i*2+1], value) == 0) {
						nodes[count++] = node;

						if (count >= length)
							return count;
					}

					break;
				}
			}
		}
	}

	return count;
}
