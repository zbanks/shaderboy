/* Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
/* This is a simple example of using GLSL shaders with SDL */

#include <stdlib.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

enum {
    SHADER_COLOR,
    SHADER_TEXTURE,
    SHADER_TEXCOORDS,
    NUM_SHADERS
};

#define GLSL(x) #x
#define GLSL_DECL(x) "uniform vec2 resolution; " #x

static const char * vertex_shader_source = GLSL(
    void main() {
        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    }
);

static const char * shader_sources[] = {
	GLSL_DECL(
		void main() {
			vec2 uv = gl_FragCoord.xy / resolution;
			gl_FragColor = vec4(uv.xy, 0.5, 1.0);
		}
	),
	GLSL_DECL(
		void main() {
			vec2 uv = gl_FragCoord.xy / resolution;
			gl_FragColor = vec4(uv.xy, 0.5, 1.0).bgra;
		}
	),
	GLSL_DECL(
		void main() {
			vec2 uv = gl_FragCoord.xy / resolution;

			vec2 delta = vec2(0.5, 0.5) - uv;
			float dist = dot(delta, delta);

			gl_FragColor.rg = uv;
			gl_FragColor.b = uv.x * uv.y;
			gl_FragColor.a = clamp(1.0 - (dist * 4.0), 0., 1.);
		}
	),
};
static size_t shader_count = sizeof(shader_sources) / sizeof(*shader_sources);

struct shader_data {
	GLhandleARB program;
	GLhandleARB vert_shader;
	GLhandleARB frag_shader;
    GLfloat width;
    GLfloat height;
	GLuint out_tex;
};

static PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
static PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
static PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
static PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
static PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
static PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
static PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
static PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
static PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
static PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
static PFNGLUNIFORM1IARBPROC glUniform1iARB;
static PFNGLUNIFORM2FARBPROC glUniform2fARB;
static PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;

static int compile_shader(GLhandleARB shader, const char *source)
{
    GLint status;

    glShaderSourceARB(shader, 1, &source, NULL);
    glCompileShaderARB(shader);
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
    if (status == 0) {
        GLint length;
        char *info;

        glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
        info = SDL_stack_alloc(char, length+1);
        glGetInfoLogARB(shader, length, NULL, info);
        fprintf(stderr, "Failed to compile shader:\n%s\n%s", source, info);
        SDL_stack_free(info);

        return -1;
    }
	
    return 0;
}

static int shader_data_init(struct shader_data * output, const char * source) {
    //const int num_tmus_bound = shader_count + 1;
    //GLint location;

    glGetError();

    /* Create one program object to rule them all */
    output->program = glCreateProgramObjectARB();

    /* Create the vertex shader */
    output->vert_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
    if (compile_shader(output->vert_shader, vertex_shader_source))
        return -1;

    /* Create the fragment shader */
    output->frag_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
    if (compile_shader(output->frag_shader, source))
        return -1;

    /* ... and in the darkness bind them */
    glAttachObjectARB(output->program, output->vert_shader);
    glAttachObjectARB(output->program, output->frag_shader);
    glLinkProgramARB(output->program);

    /* Set up some uniform variables */
    glUseProgramObjectARB(output->program);
    GLint location = glGetUniformLocationARB(output->program, "resolution");
    if (location >= 0) {
        glUniform2fARB(location, output->width, output->height);
    }

    /* TODO
    for (i = 0; i < num_tmus_bound; ++i) {
        char tex_name[5];
        SDL_snprintf(tex_name, SDL_arraysize(tex_name), "tex%d", i);
        location = glGetUniformLocationARB(data->program, tex_name);
        if (location >= 0) {
            glUniform1iARB(location, i);
        }
    }
    */
    glUseProgramObjectARB(0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "Got error: %d", err);
        return -1;
    }

    return 0;
}

static void shader_data_term(struct shader_data * data) {
	glDeleteObjectARB(data->vert_shader);
	glDeleteObjectARB(data->frag_shader);
	glDeleteObjectARB(data->program);
}

