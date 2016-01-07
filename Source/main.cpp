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

#include <GL/glui.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <GL/glut.h>
#include "gltb.h"
#include "Model.h"
#include "dirent32.h"

#include "PixelMap.h"

PixelMap pix[2];

#pragma comment( linker, "/entry:\"mainCRTStartup\"" )  // set the entry point to be main()

#define DATA_DIR "data/"
#define DATAA2_DIR "data/A2/"

#define AERIALPER_DIR "textures/aerial-persp-fig9/"
#define DEPTHFIELD_DIR "textures/depth-field-fig8/"
#define MATERIAL_DIR "textures/material-fig11/"
#define SILHBCKLIG_DIR "textures/silh-bcklig-fig10/"
#define TOON_DIR "textures/toon-fig7/"

char*      model_file = NULL;		/* name of the obect file */
GLuint     model_list = 0;		    /* display list for object */
Model*     model;			        /* model data structure */
Shader	   SM;
GLfloat    scale;			        /* original scale factor */
GLfloat    smoothing_angle = 90.0;	/* smoothing angle */
GLfloat    weld_distance = 0.00001;	/* epsilon for welding vertices */
GLboolean  facet_normal = GL_FALSE;	/* draw with facet normal? */
GLboolean  bounding_box = GL_FALSE;	/* bounding box on? */
GLboolean  performance = GL_FALSE;	/* performance counter on? */
GLboolean  stats = GL_FALSE;		/* statistics on? */
GLuint     material_mode = 0;		/* 0=none, 1=color, 2=material */
GLint      entries = 0;			    /* entries in model menu */
GLdouble   pan_x = 0.0;
GLdouble   pan_y = 0.0;
GLdouble   pan_z = 0.0;
GLuint     mode = WIRE_FRAME;

GLUI	   *glui;
int		   windowId;

//light location
GLfloat xlight = 0.0;
GLfloat ylight = 0.0;
GLfloat zlight = 1.0;

#define _WIN32

#if defined(_WIN32)
#include <sys/timeb.h>
#define CLK_TCK 1000
#else
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#endif

void createGLUI(void);

float
elapsed(void)
{
    static long begin = 0;
    static long finish, difference;
    
#if defined(_WIN32)
    static struct timeb tb;
    ftime(&tb);
    finish = tb.time*1000+tb.millitm;
#else
    static struct tms tb;
    finish = times(&tb);
#endif
    
    difference = finish - begin;
    begin = finish;
    
    return (float)difference/(float)CLK_TCK;
}

