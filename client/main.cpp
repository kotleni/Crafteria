#include <SDL2/SDL.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/ext.hpp>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#define WINDOW_NAME "Crafteria"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static GLuint tri_vbo; /* the VBO id for vertex data */
static GLuint tri_color_vbo; /* the VBO id for color data */
static unsigned tri_vbo_elem; /* number of elements in this one */

#define check_gl_error() do { \
	GLenum e##__LINE__ = glGetError(); \
	if (e##__LINE__ != GL_NO_ERROR) \
		fprintf(stderr,"ERROR:%d:glerr=%d\n", __LINE__, e##__LINE__); \
	} while(0)


static void initialize_scene(void)
{
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	/* black screen */
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapWindow(win);

	/* setup projection matrix */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/* hacky way to correct for aspect ratio */
	int width, height;
	SDL_GetWindowSize(win, &width, &height);
	double aspect_root = sqrt((double)width / (double)height);
	double nearest = 0.125; /* how close you can get before it's clipped. */
	glFrustum(-nearest * aspect_root, nearest * aspect_root,
		-nearest / aspect_root, nearest / aspect_root,
		nearest, 1000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	check_gl_error();
}
static void paint_scene(void)
{
	SDL_Window *win = SDL_GL_GetCurrentWindow();
	static float angle_x = 0.0, angle_y = 0.0, angle_z = 0.0;

	glClearColor(0.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.0, 0.0, -3.0);
	glRotatef(angle_x, 1.0, 0.0, 0.0);
	glRotatef(angle_y, 0.0, 1.0, 0.0);
	glRotatef(angle_z, 0.0, 0.0, 1.0);
	angle_x += 2.0;
	angle_y += 3.0;
	angle_z += 4.0;

	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, tri_vbo);
	glVertexPointer(3, GL_FLOAT, 0, (void*)(0));

	glEnableClientState(GL_COLOR_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, tri_color_vbo);
	glColorPointer(3, GL_FLOAT, 0, (void*)(0));

	glBindBuffer(GL_ARRAY_BUFFER, 0); /* release binding */

	glDrawArrays(GL_TRIANGLES, 0, tri_vbo_elem);
	check_gl_error();
	glDisableClientState(GL_VERTEX_ARRAY);

	glFlush();
	SDL_GL_SwapWindow(win);
	check_gl_error();
}

int main()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return EXIT_FAILURE;

#if 0
	/* configure the OpenGL version to use. */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_CORE);
