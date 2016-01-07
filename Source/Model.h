#ifndef _ModelH
#define _ModelH

//#############################################################################
//	File:		Model.h
//	Module:		
// 	Description: 	
//
//#############################################################################

//#############################################################################
//	Headers
//#############################################################################

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

#ifdef _MSC_VER
#include <string>
#include <iostream>
#include <fstream>
#include <strstream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;
#else
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <strstream>
#include <stdio.h>
#include <algorithm>
using namespace std;
#endif

#include <GL/glut.h>
#include <math.h>
#include "shader.h"

//#############################################################################
//	Global definitions
//#############################################################################

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#define WIRE_FRAME      (0)           
#define CULLING         (1 << 0)      
#define HSR             (1 << 1)      
#define FLAT            (1 << 2)       
#define SMOOTH          (1 << 3)  
#define TEXTURE         (1 << 4)

#define GOOCH			(1 << 5)
#define EDGEB			(1 << 6)
#define WHITE			(1 << 7)

//#############################################################################
//	Global declarations
//#############################################################################

class Material
{
private:
  // name of material
  char* m_name;    
  // diffuse component
  GLfloat m_diffuse[4]; 
  // ambient component
  GLfloat m_ambient[4];
  // specular component
  GLfloat m_specular[4]; 
  // emmissive component
  GLfloat m_emmissive[4]; 
  // specular exponent
  GLfloat m_shininess;  
  
public:
	Material()
	{
		m_name = NULL;
		m_diffuse[0] = 0.8;
		m_diffuse[1] = 0.8;
		m_diffuse[2] = 0.8;
		m_diffuse[3] = 1.0;

		m_specular[0] = 0.8;
		m_specular[1] = 0.8;
		m_specular[2] = 0.8;
		m_specular[3] = 1.0;

		m_ambient[0] = 0.8;
		m_ambient[1] = 0.8;
		m_ambient[2] = 0.8;
		m_ambient[3] = 1.0;

		m_emmissive[0] = 0.8;
		m_emmissive[1] = 0.8;
		m_emmissive[2] = 0.8;
		m_emmissive[3] = 1.0;

		m_shininess = 65.0;
	}

	Material (char* name, 
		      GLfloat *diffuse, 
			  GLfloat *ambient, 
			  GLfloat *specular,
			  GLfloat *emmisive, 
			  GLfloat shininess);
	~Material() {};

	GLvoid set_name(char* name) {m_name = name;}
	char* inq_name() {return m_name;}

	GLvoid set_ambient(GLfloat r, GLfloat g, GLfloat b) 
	{ m_ambient[0] = r; m_ambient[1] = g; m_ambient[2] = b; }
	GLvoid set_diffuse(GLfloat r, GLfloat g, GLfloat b) 
	{ m_diffuse[0] = r; m_diffuse[1] = g; m_diffuse[2] = b; }
	GLvoid set_specular(GLfloat r, GLfloat g, GLfloat b) 
	{ m_specular[0] = r; m_specular[1] = g; m_specular[2] = b; }
	GLvoid set_emmissive(GLfloat r, GLfloat g, GLfloat b) 
	{ m_emmissive[0] = r; m_emmissive[1] = g; m_emmissive[2] = b; }
	GLvoid set_shininess (GLfloat shininess) { m_shininess = shininess; }

	GLfloat *inq_ambient() { return m_ambient; }
	GLfloat *inq_diffuse() { return m_diffuse; }
	GLfloat *inq_specular() { return m_specular; }
	GLfloat *inq_emmisive() { return m_emmissive; }
	GLfloat inq_shininess() { return m_shininess; }
};

class Triangle 
{
private:
	// array of triangle vertex indices
  GLuint m_vindices[3];   
  // array of triangle normal indices
  GLuint m_nindices[3];  
  // array of triangle texcoord indices
  GLuint m_tindices[3]; 
  // index of triangle facet normal
  GLuint m_findex;   
  
public:
	Triangle()
	{
		m_findex = 0;
		m_vindices[0] = 0; m_vindices[1] = 0; m_vindices[1] = 0;
		m_nindices[0] = 0; m_tindices[1] = 0; m_tindices[1] = 0;
		m_tindices[0] = 0; m_nindices[1] = 0; m_nindices[1] = 0;
	}
	~Triangle() {};

