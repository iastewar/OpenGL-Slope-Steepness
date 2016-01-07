//#include "opengl.h"

// Bring in OpenGL 
// Windows
#ifdef WIN32
#include <windows.h>		// Must have for Windows platform builds
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include <glew.h>			// OpenGL Extension "autoloader"
#include <gl\gl.h>			// Microsoft OpenGL headers (version 1.1 by themselves)
#endif

// Linux
#ifdef linux
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <GL/glut.h>
#include <string.h>

class Shader {
public:
	void shaderAttachFromFile(GLuint program, GLenum type, const char *filePath);
};
