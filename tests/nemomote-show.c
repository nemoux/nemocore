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

#define NEMOMOTE_PARTICLES			(1024)

struct motecontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *view;

#ifdef NEMOUX_WITH_OPENCL
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel dispatch;
	cl_kernel clear;

	cl_mem velocities;
	cl_mem positions;
	cl_mem framebuffer;
#endif

	uint32_t msecs;
};

static void nemomote_dispatch_canvas_redraw_cs(struct nemoshow *show, struct showone *canvas)
{
	static const char *vertexshader =
		"attribute vec2 positions;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(positions, 0.0, 1.0);\n"
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

	glBindAttribLocation(program, 0, "positions");
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

#ifdef NEMOUX_WITH_OPENCL
static void nemomote_dispatch_canvas_redraw_cl(struct nemoshow *show, struct showone *canvas)
{
	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);
	GLuint width = nemoshow_canvas_get_viewport_width(canvas);
	GLuint height = nemoshow_canvas_get_viewport_height(canvas);
	char buffer[width * height * 4];
	size_t clearsize[2] = { width, height };
	size_t dispatchsize = NEMOMOTE_PARTICLES;
	uint32_t msecs = time_current_msecs();
	cl_float dt = (float)(msecs - context->msecs) / 1000.0f;
	cl_int count = NEMOMOTE_PARTICLES;
	cl_int r;

	clSetKernelArg(context->clear, 0, sizeof(cl_mem), (void *)&context->framebuffer);
	clSetKernelArg(context->clear, 1, sizeof(cl_int), &width);
	clSetKernelArg(context->clear, 2, sizeof(cl_int), &height);

	clSetKernelArg(context->dispatch, 0, sizeof(cl_mem), (void *)&context->velocities);
	clSetKernelArg(context->dispatch, 1, sizeof(cl_mem), (void *)&context->positions);
	clSetKernelArg(context->dispatch, 2, sizeof(cl_mem), (void *)&context->framebuffer);
	clSetKernelArg(context->dispatch, 3, sizeof(cl_int), &count);
	clSetKernelArg(context->dispatch, 4, sizeof(cl_float), &dt);
	clSetKernelArg(context->dispatch, 5, sizeof(cl_int), &width);
	clSetKernelArg(context->dispatch, 6, sizeof(cl_int), &height);

	clEnqueueNDRangeKernel(context->queue, context->clear, 2, NULL, clearsize, NULL, 0, NULL, NULL);
	clEnqueueNDRangeKernel(context->queue, context->dispatch, 1, NULL, &dispatchsize, NULL, 0, NULL, NULL);
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
	char *sources;
	int nsources;
	int i;

	os_load_path(path, &sources, &nsources);

	r = clGetPlatformIDs(2, platforms, &nplatforms);
	r = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &context->device, &ndevices);

	context->context = clCreateContext(NULL, 1, &context->device, NULL, NULL, &r);
	context->queue = clCreateCommandQueue(context->context, context->device, 0, &r);

	context->program = clCreateProgramWithSource(context->context, 1, (const char **)&sources, (const size_t *)&nsources, &r);
	r = clBuildProgram(context->program, 1, &context->device, NULL, NULL, NULL);

	context->clear = clCreateKernel(context->program, "clear", &r);
	context->dispatch = clCreateKernel(context->program, "dispatch", &r);

	for (i = 0; i < NEMOMOTE_PARTICLES; i++) {
		positions[i * 2 + 0] = random_get_double(0, width);
		positions[i * 2 + 1] = random_get_double(0, height);
	}

	context->velocities = clCreateBuffer(context->context, CL_MEM_READ_WRITE, sizeof(float[2]) * NEMOMOTE_PARTICLES, NULL, &r);
	context->positions = clCreateBuffer(context->context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(float[2]) * NEMOMOTE_PARTICLES, positions, &r);
	context->framebuffer = clCreateBuffer(context->context, CL_MEM_WRITE_ONLY, sizeof(char[4]) * width * height, NULL, &r);

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
	clFlush(context->queue);
	clFinish(context->queue);

	clReleaseMemObject(context->velocities);
	clReleaseMemObject(context->positions);
	clReleaseMemObject(context->framebuffer);

	clReleaseKernel(context->clear);
	clReleaseKernel(context->dispatch);
	clReleaseProgram(context->program);
	clReleaseCommandQueue(context->queue);
	clReleaseContext(context->context);
}
#endif

static void nemomote_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct motecontext *context = (struct motecontext *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

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
	char *programpath = NULL;
	int width = 640;
	int height = 480;
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
#ifdef NEMOUX_WITH_OPENCL
	nemoshow_canvas_set_dispatch_redraw(canvas, nemomote_dispatch_canvas_redraw_cl);
#else
	nemoshow_canvas_set_dispatch_redraw(canvas, nemomote_dispatch_canvas_redraw_cs);
#endif
	nemoshow_canvas_set_dispatch_event(canvas, nemomote_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

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