void
shadowtext(int x, int y, char* s) 
{
    int lines;
    char* p;
    
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 
        0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor3ub(0, 0, 0);
    glRasterPos2i(x+1, y-1);
    for(p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x+1, y-1-(lines*18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glColor3ub(0, 128, 255);
    glRasterPos2i(x, y);
    for(p = s, lines = 0; *p; p++) {
        if (*p == '\n') {
            lines++;
            glRasterPos2i(x, y-(lines*18));
        }
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

GLuint loadShaders(const char* vertex, const char* fragment) {
	GLuint g_program;
	GLint result;

	// Code from TA web page (GLSL example)
	/* create program object and attach shaders */
	g_program = glCreateProgram();
	SM.shaderAttachFromFile(g_program, GL_VERTEX_SHADER, vertex);
	SM.shaderAttachFromFile(g_program, GL_FRAGMENT_SHADER, fragment);

	/* link the program and make sure that there were no errors */
	glLinkProgram(g_program);
	glGetProgramiv(g_program, GL_LINK_STATUS, &result);
	if(result == GL_FALSE) {
		GLint length;
		char *log;

		/* get the program info log */
		glGetProgramiv(g_program, GL_INFO_LOG_LENGTH, &length);
		log = (char*) malloc(length);
		glGetProgramInfoLog(g_program, length, &result, log);

		/* print an error message and the info log */
		fprintf(stderr, "sceneInit(): Program linking failed: %s\n", log);
		free(log);

		/* delete the program */
		glDeleteProgram(g_program);
		g_program = 0;
	}
	return g_program;
}

void init(void)
{
	model->g_goochprogram = loadShaders("shaders/goochS.vp", "shaders/goochS.fp");
	model->g_texprogram = loadShaders("shaders/texture.vp", "shaders/texture.fp");

    gltbInit(GLUT_LEFT_BUTTON);

 	GLfloat diff0[] = {0.8, 0.8, 0.8, 1.0};
	GLfloat spec0[] = {0.7, 0.7, 0.7, 1.0};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diff0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);

	// light location
	//GLfloat pos0[] = {15.0, 5.0, 3.0, 1.0};
	//glLightfv(GL_LIGHT0, GL_POSITION, pos0);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

	GLfloat amb[] = {0.5, 0.5, 0.5, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

    //glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	
    glEnable(GL_DEPTH_TEST);

	// default mode
	mode = WHITE;

}

void
reshape(int width, int height)
{
    gltbReshape(width, height);
    
    glViewport(0, 0, width, height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)height / (GLfloat)width, 1.0, 128.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -3.0);
}

//invert a 4x4 matrix (code from http://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix)
bool gluInvertMatrix(const double m[16], double invOut[16])
{
    double inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

#define NUM_FRAMES 5
void display(void)
{
	glui->sync_live();
    static char s[256], t[32];
    static char* p;
    static int frames = 0;
    
    glClearColor(1.0, 1.0, 1.0, 1.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    
	// light location
	GLfloat pos0[] = {xlight, ylight, zlight, 1.0};
	glLightfv(GL_LIGHT0, GL_POSITION, pos0);

    glPushMatrix();
    glTranslatef(pan_x, pan_y, 0.0);
    gltbMatrix();

	// light location
	/*GLfloat myT[4][4];
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)myT);
	GLfloat pos0[] = { ((GLfloat*)myT)[0], ((GLfloat*)myT)[4], ((GLfloat*)myT)[8], 1.0};
	glLightfv(GL_LIGHT0, GL_POSITION, pos0);*/
	
	if (model->featureLines) {
		// get camera location for edge buffer
		GLdouble mvMat[16];
		GLdouble mvMatI[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, (GLdouble*)mvMat);
		gluInvertMatrix(mvMat, mvMatI);
		model->updateEdgeBuffer(mvMatI[12], mvMatI[13], mvMatI[14]);
	}

	model->render(mode);	// render

    glDisable(GL_LIGHTING);

    glPopMatrix();
    
    if (stats) 
	{
        int height = glutGet(GLUT_WINDOW_HEIGHT);
        glColor3ub(0, 0, 0);
        sprintf(s, "%s\n%d vertices\n%d triangles\n%d normals\n"
            "%d texcoords\n%d groups\n%d materials",
            model->inq_pathname(), model->inq_numvertices(), model->inq_numtriangles(), 
            model->inq_numnormals(), model->inq_numtexcoords(), model->inq_numgroups(),
            model->inq_nummaterials());
        shadowtext(5, height-(5+18*1), s);
    }
    
    /* spit out frame rate. */
    frames++;
    if (frames > NUM_FRAMES) {
        sprintf(t, "%g fps", frames/elapsed());
        frames = 0;
    }
    if (performance) {
        shadowtext(5, 5, t);
    }
    
    glutSwapBuffers();
    glEnable(GL_LIGHTING);
}

void SpecialFunc(int key, int x, int y) {
	GLfloat lightDelta = 0.1;

	switch(key) {
	case GLUT_KEY_LEFT:
        xlight -= lightDelta;
		break;
	case GLUT_KEY_RIGHT:
		xlight += lightDelta;
		break;
	case GLUT_KEY_DOWN:
		ylight -= lightDelta;
		break;
	case GLUT_KEY_UP:
		ylight += lightDelta;
		break;
	}
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y)
{  
	GLfloat lightDelta = 0.1;

    switch (key) {
    case 'h':
        printf("help\n\n");

        printf("w                    -  wireframe\n");
		printf("q                    -  feature lines on/off\n");
		printf("e                    -  edge lines only mode\n");
		printf("z                    -  render all triangles in white\n");
		printf("x                    -  print edge buffer to console\n");
        printf("c                    -  culling\n");
        printf("r                    -  hidden surface removal\n");
		printf("f                    -  flat shading\n");
		printf("s                    -  smooth shading\n");
		printf("g                    -  gooch shading\n");
		printf("1                    -  texture with checkboard pattern\n");
		printf("2                    -  texture with image\n");
		printf("v                    -  reverse winding\n");
		printf("+/-                  -  scale model smaller/larger\n");
        printf("i                    -  show model stats\n");
		printf("3 - 0                -  gooch parameters +- (blue, yellow, alpha, beta)\n");
		printf("o/p                  -  Orientation-based r down/up\n");
		printf("k/l                  -  Depth-based r down/up\n");
		printf("n/m                  -  Depth-based zmin down/up\n");
        printf("arrow keys, [, and ] -  move light source\n");
        printf("q/escape  -  Quit\n\n");
        break;
	case 'x':
		model->printEdgeBuffer();
		break;

	case 'o':
		model->rOrient -= 0.1f;
		if (model->rOrient < 0.0f)
			model->rOrient = 0.0f;
		printf("rOrient: %f\n", model->rOrient);
		break;
	case 'p':
		model->rOrient += 0.1f;
		printf("rOrient: %f\n", model->rOrient);
		break;
	case 'k':
		model->rDepth -= 0.05f;
		if (model->rDepth <= 1.0f)
			model->rDepth = 1.0001f;
		printf("rDepth: %f\n", model->rDepth);
		break;
	case 'l':
		model->rDepth += 0.05f;
		printf("rDepth: %f\n", model->rDepth);
		break;
	case 'n':
		model->zminDepth -= 0.1f;
		printf("zminDepth: %f\n", model->zminDepth);
		break;
	case 'm':
		model->zminDepth += 0.1f;
		printf("zminDepth: %f\n", model->zminDepth);
		break;
	case '[':
		zlight -= lightDelta;
		break;
	case ']':
		zlight += lightDelta;
		break;
    case 'w':
        mode = WIRE_FRAME;
        break;
	case 'q':
		model->featureLines = model->featureLines != true;
        break;
	case 'z':
		mode = WHITE;
		break;
	case 'e':
		mode = EDGEB;
        break;
    case 'c':
        mode = CULLING;
        break;
    case 'r':
        mode = HSR;
        break;
    case 'f':
        mode = FLAT;
        break;
    case 's':
        mode = SMOOTH;
        break;
    case '1':
		pix[0].makeCheckerBoard();
		pix[0].setTexture(1);
        mode = TEXTURE;
		model->set_texid(1);
        break;
    case '2':
		pix[0].readBMPFile("sample.bmp");
		pix[0].setTexture(1);
        mode = TEXTURE;
		model->set_texid(1);
        break;
	case 'g':
		mode = GOOCH;
		break;
    case 'v':
        model->reverse_winding();
        break;
    case '-':
        model->scale(0.8);
        break;
    case '+':
        model->scale(1.25);
        break;
    case 'i':
        stats = !stats;
        break;
    case '3':
	model->kblue[2] -= 0.05f;
	if (model->kblue[2] < 0.0f)
		model->kblue[2] = 0.0f;
	if (model->kblue[2] > 1.0f)
		model->kblue[2] = 1.0f;
	printf("kblue: %f\n", model->kblue[2]);
	break;

    case '4':
	model->kblue[2] += 0.05f;
	if (model->kblue[2] < 0.0f)
		model->kblue[2] = 0.0f;
	if (model->kblue[2] > 1.0f)
		model->kblue[2] = 1.0f;
	printf("kblue: %f\n", model->kblue[2]);
	break;

    case '5':
	model->kyellow[0] -= 0.05f;
	model->kyellow[1] -= 0.05f;
	if (model->kyellow[0] < 0.0f) {
		model->kyellow[0] = 0.0f;
		model->kyellow[1] = 0.0f;
	}
	if (model->kyellow[0] > 1.0f) {
		model->kyellow[0] = 1.0f;
		model->kyellow[1] = 1.0f;
	}
	printf("kyellow: %f\n", model->kyellow[0]);
	break;

    case '6':
	model->kyellow[0] += 0.05f;
	model->kyellow[1] += 0.05f;
	if (model->kyellow[0] < 0.0f) {
		model->kyellow[0] = 0.0f;
		model->kyellow[1] = 0.0f;
	}
	if (model->kyellow[0] > 1.0f) {
		model->kyellow[0] = 1.0f;
		model->kyellow[1] = 1.0f;
	}
	printf("kyellow: %f\n", model->kyellow[0]);
	break;

    case '7':
	model->alpha -= 0.05f;
	if (model->alpha < 0.0f)
		model->alpha = 0.0f;
	if (model->alpha > 1.0f)
		model->alpha = 1.0f;
	printf("alpha: %f\n", model->alpha);
	break;

    case '8':
	model->alpha += 0.05f;
	if (model->alpha < 0.0f)
		model->alpha = 0.0f;
	if (model->alpha > 1.0f)
		model->alpha = 1.0f;
	printf("alpha: %f\n", model->alpha);
	break;
    case '9':
	model->beta -= 0.05f;
	if (model->beta < 0.0f)
		model->beta = 0.0f;
	if (model->beta > 1.0f)
		model->beta = 1.0f;
	printf("beta: %f\n", model->beta);
	break;

    case '0':
	model->beta += 0.05f;
	if (model->beta < 0.0f)
		model->beta = 0.0f;
	if (model->beta > 1.0f)
		model->beta = 1.0f;
	printf("beta: %f\n", model->beta);
	break;

    case 27:
        exit(0);
        break;
    }
    
    glutPostRedisplay();
}

void menu(int item) {
	 int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;
    
    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
        dirp = opendir(DATA_DIR);
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;
        name = (char*)malloc(strlen(direntp->d_name) + strlen(DATA_DIR) + 1);
        strcpy(name, DATA_DIR);
        strcat(name, direntp->d_name);
		if (model)
		{
			model->free_it();
			model = NULL;
		}
		model = new Model();
		model->read_obj(name);
		scale = model->unitize();
		model->facet_normals();
		model->vertex_normals();
		model->linear_texture();
		//model->spheremap_texture();

		createGLUI();

		init();
		/*
		
		model->facet_normals();
		model->vertex_normals(smoothing_angle);
        
        if (model->inq_nummaterials() > 0)
            material_mode = 2;
        else
            material_mode = 0;
        
        lists();
		*/

        delete(name);
        
        glutPostRedisplay();
    }
}

void menua2(int item) {
	 int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;
    
    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
        dirp = opendir(DATAA2_DIR);
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;
        name = (char*)malloc(strlen(direntp->d_name) + strlen(DATAA2_DIR) + 1);
        strcpy(name, DATAA2_DIR);
        strcat(name, direntp->d_name);
		if (model)
		{
			model->free_it();
			model = NULL;
		}
		model = new Model();
		model->read_obj(name);
		scale = model->unitize();
		model->facet_normals();
		model->vertex_normals();
		model->linear_texture();
		//model->spheremap_texture();

		createGLUI();

		init();

		/*
		
		model->facet_normals();
		model->vertex_normals(smoothing_angle);
        
        if (model->inq_nummaterials() > 0)
            material_mode = 2;
        else
            material_mode = 0;
        
        lists();
		*/

        delete(name);
        
        glutPostRedisplay();
    }
}

void apmenu(int item) {
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;

    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
        dirp = opendir(AERIALPER_DIR);
        while ((direntp = readdir(dirp))) {
            if (strstr(direntp->d_name, ".bmp")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;

        name = (char*)malloc(strlen(direntp->d_name) + strlen(AERIALPER_DIR) + 1);
        strcpy(name, AERIALPER_DIR);
        strcat(name, direntp->d_name);

		mode = TEXTURE;
		// read bmp file
		pix[0].readBMPFile(name);
		pix[0].setTexture(1);
		model->set_texid(1);

		model->toneDetail = 1;
		printf("Depth-based\n");

        delete(name);
        
        glutPostRedisplay();
    }
}

void dfmenu(int item) {
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;

    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
		dirp = opendir(DEPTHFIELD_DIR);
        while ((direntp = readdir(dirp))) {
            if (strstr(direntp->d_name, ".bmp")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;

        name = (char*)malloc(strlen(direntp->d_name) + strlen(DEPTHFIELD_DIR) + 1);
        strcpy(name, DEPTHFIELD_DIR);
        strcat(name, direntp->d_name);

		mode = TEXTURE;
		pix[0].readBMPFile(name);
		pix[0].setTexture(1);
		model->set_texid(1);

		model->toneDetail = 1;
		printf("Depth-based\n");

        delete(name);
        
        glutPostRedisplay();
    }
}

void mmenu(int item) {
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;
 
    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
		dirp = opendir(MATERIAL_DIR);
        while ((direntp = readdir(dirp))) {
            if (strstr(direntp->d_name, ".bmp")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;

        name = (char*)malloc(strlen(direntp->d_name) + strlen(MATERIAL_DIR) + 1);
        strcpy(name, MATERIAL_DIR);
        strcat(name, direntp->d_name);

		mode = TEXTURE;
		pix[0].readBMPFile(name);
		pix[0].setTexture(1);
		model->set_texid(1);

		model->toneDetail = 0;
		printf("Orientation-based.\n");

        delete(name);
        
        glutPostRedisplay();
    }
}

void sbmenu(int item) {
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;
 
    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
		dirp = opendir(SILHBCKLIG_DIR);
        while ((direntp = readdir(dirp))) {
            if (strstr(direntp->d_name, ".bmp")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;

        name = (char*)malloc(strlen(direntp->d_name) + strlen(SILHBCKLIG_DIR) + 1);
        strcpy(name, SILHBCKLIG_DIR);
        strcat(name, direntp->d_name);

		mode = TEXTURE;
		pix[0].readBMPFile(name);
		pix[0].setTexture(1);
		model->set_texid(1);

		model->toneDetail = 0;
		printf("Orientation-based.\n");

        delete(name);
        
        glutPostRedisplay();
    }
}

void tmenu(int item) {
    int i = 0;
    DIR* dirp;
    char* name;
    struct dirent* direntp;
 
    if (item > 0) {
        keyboard((unsigned char)item, 0, 0);
    } else {
		dirp = opendir(TOON_DIR);
        while ((direntp = readdir(dirp))) {
            if (strstr(direntp->d_name, ".bmp")) {
                i++;
                if (i == -item)
                    break;
            }
        }
        if (!direntp)
            return;

        name = (char*)malloc(strlen(direntp->d_name) + strlen(TOON_DIR) + 1);
        strcpy(name, TOON_DIR);
        strcat(name, direntp->d_name);

		mode = TEXTURE;
		pix[0].readBMPFile(name);
		pix[0].setTexture(1);
		model->set_texid(1);

		model->toneDetail = 1;
		printf("Depth-based\n");

        delete(name);
        
        glutPostRedisplay();
    }
}

static GLint      mouse_state;
static GLint      mouse_button;

void
mouse(int button, int state, int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];
    
    /* fix for two-button mice -- left mouse + shift = middle mouse */
    if (button == GLUT_LEFT_BUTTON && glutGetModifiers() & GLUT_ACTIVE_SHIFT)
        button = GLUT_MIDDLE_BUTTON;
    
    gltbMouse(button, state, x, y);
    
    mouse_state = state;
    mouse_button = button;
    
    if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }
    
    glutPostRedisplay();
}

void
motion(int x, int y)
{
    GLdouble model[4*4];
    GLdouble proj[4*4];
    GLint view[4];
    
    gltbMotion(x, y);
    
    if (mouse_state == GLUT_DOWN && mouse_button == GLUT_MIDDLE_BUTTON) {
        glGetDoublev(GL_MODELVIEW_MATRIX, model);
        glGetDoublev(GL_PROJECTION_MATRIX, proj);
        glGetIntegerv(GL_VIEWPORT, view);
        gluProject((GLdouble)x, (GLdouble)y, 0.0,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        gluUnProject((GLdouble)x, (GLdouble)y, pan_z,
            model, proj, view,
            &pan_x, &pan_y, &pan_z);
        pan_y = -pan_y;
    }
    
    glutPostRedisplay();
}

// GLUI UI
void createGLUI() {
	GLUI_Panel* mainRoll;
	GLUI_Scrollbar* sb;
	GLUI_Rollout* roll;
	GLUI_EditText *edittext;
	GLUI_RadioGroup* rg;

	GLUI_Master.close_all();
	glui = GLUI_Master.create_glui("Options", 0, 512, 512);

	mainRoll = glui->add_panel("Feature Line Options");

	// on/off toggles
	glui->add_statictext_to_panel(mainRoll, "On/Off");
	glui->add_checkbox_to_panel(mainRoll, "Boundary", &model->boundary);
	glui->add_checkbox_to_panel(mainRoll, "Silhouette", &model->silhouette);
	glui->add_checkbox_to_panel(mainRoll, "Crease (Model)", &model->creaseModel);
	glui->add_checkbox_to_panel(mainRoll, "Crease (User)", &model->creaseUser);
	glui->add_checkbox_to_panel(mainRoll, "Slope Steepness", &model->slopeSteepness);
	glui->add_separator_to_panel(mainRoll);
	edittext = glui->add_edittext_to_panel(mainRoll, "Line Width:", GLUI_EDITTEXT_FLOAT, &model->featureLineWidth);
	edittext->set_float_limits(1.0, 10.0);
	sb = new GLUI_Scrollbar(mainRoll, "Line Width", GLUI_SCROLL_HORIZONTAL, &model->featureLineWidth);
	sb->set_float_limits(1.0, 10.0);

	// rollouts for line colors and styles
	glui->add_column_to_panel(mainRoll);
	glui->add_statictext_to_panel(mainRoll, "Color/Style");

	// boundary color and style
	roll = glui->add_rollout_to_panel(mainRoll, "Boundary", false);
	edittext = glui->add_edittext_to_panel(roll, "Red:", GLUI_EDITTEXT_FLOAT, &model->boundaryColor[0]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Red:", GLUI_SCROLL_HORIZONTAL, &model->boundaryColor[0]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Green:", GLUI_EDITTEXT_FLOAT, &model->boundaryColor[1]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Green:", GLUI_SCROLL_HORIZONTAL, &model->boundaryColor[1]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Blue:", GLUI_EDITTEXT_FLOAT, &model->boundaryColor[2]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Blue:", GLUI_SCROLL_HORIZONTAL, &model->boundaryColor[2]);
	sb->set_float_limits(0.0, 1.0);
	glui->add_statictext_to_panel(roll, "Style (Front Facing Only)");
	rg = glui->add_radiogroup_to_panel(roll, &model->boundaryFrontStyle);
	glui->add_radiobutton_to_group(rg, "Solid");
	glui->add_radiobutton_to_group(rg, "Dotted");
	glui->add_radiobutton_to_group(rg, "Dashed");
	glui->add_statictext_to_panel(roll, "Style (Back Facing Only)");
	rg = glui->add_radiogroup_to_panel(roll, &model->boundaryBackStyle);
	glui->add_radiobutton_to_group(rg, "Solid");
	glui->add_radiobutton_to_group(rg, "Dotted");
	glui->add_radiobutton_to_group(rg, "Dashed");


	// silhouette color and style
	roll = glui->add_rollout_to_panel(mainRoll, "Silhouette", false);
	edittext = glui->add_edittext_to_panel(roll, "Red:", GLUI_EDITTEXT_FLOAT, &model->silhouetteColor[0]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Red:", GLUI_SCROLL_HORIZONTAL, &model->silhouetteColor[0]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Green:", GLUI_EDITTEXT_FLOAT, &model->silhouetteColor[1]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Green:", GLUI_SCROLL_HORIZONTAL, &model->silhouetteColor[1]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Blue:", GLUI_EDITTEXT_FLOAT, &model->silhouetteColor[2]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Blue:", GLUI_SCROLL_HORIZONTAL, &model->silhouetteColor[2]);
	sb->set_float_limits(0.0, 1.0);
	glui->add_statictext_to_panel(roll, "Style");
	rg = glui->add_radiogroup_to_panel(roll, &model->silhouetteStyle);
	glui->add_radiobutton_to_group(rg, "Solid");
	glui->add_radiobutton_to_group(rg, "Dotted");
	glui->add_radiobutton_to_group(rg, "Dashed");

	// crease (model) color and style
	roll = glui->add_rollout_to_panel(mainRoll, "Crease (Model)", false);
	edittext = glui->add_edittext_to_panel(roll, "Red:", GLUI_EDITTEXT_FLOAT, &model->creaseModelColor[0]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Red:", GLUI_SCROLL_HORIZONTAL, &model->creaseModelColor[0]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Green:", GLUI_EDITTEXT_FLOAT, &model->creaseModelColor[1]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Green:", GLUI_SCROLL_HORIZONTAL, &model->creaseModelColor[1]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Blue:", GLUI_EDITTEXT_FLOAT, &model->creaseModelColor[2]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Blue:", GLUI_SCROLL_HORIZONTAL, &model->creaseModelColor[2]);
	sb->set_float_limits(0.0, 1.0);
	glui->add_statictext_to_panel(roll, "Style (Front Facing Only)");
	rg = glui->add_radiogroup_to_panel(roll, &model->creaseModelFrontStyle);
	glui->add_radiobutton_to_group(rg, "Solid");
	glui->add_radiobutton_to_group(rg, "Dotted");
	glui->add_radiobutton_to_group(rg, "Dashed");
	glui->add_statictext_to_panel(roll, "Style (Back Facing Only)");
	rg = glui->add_radiogroup_to_panel(roll, &model->creaseModelBackStyle);
	glui->add_radiobutton_to_group(rg, "Solid");
	glui->add_radiobutton_to_group(rg, "Dotted");
	glui->add_radiobutton_to_group(rg, "Dashed");

	// crease (user) color and style
	roll = glui->add_rollout_to_panel(mainRoll, "Crease (User)", false);
	edittext = glui->add_edittext_to_panel(roll, "Red:", GLUI_EDITTEXT_FLOAT, &model->creaseUserColor[0]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Red:", GLUI_SCROLL_HORIZONTAL, &model->creaseUserColor[0]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Green:", GLUI_EDITTEXT_FLOAT, &model->creaseUserColor[1]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Green:", GLUI_SCROLL_HORIZONTAL, &model->creaseUserColor[1]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Blue:", GLUI_EDITTEXT_FLOAT, &model->creaseUserColor[2]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Blue:", GLUI_SCROLL_HORIZONTAL, &model->creaseUserColor[2]);
	sb->set_float_limits(0.0, 1.0);
	glui->add_statictext_to_panel(roll, "Style");
	rg = glui->add_radiogroup_to_panel(roll, &model->creaseUserStyle);
	glui->add_radiobutton_to_group(rg, "Solid");
	glui->add_radiobutton_to_group(rg, "Dotted");
	glui->add_radiobutton_to_group(rg, "Dashed");

	// slope steepness color and style
	roll = glui->add_rollout_to_panel(mainRoll, "Slope Steepness", false);
	edittext = glui->add_edittext_to_panel(roll, "Red:", GLUI_EDITTEXT_FLOAT, &model->slopeSteepnessColor[0]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Red:", GLUI_SCROLL_HORIZONTAL, &model->slopeSteepnessColor[0]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Green:", GLUI_EDITTEXT_FLOAT, &model->slopeSteepnessColor[1]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Green:", GLUI_SCROLL_HORIZONTAL, &model->slopeSteepnessColor[1]);
	sb->set_float_limits(0.0, 1.0);
	edittext = glui->add_edittext_to_panel(roll, "Blue:", GLUI_EDITTEXT_FLOAT, &model->slopeSteepnessColor[2]);
	edittext->set_float_limits(0.0, 1.0);
	sb = new GLUI_Scrollbar(roll, "Blue:", GLUI_SCROLL_HORIZONTAL, &model->slopeSteepnessColor[2]);
	sb->set_float_limits(0.0, 1.0);
	glui->add_statictext_to_panel(roll, "Style");
	rg = glui->add_radiogroup_to_panel(roll, &model->slopeSteepnessStyle);
	glui->add_radiobutton_to_group(rg, "Solid");
	glui->add_radiobutton_to_group(rg, "Dotted");
	glui->add_radiobutton_to_group(rg, "Dashed");

	// min and max for crease (model)
	glui->add_column_to_panel(mainRoll);
	glui->add_statictext_to_panel(mainRoll, "Min/Max (degrees)");
	roll = glui->add_rollout_to_panel(mainRoll, "Crease (Model)", false);
	edittext = glui->add_edittext_to_panel(roll, "Min:", GLUI_EDITTEXT_FLOAT, &model->creaseModelMin);
	edittext->set_float_limits(0.0, 360.0);
	edittext = glui->add_edittext_to_panel(roll, "Max:", GLUI_EDITTEXT_FLOAT, &model->creaseModelMax);
	edittext->set_float_limits(0.0, 360.0);

	// min and max for crease (user)
	roll = glui->add_rollout_to_panel(mainRoll, "Crease (User)", false);
	edittext = glui->add_edittext_to_panel(roll, "Min:", GLUI_EDITTEXT_FLOAT, &model->creaseUserMin);
	edittext->set_float_limits(0.0, 360.0);
	edittext = glui->add_edittext_to_panel(roll, "Max:", GLUI_EDITTEXT_FLOAT, &model->creaseUserMax);
	edittext->set_float_limits(0.0, 360.0);

	// min and max for slope steepness
	roll = glui->add_rollout_to_panel(mainRoll, "Slope Steepness (GA)", false);
	edittext = glui->add_edittext_to_panel(roll, "Min:", GLUI_EDITTEXT_FLOAT, &model->slopeSteepnessMin);
	edittext->set_float_limits(0.0, 90.0);
	edittext = glui->add_edittext_to_panel(roll, "Max:", GLUI_EDITTEXT_FLOAT, &model->slopeSteepnessMax);
	edittext->set_float_limits(0.0, 90.0);

	glui->add_column_to_panel(mainRoll);
	glui->add_statictext_to_panel(mainRoll, "Slope Steepness Rotation");
	edittext = glui->add_edittext_to_panel(mainRoll, "X axis:", GLUI_EDITTEXT_FLOAT, &model->slopeSteepnessRotateX);
	edittext->set_float_limits(-90.0, 90.0);
	sb = new GLUI_Scrollbar(mainRoll, "X axis:", GLUI_SCROLL_HORIZONTAL, &model->slopeSteepnessRotateX);
	sb->set_float_limits(-90.0, 90.0);
	edittext = glui->add_edittext_to_panel(mainRoll, "Y axis:", GLUI_EDITTEXT_FLOAT, &model->slopeSteepnessRotateY);
	edittext->set_float_limits(-90.0, 90.0);
	sb = new GLUI_Scrollbar(mainRoll, "Y axis:", GLUI_SCROLL_HORIZONTAL, &model->slopeSteepnessRotateY);
	sb->set_float_limits(-90.0, 90.0);

	glui->set_main_gfx_window(windowId);
}

int
main(int argc, char** argv)
{
    int buffering = GLUT_DOUBLE;
    struct dirent* direntp;
    DIR* dirp;
    int models;
	int modelsA2;
	int aptextures;
	int dftextures;
	int mtextures;
	int sbtextures;
	int ttextures;
    
    glutInitWindowSize(512, 512);
    glutInit(&argc, argv);
    
    while (--argc) {
        if (strcmp(argv[argc], "-sb") == 0)
            buffering = GLUT_SINGLE;
        else
            model_file = argv[argc];
    }
    
    if (!model_file) {
        //model_file = "data/beethovan_bust.obj";
		//model_file = "data/A2/venus.obj";
		model_file = "data/A2/terrain.obj";
    }
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | buffering);
    windowId = glutCreateWindow("CPSC591");
    
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
	glutSpecialFunc(SpecialFunc);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    
    models = glutCreateMenu(menu);
    dirp = opendir(DATA_DIR);
    if (!dirp) {
        fprintf(stderr, "%s: can't open data directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
	entries = 0;

	modelsA2 = glutCreateMenu(menua2);
    dirp = opendir(DATAA2_DIR);
    if (!dirp) {
        fprintf(stderr, "%s: can't open data directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".obj")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
	entries = 0;

	aptextures = glutCreateMenu(apmenu);
	dirp = opendir(AERIALPER_DIR);
	if (!dirp) {
        fprintf(stderr, "%s: can't open aerial-persp directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".bmp")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
	entries = 0;

	dftextures = glutCreateMenu(dfmenu);
	dirp = opendir(DEPTHFIELD_DIR);
	if (!dirp) {
        fprintf(stderr, "%s: can't open depth-field directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".bmp")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
	entries = 0;

	mtextures = glutCreateMenu(mmenu);
	dirp = opendir(MATERIAL_DIR);
	if (!dirp) {
        fprintf(stderr, "%s: can't open material directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".bmp")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
	entries = 0;

	sbtextures = glutCreateMenu(sbmenu);
	dirp = opendir(SILHBCKLIG_DIR);
	if (!dirp) {
        fprintf(stderr, "%s: can't open silh bklg directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".bmp")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
	entries = 0;

	ttextures = glutCreateMenu(tmenu);
	dirp = opendir(TOON_DIR);
	if (!dirp) {
        fprintf(stderr, "%s: can't open toon directory.\n", argv[0]);
    } else {
        while ((direntp = readdir(dirp)) != NULL) {
            if (strstr(direntp->d_name, ".bmp")) {
                entries++;
                glutAddMenuEntry(direntp->d_name, -entries);
            }
        }
        closedir(dirp);
    }
    
    glutCreateMenu(menu);
    glutAddMenuEntry("Rendering Pipeline", 0);
    glutAddMenuEntry("", 0);
    glutAddSubMenu("Models", models);
	glutAddSubMenu("A2 Models", modelsA2);
    glutAddMenuEntry("", 0);
	glutAddSubMenu("Aerial Persp Textures", aptextures);
	glutAddSubMenu("Depth Field Textures", dftextures);
	glutAddSubMenu("Material Textures", mtextures);
	glutAddSubMenu("Silh Bckl Textures", sbtextures);
	glutAddSubMenu("Toon Textures", ttextures);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    
	// read in the model
	model = new Model();
	model->read_obj(model_file);
	scale = model->unitize();
	model->facet_normals();
	model->vertex_normals();
	model->linear_texture();

	GLUI_Master.set_glutIdleFunc( NULL );
	createGLUI();

	GLenum err = glewInit(); // needed for glew
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
        }

    init();
    
    glutMainLoop();
    return 0;
}
