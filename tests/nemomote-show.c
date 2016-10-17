#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <ctype.h>

#ifdef NEMOUX_WITH_OPENCL
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#endif

#include <nemoshow.h>
#include <showhelper.h>
#include <fbohelper.h>
#include <glhelper.h>
#include <nemohelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#define NEMOMOTE_PARTICLES			(15000)

struct motecontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *view;
	struct showone *sprite;

#ifdef NEMOUX_WITH_OPENCL
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel mutualgravity;
	cl_kernel gravitywell;
	cl_kernel boundingbox;
	cl_kernel update;
	cl_kernel render;
	cl_kernel clear;

	cl_mem velocities;
	cl_mem positions;
	cl_mem framebuffer;
#endif

	uint32_t msecs;

	float taps[16];
	int ntaps;
};

static void nemomote_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	static const char *vertexshader =
		"attribute vec3 position;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position.xy, 0.0, 1.0);\n"
		"  gl_PointSize = position.z;\n"
		"}\n";

	static const char *fragmentshader =
		"precision mediump float;\n"
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(tex, gl_PointCoord);\n"
		"}\n";

	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);
	GLuint width = nemoshow_canvas_get_viewport_width(canvas);
	GLuint height = nemoshow_canvas_get_viewport_height(canvas);
	GLuint fbo, dbo;
	GLuint program;
	GLuint frag, vert;
	GLfloat vertices[128 * 3];
	int i;

	for (i = 0; i < 128; i++) {
		vertices[i * 3 + 0] = random_get_double(-1.0f, 1.0f);
		vertices[i * 3 + 1] = random_get_double(-1.0f, 1.0f);
		vertices[i * 3 + 2] = random_get_double(32.0f, 64.0f);
	}

	fbo_prepare_context(
			nemoshow_canvas_get_texture(canvas),
			width, height,
			&fbo, &dbo);

	NEMO_CHECK((frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fragmentshader)) == GL_NONE, "failed to compile shader\n");
	NEMO_CHECK((vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertexshader)) == GL_NONE, "failed to compile shader\n");

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);
	glUseProgram(program);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glViewport(0, 0, width, height);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glBindAttribLocation(program, 0, "position");

	glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(context->sprite));

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_POINTS, 0, 128);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteProgram(program);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &dbo);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_feedback(show);
}

#ifdef NEMOUX_WITH_OPENGL_CS
static void nemomote_dispatch_canvas_redraw_cs(struct nemoshow *show, struct showone *canvas)
{
	static const char *vertexshader =
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position, 0.0, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";

	static const char *fragmentshader =
		"precision mediump float;\n"
		"varying vec2 vtexcoord;\n"
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(tex, vtexcoord);\n"
		"  gl_FragColor.a = 1.0;\n"
		"}\n";

	static const char *computeshader =
		"#version 310 es\n"
		"uniform float roll;\n"
		"layout(rgba32f, binding = 0) writeonly uniform mediump image2D tex0;\n"
		"layout(local_size_x = 16, local_size_y = 16) in;\n"
		"void main() {\n"
		"  ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n"
		"  float lcoef = length(vec2(ivec2(gl_LocalInvocationID.xy) - 8) / 8.0);\n"
		"  float gcoef = sin(float(gl_WorkGroupID.x + gl_WorkGroupID.y) * 0.1 + roll) * 0.5;\n"
		"  imageStore(tex0, pos, vec4(1.0 - gcoef * lcoef, 1.0, 1.0, 1.0));\n"
		"}\n";

	static GLfloat vertices[] = {
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	GLuint width = nemoshow_canvas_get_viewport_width(canvas);
	GLuint height = nemoshow_canvas_get_viewport_height(canvas);
	GLuint fbo, dbo;
	GLuint program;
	GLuint frag, vert;
	GLuint comp;
	GLuint texture;

	fbo_prepare_context(
			nemoshow_canvas_get_texture(canvas),
			width, height,
			&fbo, &dbo);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glViewport(0, 0, width, height);

	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	NEMO_CHECK((comp = glshader_compile(GL_COMPUTE_SHADER, 1, &computeshader)) == GL_NONE, "failed to compile shader\n");

	program = glCreateProgram();
	glAttachShader(program, comp);
	glLinkProgram(program);
	glUseProgram(program);

	glUniform1i(glGetUniformLocation(program, "tex0"), 0);
	glUniform1f(glGetUniformLocation(program, "roll"), (float)time_current_msecs());

	glBindTexture(GL_TEXTURE_2D, texture);

	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(16, 16, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glDeleteProgram(program);

	NEMO_CHECK((frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &fragmentshader)) == GL_NONE, "failed to compile shader\n");
	NEMO_CHECK((vert = glshader_compile(GL_VERTEX_SHADER, 1, &vertexshader)) == GL_NONE, "failed to compile shader\n");

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);
	glUseProgram(program);

	glUniform1i(glGetUniformLocation(program, "tex"), 0);

	glBindTexture(GL_TEXTURE_2D, texture);

	glBindAttribLocation(program, 0, "position");
	glBindAttribLocation(program, 1, "texcoord");

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glDeleteProgram(program);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteTextures(1, &texture);
	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &dbo);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_feedback(show);
}
#endif