#endif

	/* create a window suitable for OpenGL. */
	SDL_Window *mainwin = SDL_CreateWindow(WINDOW_NAME,
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!mainwin) {
		fprintf(stderr, "ERROR:%s\n", SDL_GetError());
		SDL_DestroyWindow(mainwin);
	    SDL_Quit();
        return EXIT_FAILURE;
	}

	/* create a context. */
	SDL_GLContext glctx = SDL_GL_CreateContext(mainwin);
	if (!glctx) {
		fprintf(stderr, "ERROR:%s\n", SDL_GetError());
		SDL_DestroyWindow(mainwin);
	    SDL_Quit();
        return EXIT_FAILURE;
	}
	SDL_GL_SetSwapInterval(1);

	/* initialize the scene and create the VBO */
	initialize_scene();

	float tri_data[] = {
		/* face 1 */
		1.0, -1.0, -1.0,
		-1.0, 1.0, -1.0,
		1.0, 1.0, -1.0,

		-1.0, 1.0, -1.0,
		1.0, -1.0, -1.0,
		-1.0, -1.0, -1.0,

		/* face 2 */
		-1.0, 1.0, 1.0,
		1.0, -1.0, 1.0,
		1.0, 1.0, 1.0,

		1.0, -1.0, 1.0,
		-1.0, 1.0, 1.0,
		-1.0, -1.0, 1.0,

		/* face 3 */
		-1.0, 1.0, -1.0,
		-1.0, 1.0, 1.0,
		1.0, 1.0, -1.0,

		-1.0, 1.0, 1.0,
		1.0, 1.0, 1.0,
		1.0, 1.0, -1.0,

		/* face 4 */
		-1.0, -1.0, 1.0,
		-1.0, -1.0, -1.0,
		1.0, -1.0, -1.0,

		1.0, -1.0, 1.0,
		-1.0, -1.0, 1.0,
		1.0, -1.0, -1.0,

		/* face 5 */
		1.0, 1.0, -1.0,
		1.0, 1.0, 1.0,
		1.0, -1.0, 1.0,

		1.0, 1.0, -1.0,
		1.0, -1.0, 1.0,
		1.0, -1.0, -1.0,

		/* face 6 */
		-1.0, 1.0, 1.0,
		-1.0, 1.0, -1.0,
		-1.0, -1.0, 1.0,

		-1.0, -1.0, 1.0,
		-1.0, 1.0, -1.0,
		-1.0, -1.0, -1.0,
	};
	float tri_color[] = {
		/* face 1 */
		0.0, 0.7, 1.0,
		0.0, 0.7, 1.0,
		0.0, 0.7, 1.0,
		0.0, 0.7, 1.0,
		0.0, 0.7, 1.0,
		0.0, 0.7, 1.0,

		/* face 2 */
		0.9, 1.0, 0.0,
		0.9, 1.0, 0.0,
		0.9, 1.0, 0.0,
		0.9, 1.0, 0.0,
		0.9, 1.0, 0.0,
		0.9, 1.0, 0.0,

		/* face 3 */
		0.2, 0.2, 1.0,
		0.2, 0.2, 1.0,
		0.2, 0.2, 1.0,
		0.2, 0.2, 1.0,
		0.2, 0.2, 1.0,
		0.2, 0.2, 1.0,

		/* face 4 */
		0.9, 0.1, 0.1,
		0.9, 0.1, 0.1,
		0.9, 0.1, 0.1,
		0.9, 0.1, 0.1,
		0.9, 0.1, 0.1,
		0.9, 0.1, 0.1,

		/* face 5 */
		0.1, 0.7, 0.1,
		0.1, 0.7, 0.1,
		0.1, 0.7, 0.1,
		0.1, 0.7, 0.1,
		0.1, 0.7, 0.1,
		0.1, 0.7, 0.1,

		/* face 6 */
		0.7, 0.0, 0.7,
		0.7, 0.0, 0.7,
		0.7, 0.0, 0.7,
		0.7, 0.0, 0.7,
		0.7, 0.0, 0.7,
		0.7, 0.0, 0.7,
	};

	/* vertex buffer ... */
	glGenBuffers(1, &tri_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, tri_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri_data), tri_data,
		GL_STATIC_DRAW);
	tri_vbo_elem = sizeof(tri_data) / sizeof(*tri_data) / 3;
	glBindBuffer(GL_ARRAY_BUFFER, 0); /* release binding */

	/* color buffer */
	glGenBuffers(1, &tri_color_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, tri_color_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(tri_color), tri_color,
		GL_STATIC_DRAW);
	if (tri_vbo_elem != (sizeof(tri_color) / sizeof(*tri_color) / 3)) {
		fprintf(stderr, "ERROR:tri_color[] does not have the same number of elements as tri_data[]\n");
		// TODO: bail
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0); /* release binding */

	/* Event Loop */
	bool quit = false;
	while (1) {
		SDL_Event ev;

		/* process events until timeout occurs */
		while (SDL_WaitEventTimeout(&ev, 15)) {
			switch (ev.type) {
			case SDL_QUIT:
				quit = true;
				SDL_DestroyWindow(mainwin);
	            SDL_Quit();
                return EXIT_FAILURE;
			}
		}

		paint_scene();
	}
    
	SDL_DestroyWindow(mainwin);
	SDL_Quit();

	return 0;
}