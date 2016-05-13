/* Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/
/* This is a simple example of using GLSL shaders with SDL */
#define GL_GLEXT_PROTOTYPES

#include <stdlib.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

struct shader_data {
	GLhandleARB program;
	GLhandleARB vert_shader;
	GLhandleARB frag_shader;
    GLfloat width;
    GLfloat height;
	GLuint fb;
	GLuint tex;
};

#define GLSL(x) "#version 130\n" #x
#define GLSL_DECL(x) "uniform sampler2D tex; uniform vec2 resolution;" #x

static const char * vertex_shader_source = GLSL(
    void main() {
        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    }
);

static const char * final_shader_source = GLSL_DECL(
    void main() {
        vec2 uv = gl_FragCoord.xy / resolution;
        gl_FragColor = texture2D(tex, uv);
    }
);
static struct shader_data final_shader;

static const char * shader_sources[] = {
	GLSL_DECL(
		void main() {
			vec2 uv = gl_FragCoord.xy / resolution;
            vec4 color = texture2D(tex, uv);
			gl_FragColor = vec4(uv.xy, color.b, 1.0);
		}
	),
	GLSL_DECL(
		void main() {
			vec2 uv = gl_FragCoord.xy / resolution;
            vec4 color = texture2D(tex, uv);
			gl_FragColor = vec4(color.b, uv.xy, 1.0);
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
    glUniform2fARB(location, output->width, output->height);

    /*
    location = glGetUniformLocationARB(output->program, "tex");
    if (location < 0) goto fail;
    glUniform1iARB(location, child_tex);
    */

    /*
    for (size_t x = 0; x < 10; x++) {
        GLsizei sz, usz;
        GLenum utype;
        char buf[1024];
        glGetActiveUniform(output->program, x, sizeof buf, &sz, &usz, &utype, buf);
        printf("uniform: %s\n", buf);
    }
    */

    /* TODO
    for (i = 0; i < num_tmus_bound; ++i) {
        char tex_name[5];
        SDL_snprintf(tex_name, SDL_arraysize(tex_name), "tex%d", i);
        location = glGetUniformLocation(data->program, tex_name);
        if (location >= 0) {
            glUniform1iARB(location, i);
        }
    }
    */
    glUseProgramObjectARB(0);

    // Make the framebuffer
    glGenFramebuffersEXT(1, &output->fb);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, output->fb);
    glGenTextures(1, &output->tex);
    glBindTexture(GL_TEXTURE_2D, output->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, output->width, output->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, output->tex, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "Got error: %d (%#04x)\n", err, err);
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
void shader_draw(struct shader_data * shaders, size_t i) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        // Clear The Screen And The Depth Buffer
    glLoadIdentity();                // Reset The View

	// Enable blending
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glEnable(GL_TEXTURE_2D);

    for (size_t z = 0; z < shader_count; z++) {
        glUseProgramObjectARB(shaders[z].program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shaders[(z+1)%shader_count].tex);
        GLint loc = glGetUniformLocationARB(shaders[z].program, "tex");
        glUniform1iARB(loc, 0);

        glEnable(GL_TEXTURE_2D);

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, shaders[i].fb);
        glBegin(GL_QUADS);                // start drawing a polygon (4 sided)
        glVertex2f(-1, -1);
        glVertex2f(1, -1);
        glVertex2f(1, 1);
        glVertex2f(-1, 1);
        glEnd();                    // done with the polygon
    }

	glUseProgramObjectARB(final_shader.program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shaders[i].tex);
    GLint loc = glGetUniformLocationARB(final_shader.program, "tex");
    if (loc < 0) { fprintf(stderr, "fail\n"); }
    glUniform1iARB(loc, 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glBegin(GL_QUADS);                // start drawing a polygon (4 sided)
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(1, 1);
    glVertex2f(-1, 1);
    glEnd();                    // done with the polygon

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

    final_shader.width = 300;
    final_shader.height = 300;
    int rc = shader_data_init(&final_shader, final_shader_source);
    struct shader_data * shaders = shaders_create(300, 300);
    if (rc != 0 || shaders == NULL) {
        printf("Unable to initialize!\n");
		exit(1);
    }

    size_t current_shader = 0;
    while (true) {
        shader_draw(shaders, current_shader);

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