#ifdef NEMOUX_WITH_OPENCL
static void nemomote_dispatch_canvas_redraw_cl(struct nemoshow *show, struct showone *canvas)
{
	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);
	cl_int width = nemoshow_canvas_get_viewport_width(canvas);
	cl_int height = nemoshow_canvas_get_viewport_height(canvas);
	char buffer[width * height * 4];
	size_t framesize[2] = { width, height };
	size_t partcount = NEMOMOTE_PARTICLES;
	uint32_t msecs = time_current_msecs();
	cl_float dt = (float)(msecs - context->msecs) / 1000.0f;
	cl_float x0 = 0.0f;
	cl_float y0 = 0.0f;
	cl_float x1 = width;
	cl_float y1 = height;
	cl_float bounce = 0.15f;
	cl_int count = NEMOMOTE_PARTICLES;
	cl_int r;
	int i;

	clSetKernelArg(context->clear, 0, sizeof(cl_mem), (void *)&context->framebuffer);
	clSetKernelArg(context->clear, 1, sizeof(cl_int), (void *)&width);
	clSetKernelArg(context->clear, 2, sizeof(cl_int), (void *)&height);
	clEnqueueNDRangeKernel(context->queue, context->clear, 2, NULL, framesize, NULL, 0, NULL, NULL);

	clSetKernelArg(context->boundingbox, 0, sizeof(cl_mem), (void *)&context->positions);
	clSetKernelArg(context->boundingbox, 1, sizeof(cl_mem), (void *)&context->velocities);
	clSetKernelArg(context->boundingbox, 2, sizeof(cl_int), (void *)&count);
	clSetKernelArg(context->boundingbox, 3, sizeof(cl_float), (void *)&dt);
	clSetKernelArg(context->boundingbox, 4, sizeof(cl_float), (void *)&x0);
	clSetKernelArg(context->boundingbox, 5, sizeof(cl_float), (void *)&y0);
	clSetKernelArg(context->boundingbox, 6, sizeof(cl_float), (void *)&x1);
	clSetKernelArg(context->boundingbox, 7, sizeof(cl_float), (void *)&y1);
	clSetKernelArg(context->boundingbox, 8, sizeof(cl_float), (void *)&bounce);
	clEnqueueNDRangeKernel(context->queue, context->boundingbox, 1, NULL, &partcount, NULL, 0, NULL, NULL);

	clSetKernelArg(context->mutualgravity, 0, sizeof(cl_mem), (void *)&context->positions);
	clSetKernelArg(context->mutualgravity, 1, sizeof(cl_mem), (void *)&context->velocities);
	clSetKernelArg(context->mutualgravity, 2, sizeof(cl_int), (void *)&count);
	clSetKernelArg(context->mutualgravity, 3, sizeof(cl_float), (void *)&dt);
	clEnqueueNDRangeKernel(context->queue, context->mutualgravity, 1, NULL, &partcount, NULL, 0, NULL, NULL);

	for (i = 0; i < context->ntaps; i++) {
		cl_float cx = context->taps[i * 2 + 0];
		cl_float cy = context->taps[i * 2 + 1];
		cl_float intensity = 700000.0f;

		clSetKernelArg(context->gravitywell, 0, sizeof(cl_mem), (void *)&context->positions);
		clSetKernelArg(context->gravitywell, 1, sizeof(cl_mem), (void *)&context->velocities);
		clSetKernelArg(context->gravitywell, 2, sizeof(cl_int), (void *)&count);
		clSetKernelArg(context->gravitywell, 3, sizeof(cl_float), (void *)&dt);
		clSetKernelArg(context->gravitywell, 4, sizeof(cl_float), (void *)&cx);
		clSetKernelArg(context->gravitywell, 5, sizeof(cl_float), (void *)&cy);
		clSetKernelArg(context->gravitywell, 6, sizeof(cl_float), (void *)&intensity);
		clEnqueueNDRangeKernel(context->queue, context->gravitywell, 1, NULL, &partcount, NULL, 0, NULL, NULL);
	}

	clSetKernelArg(context->update, 0, sizeof(cl_mem), (void *)&context->positions);
	clSetKernelArg(context->update, 1, sizeof(cl_mem), (void *)&context->velocities);
	clSetKernelArg(context->update, 2, sizeof(cl_float), (void *)&dt);
	clEnqueueNDRangeKernel(context->queue, context->update, 1, NULL, &partcount, NULL, 0, NULL, NULL);

	clSetKernelArg(context->render, 0, sizeof(cl_mem), (void *)&context->framebuffer);
	clSetKernelArg(context->render, 1, sizeof(cl_int), (void *)&width);
	clSetKernelArg(context->render, 2, sizeof(cl_int), (void *)&height);
	clSetKernelArg(context->render, 3, sizeof(cl_mem), (void *)&context->positions);
	clEnqueueNDRangeKernel(context->queue, context->render, 1, NULL, &partcount, NULL, 0, NULL, NULL);

	clEnqueueReadBuffer(context->queue, context->framebuffer, CL_TRUE, 0, sizeof(buffer), buffer, 0, NULL, NULL);

	glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(canvas));
	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, width);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);
	glBindTexture(GL_TEXTURE_2D, 0);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_feedback(show);

	context->msecs = msecs;
}