static struct shader_data * shaders_create(float width, float height) {
/*
    if (SDL_GL_ExtensionSupported("GL_ARB_shader_objects") &&
        SDL_GL_ExtensionSupported("GL_ARB_shading_language_100") &&
        SDL_GL_ExtensionSupported("GL_ARB_vertex_shader") &&
        SDL_GL_ExtensionSupported("GL_ARB_fragment_shader")) {
*/
	glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) SDL_GL_GetProcAddress("glAttachObjectARB");
	glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) SDL_GL_GetProcAddress("glCompileShaderARB");
	glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glCreateProgramObjectARB");
	glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) SDL_GL_GetProcAddress("glCreateShaderObjectARB");
	glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC) SDL_GL_GetProcAddress("glDeleteObjectARB");
	glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC) SDL_GL_GetProcAddress("glGetInfoLogARB");
	glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) SDL_GL_GetProcAddress("glGetObjectParameterivARB");
	glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) SDL_GL_GetProcAddress("glGetUniformLocationARB");
	glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) SDL_GL_GetProcAddress("glLinkProgramARB");
	glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) SDL_GL_GetProcAddress("glShaderSourceARB");
	glUniform1iARB = (PFNGLUNIFORM1IARBPROC) SDL_GL_GetProcAddress("glUniform1iARB");
	glUniform2fARB = (PFNGLUNIFORM2FARBPROC) SDL_GL_GetProcAddress("glUniform2fARB");
	glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glUseProgramObjectARB");
	if (glAttachObjectARB &&
		glCompileShaderARB &&
		glCreateProgramObjectARB &&
		glCreateShaderObjectARB &&
		glDeleteObjectARB &&
		glGetInfoLogARB &&
		glGetObjectParameterivARB &&
		glGetUniformLocationARB &&
		glLinkProgramARB &&
		glShaderSourceARB &&
		glUniform1iARB &&
		glUseProgramObjectARB) {
	}

    struct shader_data * shaders = calloc(shader_count, sizeof *shaders);
    /* Compile all the shaders */
    for (size_t i = 0; i < shader_count; ++i) {
        shaders[i].width = width;
        shaders[i].height = height;
        if (shader_data_init(&shaders[i], shader_sources[i])) {
            fprintf(stderr, "Unable to compile shader!\n");
            return NULL;
        }
    }

    /* We're done! */
    return shaders;
}

static void shaders_destroy(struct shader_data * shaders) {
    for (size_t i = 0; i < shader_count; ++i) {
        shader_data_term(&shaders[i]);
    }
    free(shaders);
}

/* A general OpenGL initialization function.    Sets all of the initial parameters. */
void InitGL(int Width, int Height)                    // We call this right after our OpenGL window is created.
{
    GLdouble aspect;

    glViewport(0, 0, Width, Height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);        // This Will Clear The Background Color To Black
    glClearDepth(1.0);                // Enables Clearing Of The Depth Buffer
    glDepthFunc(GL_LESS);                // The Type Of Depth Test To Do
    glEnable(GL_DEPTH_TEST);            // Enables Depth Testing
    glShadeModel(GL_SMOOTH);            // Enables Smooth Color Shading

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();                // Reset The Projection Matrix

    aspect = (GLdouble)Width / Height;
    //glOrtho(-3.0, 3.0, -3.0 / aspect, 3.0 / aspect, 0.0, 1.0);
    glOrtho(-1.0, 1.0, -1.0 / aspect, 1.0 / aspect, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
}

/* The main drawing function. */
void shader_draw(struct shader_data * shader) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        // Clear The Screen And The Depth Buffer
    glLoadIdentity();                // Reset The View

	// Enable blending
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // draw a textured square (quadrilateral)
    glEnable(GL_TEXTURE_2D);
    //glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1.0f,1.0f,1.0f);
	glUseProgramObjectARB(shader->program);

    glBegin(GL_QUADS);                // start drawing a polygon (4 sided)
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, 1.0f, 0.0f);        // Top Left
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f( 1.0f, 1.0f, 0.0f);        // Top Right
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f( 1.0f,-1.0f, 0.0f);        // Bottom Right
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f,-1.0f, 0.0f);        // Bottom Left    
    glEnd();                    // done with the polygon

	glUseProgramObjectARB(0);
    glDisable(GL_TEXTURE_2D);

    // swap buffers to display, since we're double buffered.
    SDL_GL_SwapBuffers();
}

int main(int argc, char **argv) {    
    /* Initialize SDL for video output */
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    /* Create a 300x300 OpenGL screen */
    if ( SDL_SetVideoMode(300, 300, 0, SDL_OPENGL) == NULL ) {
        fprintf(stderr, "Unable to create OpenGL screen: %s\n", SDL_GetError());
        SDL_Quit();
        exit(2);
    }

    /* Set the title bar in environments that support it */
    SDL_WM_SetCaption("Shaderboy", NULL);

    /* Loop, drawing and checking events */
    InitGL(300, 300);

    struct shader_data * shaders = shaders_create(300, 300);
    if (shaders == NULL) {
        printf("Shaders not supported!\n");
		exit(1);
    }

    size_t current_shader = 0;
    while (true) {
        shader_draw(&shaders[current_shader]);

		SDL_Event event;
		while ( SDL_PollEvent(&event) ) {
			switch(event.type) {
			case SDL_QUIT:
				goto quit;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_SPACE:
					current_shader = (current_shader + 1) % shader_count;
					break;
				case SDLK_ESCAPE:
					goto quit;
				default:
					break;
				}
			default:
				break;
			}
		}
    }

quit:
    shaders_destroy(shaders);
    SDL_Quit();
    return 0;
}
