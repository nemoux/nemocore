#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <grilo.h>

#include <nemolog.h>
#include <nemomisc.h>

#define NEMOGRILO_YOUTUBE_KEY			"AIzaSyDLRGH1fK0-hIlkuVOgi96FLeskI_BDj2k"
#define NEMOGRILO_VIMEO_KEY				"4d908c69e05a9d5b5c6669d302f920cb"
#define NEMOGRILO_VIMEO_SECRET		"4a923ffaab6238eb"

typedef enum {
	NEMOGRILO_NONE_OPERATION = 0,
	NEMOGRILO_BROWSE_OPERATION = 1,
	NEMOGRILO_SEARCH_OPERATION = 2,
	NEMOGRILO_QUERY_OPERATION = 3,
	NEMOGRILO_MULTISEARCH_OPERATION = 4,
	NEMOGRILO_LAST_OPERATION
} NemoGriloOperationType;

struct nemogrilo {
	GrlSource *source;

	GList *keys;

	GrlOperationOptions *default_options;
	GrlOperationOptions *default_resolve_options;

	uint32_t index;
};

struct grilostate {
	struct nemogrilo *grilo;

	gint type;

	guint offset;
	guint count;
	gchar *keyword;
};

static void nemogrilo_setup_options(struct nemogrilo *grilo)
{
	grilo->default_options = grl_operation_options_new(NULL);
	grl_operation_options_set_resolution_flags(grilo->default_options, GRL_RESOLVE_FAST_ONLY | GRL_RESOLVE_IDLE_RELAY);
	grl_operation_options_set_skip(grilo->default_options, 0);
	grl_operation_options_set_count(grilo->default_options, 75);

	grilo->default_resolve_options = grl_operation_options_new(NULL);
	grl_operation_options_set_resolution_flags(grilo->default_resolve_options, GRL_RESOLVE_FULL | GRL_RESOLVE_IDLE_RELAY);
}

static void nemogrilo_handle_source_added(GrlRegistry *registry, GrlSource *source, gpointer userdata)
{
	nemolog_message("GRILO", "added source available: %s (%s)\n",
			grl_source_get_name(source),
			grl_source_get_description(source));
}

static void nemogrilo_handle_source_removed(GrlRegistry *registry, GrlSource *source, gpointer userdata)
{
	nemolog_message("GRILO", "removed source available: %s\n",
			grl_source_get_name(source));
}

static void nemogrilo_handle_metadata_key_added(GrlRegistry *registry, const gchar *key, gpointer userdata)
{
}

static void nemogrilo_load_plugin(struct nemogrilo *grilo)
{
	GrlRegistry *registry;

	registry = grl_registry_get_default();
	g_signal_connect(registry, "source-added", G_CALLBACK(nemogrilo_handle_source_added), NULL);
	g_signal_connect(registry, "source-removed", G_CALLBACK(nemogrilo_handle_source_removed), NULL);
	g_signal_connect(registry, "metadata-key-added", G_CALLBACK(nemogrilo_handle_metadata_key_added), NULL);

	grl_registry_load_all_plugins(registry, NULL);
}

static void nemogrilo_load_file_config(struct nemogrilo *grilo)
{
	GrlRegistry *registry;
	gchar *configfile;

	registry = grl_registry_get_default();

	configfile = g_strconcat(g_get_user_config_dir(),
			G_DIR_SEPARATOR_S, "grilo.conf",
			NULL);

	if (g_file_test(configfile, G_FILE_TEST_EXISTS)) {
		grl_registry_add_config_from_file(registry, configfile, NULL);
	}

	g_free(configfile);
}

static void nemogrilo_set_youtube_config(struct nemogrilo *grilo)
{
	GrlConfig *config;
	GrlRegistry *registry;

	config = grl_config_new("grl-youtube", NULL);
	grl_config_set_api_key(config, NEMOGRILO_YOUTUBE_KEY);

	registry = grl_registry_get_default();
	grl_registry_add_config(registry, config, NULL);
}

static void nemogrilo_set_vimeo_config(struct nemogrilo *grilo)
{
	GrlConfig *config;
	GrlRegistry *registry;

	config = grl_config_new("grl-vimeo", NULL);
	grl_config_set_api_key(config, NEMOGRILO_VIMEO_KEY);
	grl_config_set_api_secret(config, NEMOGRILO_VIMEO_SECRET);

	registry = grl_registry_get_default();
	grl_registry_add_config(registry, config, NULL);
}