static int nemomote_prepare_opencl(struct motecontext *context, const char *path, int width, int height)
{
	cl_platform_id platforms[2] = { 0 };
	cl_uint ndevices;
	cl_uint nplatforms;
	cl_int r;
	float positions[NEMOMOTE_PARTICLES * 2];
	float velocities[NEMOMOTE_PARTICLES * 2];
	char *sources;
	int nsources;
	int i;

	os_load_path(path, &sources, &nsources);

	clGetPlatformIDs(2, platforms, &nplatforms);
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &context->device, &ndevices);

	context->context = clCreateContext(NULL, 1, &context->device, NULL, NULL, &r);
	context->queue = clCreateCommandQueue(context->context, context->device, 0, &r);

	context->program = clCreateProgramWithSource(context->context, 1, (const char **)&sources, (const size_t *)&nsources, &r);
	NEMO_CHECK(clBuildProgram(context->program, 1, &context->device, NULL, NULL, NULL) != CL_SUCCESS, "failed to build opencl program\n");

	context->clear = clCreateKernel(context->program, "clear", &r);
	context->render = clCreateKernel(context->program, "render", &r);
	context->update = clCreateKernel(context->program, "update", &r);
	context->mutualgravity = clCreateKernel(context->program, "mutualgravity", &r);
	context->gravitywell = clCreateKernel(context->program, "gravitywell", &r);
	context->boundingbox = clCreateKernel(context->program, "boundingbox", &r);

	for (i = 0; i < NEMOMOTE_PARTICLES; i++) {
		positions[i * 2 + 0] = random_get_double(0, width);
		positions[i * 2 + 1] = random_get_double(0, height);

		velocities[i * 2 + 0] = 0.0f;
		velocities[i * 2 + 1] = 0.0f;
	}

	context->velocities = clCreateBuffer(context->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float[2]) * NEMOMOTE_PARTICLES, velocities, &r);
	context->positions = clCreateBuffer(context->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float[2]) * NEMOMOTE_PARTICLES, positions, &r);
	context->framebuffer = clCreateBuffer(context->context, CL_MEM_WRITE_ONLY, sizeof(char[4]) * width * height, NULL, &r);

	context->msecs = time_current_msecs();

	return 0;
}