	GLvoid set_findex(GLuint findex) {m_findex = findex;}
	GLvoid set_vindex(GLuint i, GLuint vindex) {m_vindices[i] = vindex;}
	GLvoid set_nindex(GLuint i, GLuint nindex) {m_nindices[i] = nindex;}
	GLvoid set_tindex(GLuint i, GLuint tindex) {m_tindices[i] = tindex;}

	GLuint inq_findex() {return m_findex;}
	GLuint inq_vindex(GLuint i) {return m_vindices[i];}
	GLuint inq_nindex(GLuint i) {return m_nindices[i];}
	GLuint inq_tindex(GLuint i) {return m_tindices[i];}
};


typedef struct _Group 
{
	char*             name;           // name of this group 
	GLuint            numtriangles;   // number of triangles in this group 
	GLuint*           triangles;      // array of triangle indices 
	GLuint            material;       // index to material for group 
	struct _Group* next;              // pointer to next group in model 
} Group;


class Edge
{
public:
	GLuint V;				// vertex 2
	GLuint T1, T2;			// adjacent triangles
	bool A_CModel, A_CUser;	// artist crease flags (model and user)
	bool A_SlopeS;			// artist slope steepness flag
	bool F, B, Fa, Ba;		// face flags

	Edge()
	{
		V = 0;
		T1 = 0; T2 = 0;
		A_CModel = false; A_CUser = false;
		A_SlopeS = false;
		F = false; B = false; Fa = false; Ba = false;
	}
};

class Model
{
private:
	// path to this model
	char* m_pathname;    
	// name of the material library
	char* m_mtllibname;         

	// number of vertices in model
	GLuint m_numvertices; 
	// array of vertices
	GLfloat* m_vertices;         

	// number of normals in model
	GLuint   m_numnormals; 
	// array of normals
	GLfloat* m_normals;             

	// number of texcoords in model
	GLuint m_numtexcoords; 
	// array of texture coordinates
	GLfloat* m_texcoords;
	// texture id (2001 - checkboard; 2002 - image)
	GLuint m_texid;

	// number of facetnorms in model
	GLuint m_numfacetnorms; 
    // array of facetnorms
	GLfloat* m_facetnorms;          

	// number of triangles in model
	GLuint m_numtriangles;
    // array of triangles
	Triangle* m_triangles;      

	// number of materials in model
	GLuint m_nummaterials;
    // array of materials
	Material* m_materials;     

	//number of groups in model
	GLuint m_numgroups;    
	// linked list of groups
	Group *m_groups;                
	
	// position of the model
	GLfloat m_position[3];     

	GLvoid read_MTL(char* name);
	GLuint find_material(char* name);
	GLvoid first_pass(FILE* file);
	GLvoid second_pass(FILE* file);
	Group* find_group(char* name);
	Group* add_group(char* name);

	GLvoid setMVFlags(GLuint i, GLuint j); // Calculate the morphometric variables and set A flags appropriately
	GLvoid createEdgeBuffer(void); // create edge buffer with V, T1 and T2, set A flags
	GLvoid resetEdgeBuffer(void);
	GLvoid updateTriangleEdges(GLfloat nDotE, GLuint i, GLfloat* normal, GLuint v1i, GLuint v2i, GLuint v3i);

public:
	Model();
	~Model();

	GLuint g_texprogram;
	GLuint g_goochprogram;
	GLint locAmbient;
	GLint locDiffuse;
	GLint locSpecular;

	GLint goochKCoolColor;
	GLint goochKWarmColor;
	GLint goochAlpha;
	GLint goochBeta;
	GLfloat alpha;
	GLfloat beta;
	GLfloat kblue[4];
	GLfloat kyellow[4];
	
	GLint toneDetail; // 0 is orientation-based, 1 is depth-based.
	GLfloat rOrient;
	GLfloat rDepth;
	GLfloat zminDepth;

