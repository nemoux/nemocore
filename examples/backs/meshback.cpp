#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <tiny_obj_loader.h>

#include <meshback.h>

#include <nemotool.h>
#include <nemoegl.h>
#include <talehelper.h>
#include <pixmanhelper.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <nemomisc.h>

static GLuint meshback_create_shader(void)
{
	static const char *vert_shader_text =
		"uniform mat4 projection;\n"
		"attribute vec3 vertex;\n"
		"void main() {\n"
		"  gl_Position = projection * vec4(vertex, 1.0f);\n"
		"}\n";
	static const char *frag_shader_text =
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"void main() {\n"
		"  gl_FragColor = color;\n"
		"}\n";
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = glshader_compile(GL_FRAGMENT_SHADER, 1, &frag_shader_text);
	vert = glshader_compile(GL_VERTEX_SHADER, 1, &vert_shader_text);

	program = glCreateProgram();
	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%*s\n", len, log);
		exit(1);
	}

	glUseProgram(program);

	glBindAttribLocation(program, 0, "vertex");
	glLinkProgram(program);

	return program;
}

static void meshback_prepare(struct meshback *mesh, const char *filepath, const char *basepath)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string r;
	GLfloat vertices[12] = {
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f
	};
	GLuint indices[6] = {
		0, 1, 2,
		0, 2, 3
	};
	int i, j;

	r = tinyobj::LoadObj(shapes, materials, filepath, basepath);
	if (!r.empty())
		exit(-1);

	for (i = 0; i < shapes.size(); i++) {
		for (j = 0; j < shapes[i].mesh.indices.size() / 3; j++) {
			NEMO_DEBUG("%d: %d %d %d\n", j,
					shapes[i].mesh.indices[j * 3 + 0],
					shapes[i].mesh.indices[j * 3 + 1],
					shapes[i].mesh.indices[j * 3 + 2]);
		}

		for (j = 0; j < shapes[i].mesh.positions.size() / 3; j++) {
			NEMO_DEBUG("%d: %f %f %f\n", j,
					shapes[i].mesh.positions[j * 3 + 0],
					shapes[i].mesh.positions[j * 3 + 1],
					shapes[i].mesh.positions[j * 3 + 2]);
		}
	}

	mesh->program = meshback_create_shader();

	mesh->uprojection = glGetUniformLocation(mesh->program, "projection");
	mesh->ucolor = glGetUniformLocation(mesh->program, "color");

	fbo_prepare_context(
			nemotale_node_get_texture(mesh->node),
			mesh->width, mesh->height,
			&mesh->fbo, &mesh->dbo);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	nemomatrix_init_identity(&mesh->matrix);
	nemomatrix_scale(&mesh->matrix, 1.0f, -1.0f);

	glGenVertexArrays(1, &mesh->varray);
	glBindVertexArray(mesh->varray);

	glGenBuffers(1, &mesh->vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void *)0);
	glEnableVertexAttribArray(0);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &mesh->vindex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vindex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 6, indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

static void meshback_finish(struct meshback *mesh)
{
	glDeleteBuffers(1, &mesh->vbuffer);
	glDeleteBuffers(1, &mesh->vindex);
	glDeleteVertexArrays(1, &mesh->varray);

	glDeleteFramebuffers(1, &mesh->fbo);
	glDeleteRenderbuffers(1, &mesh->dbo);
}

static void meshback_render(struct meshback *mesh)
{
	GLfloat rgba[4] = { 0.0f, 1.0f, 1.0f, 1.0f };

	glUseProgram(mesh->program);

	glBindFramebuffer(GL_FRAMEBUFFER, mesh->fbo);

	glViewport(0, 0, mesh->width, mesh->height);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(mesh->uprojection, 1, GL_FALSE, (GLfloat *)mesh->matrix.d);
	glUniform4fv(mesh->ucolor, 1, rgba);

	glBindVertexArray(mesh->varray);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void meshback_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

static void meshback_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",				required_argument,			NULL,		'f' },
		{ "path",				required_argument,			NULL,		'p' },
		{ "type",				required_argument,			NULL,		't' },
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ "background",	no_argument,						NULL,		'b' },
		{ 0 }
	};
	struct meshback *mesh;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	char *filepath = NULL;
	char *basepath = NULL;
	char *type = NULL;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;

	while (opt = getopt_long(argc, argv, "f:p:t:w:h:b", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 'p':
				basepath = strdup(optarg);
				break;

			case 't':
				type = strdup(optarg);
				break;

			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	mesh = (struct meshback *)malloc(sizeof(struct meshback));
	if (mesh == NULL)
		return -1;
	memset(mesh, 0, sizeof(struct meshback));

	mesh->width = width;
	mesh->height = height;

	mesh->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	mesh->egl = egl = nemotool_create_egl(tool);

	mesh->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_opaque(NTEGL_CANVAS(canvas), 0, 0, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_layer(NTEGL_CANVAS(canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), meshback_dispatch_canvas_resize);

	mesh->canvas = NTEGL_CANVAS(canvas);

	mesh->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), meshback_dispatch_tale_event);
	nemotale_set_userdata(tale, mesh);

	mesh->node = node = nemotale_node_create_gl(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_attach_node(tale, node);

	meshback_prepare(mesh, filepath, basepath);
	meshback_render(mesh);

	nemotale_node_damage_all(node);

	nemotale_composite_egl(tale, NULL);

	nemotool_run(tool);

	meshback_finish(mesh);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(filepath);
	free(basepath);
	free(mesh);

	return 0;
}