static int nemomote_resize_opencl(struct motecontext *context, int width, int height)
{
	cl_int r;

	clReleaseMemObject(context->framebuffer);

	context->framebuffer = clCreateBuffer(context->context, CL_MEM_WRITE_ONLY, sizeof(char[4]) * width * height, NULL, &r);

	return 0;
}

static void nemomote_finish_opencl(struct motecontext *context)
{
	clReleaseMemObject(context->velocities);
	clReleaseMemObject(context->positions);
	clReleaseMemObject(context->framebuffer);

	clReleaseKernel(context->clear);
	clReleaseKernel(context->render);
	clReleaseKernel(context->update);
	clReleaseKernel(context->mutualgravity);
	clReleaseKernel(context->gravitywell);
	clReleaseKernel(context->boundingbox);
	clReleaseProgram(context->program);
	clReleaseCommandQueue(context->queue);
	clReleaseContext(context->context);
}
#endif

static void nemomote_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);
	int i;

	nemoshow_event_update_taps(show, canvas, event);

	context->ntaps = nemoshow_event_get_tapcount(event);

	for (i = 0; i < context->ntaps; i++) {
		nemoshow_transform_to_viewport(show,
				nemoshow_event_get_x_on(event, i),
				nemoshow_event_get_y_on(event, i),
				&context->taps[i * 2 + 0],
				&context->taps[i * 2 + 1]);
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		if (nemoshow_event_is_more_taps(show, event, 3)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}
}

static void nemomote_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

#ifdef NEMOUX_WITH_OPENCL
	nemomote_resize_opencl(context, width, height);
#endif

	nemoshow_view_redraw(context->show);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "program",				required_argument,			NULL,			'p' },
		{ 0 }
	};

	struct motecontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showone *blur;
	char *programpath = NULL;
	int width = 800;
	int height = 800;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "p:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'p':
				programpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

#ifdef NEMOUX_WITH_OPENCL
	if (programpath == NULL)
		return 0;
#endif

	context = (struct motecontext *)malloc(sizeof(struct motecontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct motecontext));

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemomote_dispatch_show_resize);
	nemoshow_set_userdata(show, context);

	nemoshow_view_put_state(show, "smooth");

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->view = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_smooth(canvas, 0);
#ifdef NEMOUX_WITH_OPENCL
	nemoshow_canvas_set_dispatch_redraw(canvas, nemomote_dispatch_canvas_redraw_cl);
#elif NEMOUX_WITH_OPENGL_CS
	nemoshow_canvas_set_dispatch_redraw(canvas, nemomote_dispatch_canvas_redraw_cs);
#else
	nemoshow_canvas_set_dispatch_redraw(canvas, nemomote_dispatch_canvas_redraw);
#endif
	nemoshow_canvas_set_dispatch_event(canvas, nemomote_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	context->sprite = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, 64);
	nemoshow_canvas_set_height(canvas, 64);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_attach_one(show, canvas);

	blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(blur, "solid", 8.0f);

	one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_cx(one, 64.0f / 2.0f);
	nemoshow_item_set_cy(one, 64.0f / 2.0f);
	nemoshow_item_set_r(one, 64.0f / 3.0f);
	nemoshow_item_set_fill_color(one, 255.0f, 255.0f, 255.0f, 255.0f);
	nemoshow_item_set_filter(one, blur);

#ifdef NEMOUX_WITH_OPENCL
	nemomote_prepare_opencl(context, programpath, width, height);
#endif

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

#ifdef NEMOUX_WITH_OPENCL
	nemomote_finish_opencl(context);
#endif

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