static GrlSource *nemogrilo_get_source_by_name(struct nemogrilo *grilo, const char *name)
{
	GrlRegistry *registry;
	GrlSource *source;
	GList *sources = NULL;
	GList *iter;

	registry = grl_registry_get_default();
	sources = grl_registry_get_sources_by_operations(registry, GRL_OP_SEARCH, FALSE);

	for (iter = sources; iter; iter = g_list_next(iter)) {
		source = GRL_SOURCE(iter->data);

		if (strcasecmp(grl_source_get_name(source), name) == 0)
			return source;
	}

	return NULL;
}

static void nemogrilo_setup_keys(struct nemogrilo *grilo)
{
	GrlRegistry *registry;

	registry = grl_registry_get_default();
	grilo->keys = grl_registry_get_metadata_keys(registry);
}

static gchar *nemogrilo_string_value_description(const GValue *value)
{
	if (value == NULL)
		return g_strdup("");

	if (G_VALUE_HOLDS_BOXED(value) &&
			G_VALUE_TYPE(value) == G_TYPE_DATE_TIME) {
		GDateTime *dtime = g_value_get_boxed(value);

		return g_date_time_format(dtime, "%FT%H:%M:%SZ");
	} else if (G_VALUE_HOLDS_STRING(value)) {
		return g_value_dup_string(value);
	}

	return g_strdup_value_contents(value);
}

static gchar *nemogrilo_compose_value_description(GList *values)
{
	gint length = g_list_length(values);
	GString *composed = g_string_new("");

	while (values) {
		composed = g_string_append(composed,
				nemogrilo_string_value_description((GValue *)values->data));
		if (--length > 0) {
			composed = g_string_append(composed, ", ");
		}

		values = g_list_next(values);
	}

	return g_string_free(composed, FALSE);
}

static void nemogrilo_dispatch_resolve_callback(GrlSource *source, guint id, GrlMedia *media, gpointer userdata, const GError *error)
{
	struct nemogrilo *grilo = (struct nemogrilo *)userdata;
	GList *keys, *iter;
	GrlKeyID key;
	const gchar *keyname;
	const gchar *mediaurl;

	if (error != NULL) {
		if (g_error_matches(error, GRL_CORE_ERROR, GRL_CORE_ERROR_OPERATION_CANCELLED)) {
			nemolog_message("GRILO", "RESOLVE[-]: cancelled\n");
		} else {
			nemolog_message("GRILO", "RESOLVE[-]: %s\n", error->message);
		}

		return;
	}

	if (media != NULL) {
		keys = grl_data_get_keys(GRL_DATA(media));

		for (iter = keys; iter; iter = g_list_next(iter)) {
			key = GRLPOINTER_TO_KEYID(iter->data);
			keyname = grl_metadata_key_get_name(key);

			if (grl_data_has_key(GRL_DATA(media), key)) {
				GList *values;
				gchar *composed;

				values = grl_data_get_single_values_for_key(GRL_DATA(media), key);
				composed = nemogrilo_compose_value_description(values);
				g_list_free(values);

				nemolog_message("GRILO", "RESOLVE[%d]: %s > %s\n", grilo->index, keyname, composed);
			}
		}

		g_list_free(keys);

		if (GRL_IS_MEDIA_AUDIO(media) ||
				GRL_IS_MEDIA_VIDEO(media) ||
				GRL_IS_MEDIA_IMAGE(media)) {
			mediaurl = grl_media_get_url(media);

			if (mediaurl != NULL) {
				nemolog_message("GRILO", "RESOLVE[%d]: url(%s)\n", grilo->index, mediaurl);
			}
		}
	}
}

