#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoyoyo.h>
#include <yoyoone.h>
#include <yoyosweep.h>
#include <yoyoactor.h>
#include <nemojson.h>
#include <nemofs.h>
#include <nemomisc.h>

int nemoyoyo_load_contents(struct nemoyoyo *yoyo)
{
	const char *sweepurl;

	sweepurl = nemojson_search_string(yoyo->config, 0, NULL, 2, "sweep", "url");
	if (sweepurl != NULL) {
		struct cooktex *tex;
		struct fsdir *contents;
		int i;

		contents = nemofs_dir_create(128);
		nemofs_dir_scan_extensions(contents, sweepurl, 3, "png", "jpg", "jpeg");

		yoyo->sweeps = (struct cooktex **)malloc(sizeof(struct cooktex *) * nemofs_dir_get_filecount(contents));
		yoyo->nsweeps = nemofs_dir_get_filecount(contents);

		for (i = 0; i < nemofs_dir_get_filecount(contents); i++) {
			tex = yoyo->sweeps[i] = nemocook_texture_create();
			nemocook_texture_assign(tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, 0, 0);
			nemocook_texture_load_image(tex, nemofs_dir_get_filepath(contents, i));
		}
	}

	return 0;
}

int nemoyoyo_update_one(struct nemoyoyo *yoyo)
{
	struct yoyoone *one;
	int count = 0;

	nemolist_for_each(one, &yoyo->one_list, link) {
		if (nemoyoyo_one_has_no_dirty(one) == 0) {
			nemoyoyo_one_update(yoyo, one);

			count++;
		}
	}

	return count;
}

int nemoyoyo_update_frame(struct nemoyoyo *yoyo)
{
	struct yoyoone *one;
	pixman_region32_t damage;

	pixman_region32_init(&damage);

	nemocook_egl_fetch_damage(yoyo->egl, &damage);
	nemocook_egl_rotate_damage(yoyo->egl, &yoyo->damage);

	pixman_region32_union(&damage, &damage, &yoyo->damage);

	nemocook_egl_make_current(yoyo->egl);
	nemocook_egl_update_state(yoyo->egl);

	nemocook_egl_use_shader(yoyo->egl, yoyo->shader);

	nemolist_for_each(one, &yoyo->one_list, link) {
		pixman_region32_t region;

		pixman_region32_init(&region);
		pixman_region32_intersect(&region, &one->bounds, &damage);

		if (pixman_region32_not_empty(&region)) {
			float vertices[64 * 8];
			float texcoords[64 * 8];
			int slices[64];
			int count;

			nemocook_shader_set_uniform_matrix4fv(yoyo->shader, 0, nemocook_polygon_get_matrix4fv(one->poly));
			nemocook_shader_set_uniform_1f(yoyo->shader, 1, one->alpha);

			nemocook_polygon_update_state(one->poly);

			nemocook_texture_update_state(one->tex);
			nemocook_texture_bind(one->tex);

			count = nemoyoyo_one_clip_slice(one, &region, vertices, texcoords, slices);

			nemocook_shader_set_attrib_pointer(yoyo->shader, 0, vertices);
			nemocook_shader_set_attrib_pointer(yoyo->shader, 1, texcoords);

			nemocook_draw_arrays(GL_TRIANGLE_FAN, count, slices);

			nemocook_shader_put_attrib_pointer(yoyo->shader, 0);
			nemocook_shader_put_attrib_pointer(yoyo->shader, 1);

			nemocook_texture_unbind(one->tex);
		}

		pixman_region32_fini(&region);
	}

	nemocook_egl_swap_buffers_with_damage(yoyo->egl, &yoyo->damage);

	pixman_region32_fini(&damage);

	return 0;
}

void nemoyoyo_attach_one(struct nemoyoyo *yoyo, struct yoyoone *one)
{
	nemolist_insert(&yoyo->one_list, &one->link);

	nemocook_transform_set_parent(one->trans, yoyo->projection);
}

void nemoyoyo_detach_one(struct nemoyoyo *yoyo, struct yoyoone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);

	nemocook_transform_set_parent(one->trans, NULL);
}

struct yoyoone *nemoyoyo_pick_one(struct nemoyoyo *yoyo, float x, float y)
{
	struct yoyoone *one;
	float sx, sy;

	nemolist_for_each_reverse(one, &yoyo->one_list, link) {
		if (nemoyoyo_one_has_flags(one, NEMOYOYO_ONE_PICK_FLAG) == 0)
			continue;

		nemocook_2d_transform_from_global(one->trans, x, y, &sx, &sy);

		if (0.0f <= sx && sx <= one->geometry.w && 0.0f <= sy && sy <= one->geometry.h)
			return one;
	}

	return NULL;
}

void nemoyoyo_attach_actor(struct nemoyoyo *yoyo, struct yoyoactor *actor)
{
	nemolist_insert_tail(&yoyo->actor_list, &actor->link);
}

void nemoyoyo_detach_actor(struct nemoyoyo *yoyo, struct yoyoactor *actor)
{
	nemolist_remove(&actor->link);
	nemolist_init(&actor->link);
}

int nemoyoyo_overlap_actor(struct nemoyoyo *yoyo, float x, float y)
{
	struct yoyoactor *actor;
	float dx, dy;

	nemolist_for_each(actor, &yoyo->actor_list, link) {
		dx = actor->geometry.x - x;
		dy = actor->geometry.y - y;

		if (sqrtf(dx * dx + dy * dy) < 300.0f)
			return 1;
	}

	return 0;
}

struct cooktex *nemoyoyo_search_tex(struct nemoyoyo *yoyo, const char *path)
{
	struct cooktex *tex;

	tex = (struct cooktex *)nemodick_search(yoyo->textures, path);
	if (tex != NULL)
		return tex;

	tex = nemocook_texture_create();
	nemocook_texture_assign(tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, 0, 0);

	if (nemocook_texture_load_image(tex, path) < 0) {
		nemocook_texture_destroy(tex);
		return NULL;
	}

	nemodick_insert(yoyo->textures, path, tex);

	return tex;
}