	GLuint maxAdjVertices;
	int featureLines; // feature lines (edge buffer) on (true) or off (false)
	GLfloat featureLineWidth;
	int boundary; // 0 or 1
	int boundaryFrontStyle; // 0 solid, 1 dotted, 2 dashed
	int boundaryBackStyle; // 0 solid, 1 dotted, 2 dashed
	GLfloat boundaryColor[4];
	int silhouette;  // 0 or 1
	int silhouetteStyle; // 0 solid, 1 dotted, 2 dashed
	GLfloat silhouetteColor[4];
	int creaseModel;  // 0 or 1
	int creaseModelFrontStyle; // 0 solid, 1 dotted, 2 dashed
	int creaseModelBackStyle; // 0 solid, 1 dotted, 2 dashed
	GLfloat creaseModelColor[4];
	GLfloat creaseModelMin;
	GLfloat creaseModelMax;
	int creaseUser;  // 0 or 1
	int creaseUserStyle; // 0 solid, 1 dotted, 2 dashed
	GLfloat creaseUserColor[4];
	GLfloat creaseUserMin;
	GLfloat creaseUserMax;
	int slopeSteepness;  // 0 or 1
	int slopeSteepnessStyle; // 0 solid, 1 dotted, 2 dashed
	GLfloat slopeSteepnessColor[4];
	GLfloat slopeSteepnessMin;
	GLfloat slopeSteepnessMax;
	GLfloat slopeSteepnessRotateX;
	GLfloat slopeSteepnessRotateY;
	Edge** m_edges; // edge buffer
	GLvoid printEdgeBuffer(void);
	GLvoid updateEdgeBuffer(GLdouble cameraX, GLdouble cameraY, GLdouble cameraZ); //update flags
	GLvoid shapeMeasures(GLuint i, GLuint j); // Edge-based shape measures to set A flags. Set the crease flags and slope steepness flag (A_CUser, A_CModel, and A_SlopeS) in edge buffer (m_edges) for an edge (index i,j)

	char* inq_pathname() {return m_pathname;}
	GLuint inq_numvertices() {return m_numvertices;}
	GLuint inq_numnormals() {return m_numnormals;}
	GLuint inq_numtexcoords() {return m_numtexcoords;}
	GLuint inq_numfacetnorms() {return m_numfacetnorms;}
	GLuint inq_numtriangles() {return m_numtriangles;}
	GLuint inq_nummaterials() {return m_nummaterials;}
	GLuint inq_numgroups() {return m_numgroups;}

	Material *inq_materials() {return m_materials; }
	GLvoid inq_vertices(GLuint it, Group *group, GLfloat* v1, GLfloat* v2, GLfloat* v3);
	GLfloat inq_vertex(GLuint i) {return m_vertices[i];}
	GLvoid set_vertex(GLuint i, GLfloat v) {m_vertices[i] = v;}
	GLvoid set_texid(GLuint texid) {m_texid = texid;}

    GLvoid read_obj(char* filename);
	GLvoid free_it();
	GLvoid linear_texture();
	GLvoid spheremap_texture();


	// ** WORK ON THE FOLLOWING 13 METHODS ** //

	// model measurements/transformations (3 methods)
	GLvoid compute_dimensions (GLfloat& maxx, GLfloat&minx, 
							   GLfloat& maxy, GLfloat& miny, 
							   GLfloat& maxz, GLfloat& minz,
							   GLfloat& width, GLfloat& height, GLfloat& depth);
	GLfloat unitize();
	GLvoid scale(GLfloat scale);

	// normal computations (3 methods)
	GLvoid reverse_winding();
	GLvoid facet_normals();
	GLvoid vertex_normals();

	// shading (7 methods)
	GLvoid wire_frame_mode(Triangle* triangle, Material* material);
	GLvoid white_mode(Triangle* triangle, Material* material);
	GLvoid edge_buffer_mode(Triangle* triangle, Material* material);
	GLvoid culling_mode(Triangle* triangle, Material* material);
	GLvoid hsr_mode(Triangle* triangle, Material* material);
	GLvoid flat_shading_mode(Triangle* triangle, Material* material);
	GLvoid smooth_shading_mode(Triangle* triangle, Material* material);
	GLvoid texture_mode(Triangle* triangle, Material* material);
	GLvoid gooch_mode(Triangle* triangle, Material* material);
	GLvoid render(GLuint mode);
};


//#############################################################################
//	End
//#############################################################################

#endif /* ModelH */