static void nemogrilo_dispatch_search_callback(GrlSource *source, guint id, GrlMedia *media, guint remaining, gpointer userdata, const GError *error)
{
	struct grilostate *state = (struct grilostate *)userdata;
	struct nemogrilo *grilo = state->grilo;
	const gchar *name;
	guint nextid;

	if (error != NULL) {
		if (g_error_matches(error, GRL_CORE_ERROR, GRL_CORE_ERROR_OPERATION_CANCELLED)) {
			nemolog_message("GRILO", "SEARCH[-]: cancelled\n");
		} else {
			nemolog_message("GRILO", "SEARCH[-]: %s\n", error->message);
		}

		return;
	}

	state->count++;

	if (media != NULL) {
		name = grl_media_get_title(media);

		if (GRL_IS_MEDIA_BOX(media)) {
			nemolog_message("GRILO", "SEARCH[%d]: mediabox (%s) %d\n", state->count + state->offset, name, grl_media_box_get_childcount(GRL_MEDIA_BOX(media)));
		} else {
			nemolog_message("GRILO", "SEARCH[%d]: media (%s)\n", state->count + state->offset, name);
		}

		if (state->count + state->offset == grilo->index) {
			grl_source_resolve(grilo->source,
					media,
					grilo->keys,
					grilo->default_resolve_options,
					nemogrilo_dispatch_resolve_callback,
					grilo);
		}
	}

	if (remaining == 0) {
		GrlOperationOptions *options;
		GrlOperationOptions *supported_options = NULL;

		state->offset += state->count;
		state->count = 0;

		options = grl_operation_options_copy(grilo->default_options);
		grl_operation_options_set_type_filter(options, GRL_TYPE_FILTER_AUDIO | GRL_TYPE_FILTER_VIDEO | GRL_TYPE_FILTER_IMAGE);

		grl_operation_options_set_skip(options, state->offset);
		grl_operation_options_set_count(options, 75);

		grl_operation_options_obey_caps(options,
				grl_source_get_caps(GRL_SOURCE(grilo->source), GRL_OP_SEARCH),
				&supported_options,
				NULL);

		switch (state->type) {
			case NEMOGRILO_SEARCH_OPERATION:
				nextid = grl_source_search(grilo->source,
						state->keyword,
						grilo->keys,
						supported_options,
						nemogrilo_dispatch_search_callback,
						state);
				break;

			default:
				break;
		}
	}
}

static int nemogrilo_search_keyword(struct nemogrilo *grilo, const char *keyword)
{
	struct grilostate *state;
	guint id;
	GrlOperationOptions *options;
	GrlOperationOptions *supported_options = NULL;

	state = (struct grilostate *)malloc(sizeof(struct grilostate));
	if (state == NULL)
		return -1;
	memset(state, 0, sizeof(struct grilostate));

	state->grilo = grilo;

	options = grl_operation_options_copy(grilo->default_options);
	grl_operation_options_set_type_filter(options, GRL_TYPE_FILTER_AUDIO | GRL_TYPE_FILTER_VIDEO | GRL_TYPE_FILTER_IMAGE);

	state->type = NEMOGRILO_SEARCH_OPERATION;
	state->keyword = strdup(keyword);

	grl_operation_options_obey_caps(options,
			grl_source_get_caps(GRL_SOURCE(grilo->source), GRL_OP_SEARCH),
			&supported_options,
			NULL);

	g_object_unref(options);

	id = grl_source_search(grilo->source,
			keyword,
			grilo->keys,
			supported_options,
			nemogrilo_dispatch_search_callback,
			state);

	g_object_unref(supported_options);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "service",			required_argument,		NULL,		's' },
		{ "keyword",			required_argument,		NULL,		'k' },
		{ "index",				required_argument,		NULL,		'i' },
		{ 0 }
	};
	GMainLoop *gmainloop;
	struct nemogrilo *grilo;
	char *service = NULL;
	char *keyword = NULL;
	uint32_t index = 0;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "s:k:i:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				service = strdup(optarg);
				break;

			case 'k':
				keyword = strdup(optarg);
				break;

			case 'i':
				index = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	if (keyword == NULL)
		return 0;
	if (service == NULL)
		service = strdup("youtube");

	grl_init(&argc, &argv);

	grilo = (struct nemogrilo *)malloc(sizeof(struct nemogrilo));
	if (grilo == NULL)
		return -1;
	memset(grilo, 0, sizeof(struct nemogrilo));

	grilo->index = index;

	nemogrilo_setup_options(grilo);
	nemogrilo_load_file_config(grilo);
	nemogrilo_set_youtube_config(grilo);
	nemogrilo_set_vimeo_config(grilo);
	nemogrilo_load_plugin(grilo);
	nemogrilo_setup_keys(grilo);

	grilo->source = nemogrilo_get_source_by_name(grilo, service);

	nemogrilo_search_keyword(grilo, keyword);

	gmainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gmainloop);
	g_main_loop_unref(gmainloop);

	grl_deinit();

	free(service);
	free(keyword);

	free(grilo);

	return 0;
}
