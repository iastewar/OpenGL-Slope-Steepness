//#############################################################################
//	File:		Model.cpp
//	Module:		
// 	Description: 	
//
//#############################################################################

//#############################################################################
//	Headers
//#############################################################################

//#include <math.h>
//#include <list>
//#include <iostream>

//using namespace std;

#include "Model.h"

//#############################################################################
//	Local declarations
//#############################################################################

//#############################################################################
// Constructors for class
//#############################################################################

Model::Model()
{
	m_pathname = NULL; 
	m_mtllibname = NULL;         
	m_numvertices = 0;
	m_vertices = NULL;         
	m_numnormals = 0;
	m_normals= NULL;             
	m_numtexcoords = 0;
	m_texcoords = NULL;  
	m_texid = 1;
	m_numfacetnorms = 0; 
	m_facetnorms = NULL;           
	m_numtriangles = 0;
	m_triangles = NULL;      
	m_nummaterials = 0;
	m_materials = NULL;      
	m_numgroups = 0;   
	m_groups = NULL; 

	g_texprogram = 0;
	g_goochprogram = 0;

	alpha = 0.25f;
	beta = 0.4f;
	kblue[0] = 0.0f;
	kblue[1] = 0.0f;
	kblue[2] = 0.75f;
	kblue[3] = 1.0f;
	kyellow[0] = 0.75f;
	kyellow[1] = 0.75f;
	kyellow[2] = 0.0f;
	kyellow[3] = 1.0f;

	toneDetail = 0; // 0 is orientation-based, 1 is depth-based.
	rOrient = 1.0f;
	rDepth = 6.0f;
	zminDepth = 2.5f;

	featureLines = true;
	featureLineWidth = 1.0;
	boundary = true;
	boundaryFrontStyle = 0;
	boundaryBackStyle = 0;
	boundaryColor[0] = 0.f; boundaryColor[1] = 0.f; boundaryColor[2] = 0.f; boundaryColor[3] = 1.f;
	silhouette = true;
	silhouetteStyle = 0;
	silhouetteColor[0] = 0.f; silhouetteColor[1] = 0.f; silhouetteColor[2] = 0.f; silhouetteColor[3] = 1.f;
	creaseModel = true;
	creaseModelFrontStyle = 0;
	creaseModelBackStyle = 0;
	creaseModelColor[0] = 0.f; creaseModelColor[1] = 0.f; creaseModelColor[2] = 0.f; creaseModelColor[3] = 1.f;
	creaseModelMin = 85.f;
	creaseModelMax = 95.f;
	creaseUser = false;
	creaseUserStyle = 0;
	creaseUserColor[0] = 0.f; creaseUserColor[1] = 0.f; creaseUserColor[2] = 0.f; creaseUserColor[3] = 1.f;
	creaseUserMin = 20.f;
	creaseUserMax = 40.f;
	slopeSteepness = true;
	slopeSteepnessStyle = 0;
	slopeSteepnessColor[0] = 0.f; slopeSteepnessColor[1] = 0.f; slopeSteepnessColor[2] = 0.f; slopeSteepnessColor[3] = 1.f;
	slopeSteepnessMin = 27.5f;
	slopeSteepnessMax = 57.3f;
	slopeSteepnessRotateX = 0.f;
	slopeSteepnessRotateY = 0.f;
}

//#############################################################################
// Destructor for class
//#############################################################################

Model::~Model()
{
	free_it();
}


//#############################################################################
//  math/vector functions (plain C)
//#############################################################################
static GLfloat maximum_value(GLfloat a, GLfloat b) 
{
    if (b > a)
        return b;
    return a;
}

static GLfloat absolute_value(GLfloat f)
{
    if (f < 0)
        return -f;
    return f;
}

static GLvoid cross_product(GLfloat* u, GLfloat* v, GLfloat* n)
{
    n[0] = u[1]*v[2] - u[2]*v[1];
    n[1] = u[2]*v[0] - u[0]*v[2];
    n[2] = u[0]*v[1] - u[1]*v[0];
}

static GLvoid normalize_vector(GLfloat* v)
{
    GLfloat l;
    
    l = (GLfloat)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}

//#############################################################################
// Generates texture coordinates according to a
// linear projection of the texture map.  It generates these by
// linearly mapping the vertices onto a square.
//#############################################################################
GLvoid Model::linear_texture()
{
	Group *group;
    GLfloat dimensions[3];
    GLfloat x, y, scalefactor;
    GLuint i;
    
    if (m_texcoords) delete [] m_texcoords;
    m_numtexcoords = m_numvertices;
    m_texcoords= new GLfloat [2*(m_numtexcoords+1)];
    
	GLfloat a, b, c, d, e, f;
    compute_dimensions(a, b, c, d, e, f,
		               dimensions[0], dimensions[1], dimensions[2]);

    scalefactor = 2.0 / absolute_value(maximum_value(maximum_value(dimensions[0], dimensions[1]), dimensions[2]));
    
    // do the calculations 
    for(i = 1; i <= m_numvertices; i++) {
        x = m_vertices[3 * i + 0] * scalefactor;
        y = m_vertices[3 * i + 2] * scalefactor;
        m_texcoords[2 * i + 0] = (x + 1.0) / 2.0;
        m_texcoords[2 * i + 1] = (y + 1.0) / 2.0;
    }
    
    // go through and put texture coordinate indices in all the triangles 
    group = m_groups;
    while(group) 
	{
        for(i = 0; i < group->numtriangles; i++) 
		{
            m_triangles[group->triangles[i]].set_tindex(0,m_triangles[group->triangles[i]].inq_vindex(0));
            m_triangles[group->triangles[i]].set_tindex(1,m_triangles[group->triangles[i]].inq_vindex(1));
            m_triangles[group->triangles[i]].set_tindex(2,m_triangles[group->triangles[i]].inq_vindex(2));
        }    
        group = group->next;
    }
}

//#############################################################################
// Generates texture coordinates according to a
// spherical projection of the texture map.  Sometimes referred to as
// spheremap, or reflection map texture coordinates.  It generates
// these by using the normal to calculate where that vertex would map
// onto a sphere.  Since it is impossible to map something flat
// perfectly onto something spherical, there is distortion at the
// poles.  This particular implementation causes the poles along the X
// axis to be distorted.
//#############################################################################
GLvoid Model::spheremap_texture()
{
	Group* group;
    GLfloat theta, phi, rho, x, y, z, r;
    GLuint i;

    if (m_texcoords) delete [] m_texcoords;
    m_numtexcoords = m_numnormals;
    m_texcoords= new GLfloat[2*(m_numtexcoords+1)];
    
    for (i = 1; i <= m_numnormals; i++) 
	{
		// re-arrange for pole distortion
        z = m_normals[3 * i + 0];  
        y = m_normals[3 * i + 1];
        x = m_normals[3 * i + 2];
        r = sqrt((x * x) + (y * y));
        rho = sqrt((r * r) + (z * z));
        
        if(r == 0.0) 
		{
            theta = 0.0;
            phi = 0.0;
        } 
		else 
		{
			if(z == 0.0)
                phi = 3.14159265 / 2.0;
            else
                phi = acos(z / rho);
            
            if(y == 0.0)
                theta = 3.141592365 / 2.0;
            else
                theta = asin(y / r) + (3.14159265 / 2.0);
        }
        
        m_texcoords[2 * i + 0] = theta / 3.14159265;
        m_texcoords[2 * i + 1] = phi / 3.14159265;
    }
    
    // go through and put texcoord indices in all the triangles 
    group = m_groups;
    while(group) 
	{
		for(i = 0; i < group->numtriangles; i++) 
		{
            m_triangles[group->triangles[i]].set_tindex(0,m_triangles[group->triangles[i]].inq_vindex(0));
            m_triangles[group->triangles[i]].set_tindex(1,m_triangles[group->triangles[i]].inq_vindex(1));
            m_triangles[group->triangles[i]].set_tindex(2,m_triangles[group->triangles[i]].inq_vindex(2));
        }    
        group = group->next;
    }
}

//#############################################################################
// Deletes a Model structure.
//#############################################################################
GLvoid Model::free_it()
{
	if (m_pathname)   delete m_pathname;
    if (m_mtllibname) delete m_mtllibname;
    if (m_vertices)   delete [] m_vertices;
    if (m_normals)    delete [] m_normals;
    if (m_texcoords)  delete [] m_texcoords;
    if (m_facetnorms) delete [] m_facetnorms;
    if (m_triangles)  delete [] m_triangles;

	if (m_materials) 
	{
		for (GLuint i = 0; i < m_nummaterials; i++)
            delete (m_materials[i].inq_name());
    }
    delete [] m_materials;
	
	while(m_groups) 
	{
		Group* group = m_groups;
        m_groups = m_groups->next;
        delete(group->name);
        delete(group->triangles);
        delete(group);
    }
}

//#############################################################################
// Calculates the dimensions (width, height, depth) of the model.
// Also returns the min/maxs of x, y, z of the model.
//#############################################################################
GLvoid Model::compute_dimensions (GLfloat& maxx, GLfloat&minx, 
								  GLfloat& maxy, GLfloat& miny, 
								  GLfloat& maxz, GLfloat& minz,
								  GLfloat& width, GLfloat& height, GLfloat& depth)
{
	GLuint i;
    
    // get the max/mins 
    maxx = minx = m_vertices[3 + 0];
    maxy = miny = m_vertices[3 + 1];
    maxz = minz = m_vertices[3 + 2];
    for (i = 1; i <= m_numvertices; i++) {
        if (maxx < m_vertices[3 * i + 0])
            maxx = m_vertices[3 * i + 0];
        if (minx > m_vertices[3 * i + 0])
            minx = m_vertices[3 * i + 0];
        
        if (maxy < m_vertices[3 * i + 1])
            maxy = m_vertices[3 * i + 1];
        if (miny > m_vertices[3 * i + 1])
            miny = m_vertices[3 * i + 1];
        
        if (maxz < m_vertices[3 * i + 2])
            maxz = m_vertices[3 * i + 2];
        if (minz > m_vertices[3 * i + 2])
            minz = m_vertices[3 * i + 2];
    }
    
    // calculate model width, height, and depth 
    width = absolute_value(maxx) + absolute_value(minx);
    height = absolute_value(maxy) + absolute_value(miny);
    depth = absolute_value(maxz) + absolute_value(minz);
}


//#############################################################################
// "unitize" a model by translating it to the origin and
// scaling it to fit in a unit cube around the origin.   
// Returns the scale factor used.
//#############################################################################
GLfloat Model::unitize()
{
	GLuint  i;

	// max/mins of the model
    GLfloat maxx, minx, maxy, miny, maxz, minz;
	// model width, height, and depth 
    GLfloat w, h, d;
	// center of the model
    GLfloat cx, cy, cz;
	// unitizing scale factor 
    GLfloat scale;
    
	// default values
	w = h = d = 2.0;

	// calculates the dimensions the model
	compute_dimensions(maxx, minx, maxy, miny, maxz, minz, w, h, d);
    
    // calculate center of the model
    cx = (maxx + minx) / 2.0;
    cy = (maxy + miny) / 2.0;
    cz = (maxz + minz) / 2.0;
    
    // calculate unitizing scale factor 
    scale = 2.0 / maximum_value( maximum_value(w, h), d );
    
    // translate around center then scale
    for (i = 1; i <= m_numvertices; i++) 
	{
        m_vertices[3 * i + 0] -= cx;
        m_vertices[3 * i + 1] -= cy;
        m_vertices[3 * i + 2] -= cz;
        m_vertices[3 * i + 0] *= scale;
        m_vertices[3 * i + 1] *= scale;
        m_vertices[3 * i + 2] *= scale;
    }
    
	// returns the scale factor used
    return scale;
}

//#############################################################################
// Scales a model by a given amount.
// scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
//#############################################################################
GLvoid Model::scale(GLfloat scale)
{
    GLuint i;
    
    for (i = 1; i <= m_numvertices; i++) 
	{
        m_vertices[3 * i + 0] *= scale;
        m_vertices[3 * i + 1] *= scale;
        m_vertices[3 * i + 2] *= scale;
    }
}

//#############################################################################
// Reverse the polygon winding for all polygons in
// this model.   Default winding is counter-clockwise.  
// Also changes the direction of the normals.
//#############################################################################
GLvoid Model::reverse_winding()
{
	GLuint i, swap;
    
    for (i = 0; i < m_numtriangles; i++) 
	{
		swap = m_triangles[i].inq_vindex(0);
        m_triangles[i].set_vindex(0,m_triangles[i].inq_vindex(2));
        m_triangles[i].set_vindex(2,swap);
        
        if (m_numnormals) 
		{
            swap = m_triangles[i].inq_nindex(0);
            m_triangles[i].set_nindex(0,m_triangles[i].inq_nindex(2));
            m_triangles[i].set_nindex(2,swap);
        }
        
        if (m_numtexcoords) 
		{
            swap = m_triangles[i].inq_tindex(0);
            m_triangles[i].set_tindex(0,m_triangles[i].inq_tindex(2));
            m_triangles[i].set_tindex(2,swap);
        }
    }
    
    // reverse facet normals 
    for (i = 1; i <= m_numfacetnorms; i++) 
	{
        m_facetnorms[3 * i + 0] = -m_facetnorms[3 * i + 0];
        m_facetnorms[3 * i + 1] = -m_facetnorms[3 * i + 1];
        m_facetnorms[3 * i + 2] = -m_facetnorms[3 * i + 2];
    }
    
    // reverse vertex normals 
    for (i = 1; i <= m_numnormals; i++) 
	{
        m_normals[3 * i + 0] = -m_normals[3 * i + 0];
        m_normals[3 * i + 1] = -m_normals[3 * i + 1];
        m_normals[3 * i + 2] = -m_normals[3 * i + 2];
    }
}

//#############################################################################
// Generates facet normals for a model (by taking the
// cross product of the two vectors derived from the sides of each
// triangle).  Assumes a counter-clockwise winding.
//#############################################################################
GLvoid Model::facet_normals()
{
	GLuint  i;
    GLfloat u[3];
    GLfloat v[3];
    
    // clobber any old facetnormals 
    if (m_facetnorms)
        delete [] m_facetnorms;
    
    // allocate memory for the new facet normals 
    m_numfacetnorms = m_numtriangles;
    m_facetnorms = new GLfloat[3 * (m_numfacetnorms + 1)];
    
    for (i = 0; i < m_numtriangles; i++) 
	{
        m_triangles[i].set_findex(i+1); 
        
        u[0] = m_vertices[3 * m_triangles[i].inq_vindex(1) + 0] -
               m_vertices[3 * m_triangles[i].inq_vindex(0) + 0];
        u[1] = m_vertices[3 * m_triangles[i].inq_vindex(1) + 1] -
               m_vertices[3 * m_triangles[i].inq_vindex(0) + 1];
        u[2] = m_vertices[3 * m_triangles[i].inq_vindex(1) + 2] -
               m_vertices[3 * m_triangles[i].inq_vindex(0) + 2];

        v[0] = m_vertices[3 * m_triangles[i].inq_vindex(2) + 0] -
               m_vertices[3 * m_triangles[i].inq_vindex(0) + 0];
        v[1] = m_vertices[3 * m_triangles[i].inq_vindex(2) + 1] -
               m_vertices[3 * m_triangles[i].inq_vindex(0) + 1];
        v[2] = m_vertices[3 * m_triangles[i].inq_vindex(2) + 2] -
               m_vertices[3 * m_triangles[i].inq_vindex(0) + 2];
        
        cross_product(u, v, &m_facetnorms[3 * (i+1)]);
        normalize_vector(&m_facetnorms[3 * (i+1)]);
    }
}
//#############################################################################
// Generates smooth vertex normals for a model.
//#############################################################################

// _Node: general purpose node 
typedef struct _Node {
    GLuint         index;
    GLboolean      averaged;
    struct _Node* next;
} Node;

GLvoid Model::vertex_normals()
{
	Node*    node;
    Node*    tail;
    Node** members;
    GLfloat*    normals;
    GLuint  numnormals;
    GLfloat average[3];
    GLuint  i, avg;
    
    // delete any previous normals 
    if (m_normals)
        delete [] m_normals;
    
    // allocate space for new normals
	
	// 3 normals per triangle
    m_numnormals = m_numtriangles * 3;  
    m_normals = new GLfloat[3 * (m_numnormals+1)];
    
    // allocate a structure that will hold a 
	// linked list of triangle indices for each vertex 
    members = new Node*[m_numvertices + 1];
    for (i = 1; i <= m_numvertices; i++)
        members[i] = NULL;
    
    // for every triangle, create a node for each vertex in it 
    for (i = 0; i < m_numtriangles; i++) 
	{
        node = new Node;
        node->index = i;
        node->next  = members[m_triangles[i].inq_vindex(0)];
        members[m_triangles[i].inq_vindex(0)] = node;
        
        node = new Node;
        node->index = i;
        node->next  = members[m_triangles[i].inq_vindex(1)];
        members[m_triangles[i].inq_vindex(1)] = node;
        
        node = new Node;
        node->index = i;
        node->next  = members[m_triangles[i].inq_vindex(2)];
        members[m_triangles[i].inq_vindex(2)] = node;
    }
    
    // calculate the average normal for each vertex 
    numnormals = 1;
    for (i = 1; i <= m_numvertices; i++) 
	{
    // calculate an average normal for this vertex by averaging the
    // facet normal of every triangle this vertex is in 
        node = members[i];
        if (!node)
            fprintf(stderr, "glmVertexNormals(): vertex w/o a triangle\n");
        average[0] = 0.0; average[1] = 0.0; average[2] = 0.0;
        avg = 0;
        while (node) 
		{
			node->averaged = GL_TRUE;

			average[0] += m_facetnorms[3 * m_triangles[node->index].inq_findex() + 0];
			average[1] += m_facetnorms[3 * m_triangles[node->index].inq_findex() + 1];
			average[2] += m_facetnorms[3 * m_triangles[node->index].inq_findex() + 2];

			node = node->next;
		}
        
            // normalize the averaged normal 
            normalize_vector(average);
            
            // add the normal to the vertex normals list 
            m_normals[3 * numnormals + 0] = average[0];
            m_normals[3 * numnormals + 1] = average[1];
            m_normals[3 * numnormals + 2] = average[2];
            avg = numnormals;
            numnormals++;
        
        // set the normal of this vertex in each triangle it is in 
        node = members[i];
        while (node) {
            if (node->averaged) 
			{
				// if this node was averaged, use the average normal 
                if (m_triangles[node->index].inq_vindex(0) == i)
                    m_triangles[node->index].set_nindex(0,avg);
                else if (m_triangles[node->index].inq_vindex(1) == i)
                    m_triangles[node->index].set_nindex(1,avg);
                else if (m_triangles[node->index].inq_vindex(2) == i)
                    m_triangles[node->index].set_nindex(2,avg);
            } 
			else 
			{
				// if this node wasn't averaged, use the facet normal 
                m_normals[3 * numnormals + 0] = 
                    m_facetnorms[3 * m_triangles[node->index].inq_findex() + 0];
                m_normals[3 * numnormals + 1] = 
                    m_facetnorms[3 * m_triangles[node->index].inq_findex() + 1];
                m_normals[3 * numnormals + 2] = 
                    m_facetnorms[3 * m_triangles[node->index].inq_findex() + 2];

                if (m_triangles[node->index].inq_vindex(0) == i)
                    m_triangles[node->index].set_nindex(0,numnormals);
                else if (m_triangles[node->index].inq_vindex(1) == i)
                    m_triangles[node->index].set_nindex(1,numnormals);
                else if (m_triangles[node->index].inq_vindex(2) == i)
                    m_triangles[node->index].set_nindex(2,numnormals);

                numnormals++;
            }
            node = node->next;
        }
    }
    
    m_numnormals = numnormals - 1;
    
    // free the member information 
    for (i = 1; i <= m_numvertices; i++) 
	{
        node = members[i];
        while (node) 
		{
            tail = node;
            node = node->next;
            delete(tail);
        }
    }
    free(members);
    
    // pack the normals array (we previously allocated the maximum
    // number of normals that could possibly be created (numtriangles * 3), 
	// so get rid of some of them (usually alot unless none of the
    // facet normals were averaged))
    normals = m_normals;
    m_normals = new GLfloat[3* (m_numnormals+1)];
    for (i = 1; i <= m_numnormals; i++) {
        m_normals[3 * i + 0] = normals[3 * i + 0];
        m_normals[3 * i + 1] = normals[3 * i + 1];
        m_normals[3 * i + 2] = normals[3 * i + 2];
    }

    delete [] normals;
}

/*
extern void shaderAttachFromFile(GLuint, GLenum, const char *);

static GLuint g_program;
static GLuint g_programCameraPositionLocation;
static GLuint g_programLightPositionLocation;
static GLuint g_programLightColorLocation;
*/

//#############################################################################
//	shade a triangle in wire-frame mode (called from render method)
//#############################################################################			
GLvoid Model::wire_frame_mode(Triangle* triangle, Material* material)
{
	GLfloat c[4];
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0);

	c[0] = 0.0; c[1] = 0.0;
	c[2] = 1.0; c[3] = 1.0;
	glColor3fv(c);

	glBegin(GL_TRIANGLES);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();
}

//#############################################################################
//	shade a triangle in white (called from render method)
//#############################################################################	
GLvoid Model::white_mode(Triangle* triangle, Material* material) {
	GLfloat c[4] = {1.f,1.f,1.f,1.f};

	glShadeModel(GL_SMOOTH);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.f);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glColor3fv(c);

	glPolygonOffset(1.0f, 2.0f); // for a line drawing on second pass
	glEnable(GL_POLYGON_OFFSET_FILL);

	glBegin(GL_TRIANGLES);
	glNormal3fv(&m_normals[3 * triangle->inq_nindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(2)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();

	glDisable(GL_POLYGON_OFFSET_FILL);
}

//#############################################################################
//	shade a triangle in edge-buffer mode (feature lines) (called from render method)
//#############################################################################			
GLvoid Model::edge_buffer_mode(Triangle* triangle, Material* material)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

//	glPushAttrib(GL_ENABLE_BIT);
//	glLineStipple(3, 0x9999);
//	glEnable(GL_LINE_STIPPLE);

	//glEnable(GL_LINE_SMOOTH);

	glLineWidth(featureLineWidth);

	// flags used for drawing. If no B then front, if B then back (eg. boundE1 is front, boundE1B is back)
	//v1 to v2
	bool silE1 = false;
	bool boundE1 = false;
	bool cModelE1 = false;
	bool cUserE1 = false;
	bool slopeSE1 = false;
	bool boundE1B = false;
	bool cModelE1B = false;

	//v1 to v3
	bool silE2 = false;
	bool boundE2 = false;
	bool cModelE2 = false;
	bool cUserE2 = false;
	bool slopeSE2 = false;
	bool boundE2B = false;
	bool cModelE2B = false;

	//v2 to v3
	bool silE3 = false;
	bool boundE3 = false;
	bool cModelE3 = false;
	bool cUserE3 = false;
	bool slopeSE3 = false;
	bool boundE3B = false;
	bool cModelE3B = false;

	GLuint v1 = triangle->inq_vindex(0);
	GLuint v2 = triangle->inq_vindex(1);
	GLuint v3 = triangle->inq_vindex(2);

	// check edge buffer and update boolean values to be drawn
	if (v1 < v2) {
		for (int j = 0; j < maxAdjVertices; j++) {
			if (m_edges[v1][j].V == v2) {

				if ((m_edges[v1][j].F == true) && (m_edges[v1][j].B == true))
					silE1 = true;
				if ((m_edges[v1][j].F == true) && (m_edges[v1][j].B == false))
					boundE1 = true;
				else if ((m_edges[v1][j].F == false) && (m_edges[v1][j].B == true))
					boundE1B = true;
				else { // not a boundary so have 2 triangles. Check A flags.
					shapeMeasures(v1, j); // in case user changes them (and nessesary for view dependent flags such as slope steepness)
					if (m_edges[v1][j].A_CModel == true) {
						if (!m_edges[v1][j].Fa && m_edges[v1][j].Ba)
							cModelE1B = true;
						else
							cModelE1 = true;
					}
					if (m_edges[v1][j].A_CUser == true) {
						cUserE1 = true;
					}
					if (m_edges[v1][j].A_SlopeS == true) {
						slopeSE1 = true;
					}
				}
				break;
			}
		}
	} else {
		for (int j = 0; j < maxAdjVertices; j++) {
			if (m_edges[v2][j].V == v1) {

				if ((m_edges[v2][j].F == true) && (m_edges[v2][j].B == true))
					silE1 = true;
				if ((m_edges[v2][j].F == true) && (m_edges[v2][j].B == false))
					boundE1 = true;
				else if ((m_edges[v2][j].F == false) && (m_edges[v2][j].B == true))
					boundE1B = true;
				else { // not a boundary so have 2 triangles. Check A flags.
					shapeMeasures(v2, j); // in case user changes them (and nessesary for view dependent flags such as slope steepness)
					if (m_edges[v2][j].A_CModel == true) {
						if (!m_edges[v2][j].Fa && m_edges[v2][j].Ba)
							cModelE1B = true;
						else
							cModelE1 = true;
					}
					if (m_edges[v2][j].A_CUser == true) {
						cUserE1 = true;
					}
					if (m_edges[v2][j].A_SlopeS == true) {
						slopeSE1 = true;
					}
				}
				break;
			}
		}
	}

	if (v1 < v3) {
		for (int j = 0; j < maxAdjVertices; j++) {
			if (m_edges[v1][j].V == v3) {

				if ((m_edges[v1][j].F == true) && (m_edges[v1][j].B == true))
					silE2 = true;
				if ((m_edges[v1][j].F == true) && (m_edges[v1][j].B == false))
					boundE2 = true;
				else if ((m_edges[v1][j].F == false) && (m_edges[v1][j].B == true))
					boundE2B = true;
				else { // not a boundary so have 2 triangles. Check A flags.
					shapeMeasures(v1, j); // in case user changes them (and nessesary for view dependent flags such as slope steepness)
					if (m_edges[v1][j].A_CModel == true) {
						if (!m_edges[v1][j].Fa && m_edges[v1][j].Ba)
							cModelE2B = true;
						else
							cModelE2 = true;
					}
					if (m_edges[v1][j].A_CUser == true) {
						cUserE2 = true;
					}
					if (m_edges[v1][j].A_SlopeS == true) {
						slopeSE2 = true;
					}
				}
				break;
			}
		}
	} else {
		for (int j = 0; j < maxAdjVertices; j++) {
			if (m_edges[v3][j].V == v1) {

				if ((m_edges[v3][j].F == true) && (m_edges[v3][j].B == true))
					silE2 = true;
				if ((m_edges[v3][j].F == true) && (m_edges[v3][j].B == false))
					boundE2 = true;
				else if ((m_edges[v3][j].F == false) && (m_edges[v3][j].B == true))
					boundE2B = true;
				else { // not a boundary so have 2 triangles. Check A flags.
					shapeMeasures(v3, j); // in case user changes them (and nessesary for view dependent flags such as slope steepness)
					if (m_edges[v3][j].A_CModel == true) {
						if (!m_edges[v3][j].Fa && m_edges[v3][j].Ba)
							cModelE2B = true;
						else
							cModelE2 = true;
					}
					if (m_edges[v3][j].A_CUser == true) {
						cUserE2 = true;
					}
					if (m_edges[v3][j].A_SlopeS == true) {
						slopeSE2 = true;
					}
				}
				break;
			}
		}
	}

	if (v2 < v3) {
		for (int j = 0; j < maxAdjVertices; j++) {
			if (m_edges[v2][j].V == v3) {

				if ((m_edges[v2][j].F == true) && (m_edges[v2][j].B == true))
					silE3 = true;
				if ((m_edges[v2][j].F == true) && (m_edges[v2][j].B == false))
					boundE3 = true;
				else if ((m_edges[v2][j].F == false) && (m_edges[v2][j].B == true))
					boundE3B = true;
				else { // not a boundary so have 2 triangles. Check A flags.
					shapeMeasures(v2, j); // in case user changes them (and nessesary for view dependent flags such as slope steepness)
					if (m_edges[v2][j].A_CModel == true) {
						if (!m_edges[v2][j].Fa && m_edges[v2][j].Ba)
							cModelE3B = true;
						else
							cModelE3 = true;
					}
					if (m_edges[v2][j].A_CUser == true) {
						cUserE3 = true;
					}
					if (m_edges[v2][j].A_SlopeS == true) {
						slopeSE3 = true;
					}
				}
				break;
			}
		}
	} else {
		for (int j = 0; j < maxAdjVertices; j++) {
			if (m_edges[v3][j].V == v2) {

				if ((m_edges[v3][j].F == true) && (m_edges[v3][j].B == true))
					silE3 = true;
				if ((m_edges[v3][j].F == true) && (m_edges[v3][j].B == false))
					boundE3 = true;
				else if ((m_edges[v3][j].F == false) && (m_edges[v3][j].B == true))
					boundE3B = true;
				else { // not a boundary so have 2 triangles. Check A flags.
					shapeMeasures(v3, j); // in case user changes them (and nessesary for view dependent flags such as slope steepness)
					if (m_edges[v3][j].A_CModel == true) {
						if (!m_edges[v3][j].Fa && m_edges[v3][j].Ba)
							cModelE3B = true;
						else
							cModelE3 = true;
					}
					if (m_edges[v3][j].A_CUser == true) {
						cUserE3 = true;
					}
					if (m_edges[v3][j].A_SlopeS == true) {
						slopeSE3 = true;
					}
				}
				break;
			}
		}
	}

	GLfloat ver1[3] = {m_vertices[3 * v1], m_vertices[3 * v1 + 1], m_vertices[3 * v1 + 2]};
	GLfloat ver2[3] = {m_vertices[3 * v2], m_vertices[3 * v2 + 1], m_vertices[3 * v2 + 2]};
	GLfloat ver3[3] = {m_vertices[3 * v3], m_vertices[3 * v3 + 1], m_vertices[3 * v3 + 2]};


	// draw in order: silhouette, boundary, creaseModel, creaseUser, slopeSteepness
	if (silE1 && silhouette) {
		glColor3fv(silhouetteColor);
		if (silhouetteStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (silhouetteStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver2);
		glEnd();
	} else if ((boundE1 || boundE1B) && boundary) {
		glColor3fv(boundaryColor);
		if ((boundE1B && (boundaryBackStyle == 2)) || (boundE1 && (boundaryFrontStyle == 2))) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if ((boundE1B && (boundaryBackStyle == 1)) || (boundE1 && (boundaryFrontStyle == 1))) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver2);
		glEnd();
	} else if ((cModelE1 || cModelE1B) && creaseModel) {
		glColor3fv(creaseModelColor);
		if ((cModelE1B && (creaseModelBackStyle == 2)) || (cModelE1 && (creaseModelFrontStyle == 2))) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if ((cModelE1B && (creaseModelBackStyle == 1)) || (cModelE1 && (creaseModelFrontStyle == 1))) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver2);
		glEnd();
	} else if ((cUserE1) && creaseUser) {
		glColor3fv(creaseUserColor);
		if (creaseUserStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (creaseUserStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver2);
		glEnd();
	} else if ((slopeSE1) && slopeSteepness) {
		glColor3fv(slopeSteepnessColor);
		if (slopeSteepnessStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (slopeSteepnessStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver2);
		glEnd();
	}
	
	glDisable(GL_LINE_STIPPLE);
	if (silE2 && silhouette) {
		glColor3fv(silhouetteColor);
		if (silhouetteStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (silhouetteStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver3);
		glEnd();
	} else if ((boundE2 || boundE2B) && boundary) {
		glColor3fv(boundaryColor);
		if ((boundE2B && (boundaryBackStyle == 2)) || (boundE2 && (boundaryFrontStyle == 2))) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if ((boundE2B && (boundaryBackStyle == 1)) || (boundE2 && (boundaryFrontStyle == 1))) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver3);
		glEnd();
	} else if ((cModelE2 || cModelE2B) && creaseModel) {
		glColor3fv(creaseModelColor);
		if ((cModelE2B && (creaseModelBackStyle == 2)) || (cModelE2 && (creaseModelFrontStyle == 2))) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if ((cModelE2B && (creaseModelBackStyle == 1)) || (cModelE2 && (creaseModelFrontStyle == 1))) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver3);
		glEnd();
	} else if ((cUserE2) && creaseUser) {
		glColor3fv(creaseUserColor);
		if (creaseUserStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (creaseUserStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver3);
		glEnd();
	} else if ((slopeSE2) && slopeSteepness) {
		glColor3fv(slopeSteepnessColor);
		if (slopeSteepnessStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (slopeSteepnessStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver1);
		glVertex3fv(ver3);
		glEnd();
	}

	glDisable(GL_LINE_STIPPLE);
	if (silE3 && silhouette) {
		glColor3fv(silhouetteColor);
		if (silhouetteStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (silhouetteStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver2);
		glVertex3fv(ver3);
		glEnd();
	} else if ((boundE3 || boundE3B) && boundary) {
		glColor3fv(boundaryColor);
		if ((boundE3B && (boundaryBackStyle == 2)) || (boundE3 && (boundaryFrontStyle == 2))) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if ((boundE3B && (boundaryBackStyle == 1)) || (boundE3 && (boundaryFrontStyle == 1))) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver2);
		glVertex3fv(ver3);
		glEnd();
	} else if ((cModelE3 || cModelE3B) && creaseModel) {
		glColor3fv(creaseModelColor);
		if ((cModelE3B && (creaseModelBackStyle == 2)) || (cModelE3 && (creaseModelFrontStyle == 2))) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if ((cModelE3B && (creaseModelBackStyle == 1)) || (cModelE3 && (creaseModelFrontStyle == 1))) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver2);
		glVertex3fv(ver3);
		glEnd();
	} else if ((cUserE3) && creaseUser) {
		glColor3fv(creaseUserColor);
		if (creaseUserStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (creaseUserStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver2);
		glVertex3fv(ver3);
		glEnd();
	} else if ((slopeSE3) && slopeSteepness) {
		glColor3fv(slopeSteepnessColor);
		if (slopeSteepnessStyle == 2) {
			glLineStipple(3, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		else if (slopeSteepnessStyle == 1) {
			glLineStipple(1, 0x9999);
			glEnable(GL_LINE_STIPPLE);
		}
		glBegin(GL_LINES);
		glVertex3fv(ver2);
		glVertex3fv(ver3);
		glEnd();
	}
}

//#############################################################################
//	shade a triangle in wire-frame mode (called from render method)
//#############################################################################			
GLvoid Model::culling_mode(Triangle* triangle, Material* material)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_LIGHTING);
	glEnable(GL_CULL_FACE);

	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0);

	GLfloat c[4];
	c[0] = 0.0; c[1] = 0.0; c[2] = 1.0; c[3] = 0.0;
	glColor3fv(c);
	
	glBegin(GL_TRIANGLES);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();
}


//#############################################################################
//	shade a triangle in hidden surface removal mode (called from render method)
//#############################################################################			
GLvoid Model::hsr_mode(Triangle* triangle, Material* material)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	glDisable(GL_LIGHTING);
	glEnable(GL_CULL_FACE);
	
	GLfloat c[4];
	c[0] = 1.0; c[1] = 1.0; c[2] = 1.0; c[3] = 0.0;
	glColor3fv(c);
	
	glBegin(GL_TRIANGLES);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();

	wire_frame_mode(triangle, material);
}

//#############################################################################
//	shade a triangle in flat shading mode (called from render method)
//#############################################################################			
GLvoid Model::flat_shading_mode(Triangle* triangle, Material* material)
{
	glShadeModel(GL_FLAT);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material->inq_ambient());
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->inq_diffuse());
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->inq_specular());
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->inq_shininess());
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glColor3fv(material->inq_diffuse());
	glNormal3fv(&m_facetnorms[3 * triangle->inq_findex()]);

	glBegin(GL_TRIANGLES);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();
}

//#############################################################################
//	shade a triangle in smooth shading mode (called from render method)
//#############################################################################			
GLvoid Model::smooth_shading_mode(Triangle* triangle, Material* material)
{
	glShadeModel(GL_SMOOTH);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material->inq_ambient());
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->inq_diffuse());
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->inq_specular());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->inq_shininess());

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glColor3fv(material->inq_diffuse());

	glPolygonOffset(1.0f, 2.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glBegin(GL_TRIANGLES);
	glNormal3fv(&m_normals[3 * triangle->inq_nindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(2)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();

	glDisable(GL_POLYGON_OFFSET_FILL);
}

//#############################################################################
//	shade a triangle in texture mode (called from render method)
//#############################################################################			
GLvoid Model::texture_mode(Triangle* triangle, Material* material)
{
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_texid);

	// start shader
	glUseProgram(g_texprogram);

	// get texture-unit variable from shader declared as uniform 
	GLint texUnitLoc = glGetUniformLocation(g_texprogram, "myTextureSampler"); 

	GLint mode = glGetUniformLocation(g_texprogram, "detailMode");
	GLint rOrienti = glGetUniformLocation(g_texprogram, "rOrient");
	GLint rDepthi = glGetUniformLocation(g_texprogram, "rDepth");
	GLint zminDepthi = glGetUniformLocation(g_texprogram, "zminDepth");
 
	// activate unit 0 
	glActiveTexture(GL_TEXTURE0);

	// setup texture to go through unit 0 
	glUniform1i(texUnitLoc, 0);

	// pass parameters
	glUniform1i(mode, toneDetail);
	glUniform1f(rOrienti, rOrient);
	glUniform1f(rDepthi, rDepth);
	glUniform1f(zminDepthi, zminDepth);

	glPolygonOffset(1.0f, 2.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	glBegin(GL_TRIANGLES);
	glNormal3fv(&m_normals[3 * triangle->inq_nindex(0)]);
	glTexCoord2fv(&m_texcoords[2 * triangle->inq_tindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(1)]);
	glTexCoord2fv(&m_texcoords[2 * triangle->inq_tindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(2)]);
	glTexCoord2fv(&m_texcoords[2 * triangle->inq_tindex(2)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();

	glDisable(GL_POLYGON_OFFSET_FILL);

//	glDeleteTextures(1, &m_texid);

	// end shader
	glUseProgram(0);

}


//#############################################################################
//	shade a triangle in gooch shading mode (called from render method)
//#############################################################################			
GLvoid Model::gooch_mode(Triangle* triangle, Material* material)
{
	// start with smooth shading
	glShadeModel(GL_SMOOTH);
   /* glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material->inq_ambient());
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->inq_diffuse());
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->inq_specular());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->inq_shininess());*/

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	//glColor3fv(material->inq_diffuse());

	// start shader
	glUseProgram(g_goochprogram);

	locAmbient	= glGetUniformLocation(g_goochprogram, "ambientColor");
	locDiffuse	= glGetUniformLocation(g_goochprogram, "diffuseColor"); // only need diff for gooch
	locSpecular = glGetUniformLocation(g_goochprogram, "specularColor");
	goochKCoolColor = glGetUniformLocation(g_goochprogram, "kblue");
	goochKWarmColor =  glGetUniformLocation(g_goochprogram, "kyellow");
	goochAlpha = glGetUniformLocation(g_goochprogram, "alpha");
	goochBeta = glGetUniformLocation(g_goochprogram, "beta");

	GLfloat c[4] = {1.f,1.f,1.f,1.f};

	// pass parameters
	glUniform4fv(locAmbient, 1, c);
	glUniform4fv(locDiffuse, 1, c);
	glUniform4fv(locSpecular, 1, c);
	glUniform4fv(goochKCoolColor, 1, kblue);
	glUniform4fv(goochKWarmColor, 1, kyellow);
	glUniform1f(goochAlpha, alpha);
	glUniform1f(goochBeta, beta);

	glPolygonOffset(1.0f, 2.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	// render
	glBegin(GL_TRIANGLES);
	glNormal3fv(&m_normals[3 * triangle->inq_nindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);

	glNormal3fv(&m_normals[3 * triangle->inq_nindex(2)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();

	glDisable(GL_POLYGON_OFFSET_FILL);

	// end shader
	glUseProgram(0);

	// draw outline
/*	GLfloat c[4];
	
	glPolygonMode(GL_BACK, GL_LINE);
	glDisable(GL_LIGHTING);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);

	glEnable(GL_LINE_SMOOTH);
	glLineWidth(3.0);

	c[0] = 0.0; c[1] = 0.0; c[2] = 0.0; c[3] = 1.0;
	glColor3fv(c);

	glBegin(GL_TRIANGLES);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(0)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(1)]);
	glVertex3fv(&m_vertices[3 * triangle->inq_vindex(2)]);	
	glEnd();
*/
	
}

//#############################################################################
//	render a model in specific modes:
//  1 - wire-frame
//  2 - back-faces removed (culling)
//  3 - hidden surfaces removed
//  4 - flat shading
//  5 - smooth shading
//#############################################################################		
GLvoid Model::render(GLuint mode)
{
    static GLuint i;
    static Group* group;
    static Triangle* triangle;
	static Material* material;

    group = m_groups;
    while (group) 
	{
		for (i = 0; i < group->numtriangles; i++) 
		{
			GLuint index = group->triangles[i];
			Triangle* triangle = &m_triangles[index];
			material = &m_materials[group->material];

			switch (mode)
			{
			case WIRE_FRAME:
				wire_frame_mode(triangle, material);
				break;
			case CULLING:
				culling_mode(triangle, material);
				break;
			case HSR:
				hsr_mode(triangle, material);
				break;
			case FLAT:
				flat_shading_mode(triangle, material);
				break;
			case SMOOTH:
				smooth_shading_mode(triangle, material);
				break;
			case TEXTURE:
				texture_mode(triangle, material);
				break;

			case GOOCH:
				gooch_mode(triangle, material);
				break;
			case EDGEB:
				break;
			case WHITE:
				white_mode(triangle, material);
				break;

			default: break;
			}

			if (featureLines)
				edge_buffer_mode(triangle, material);
		}
		group = group->next;
	}
}

//#############################################################################
//	End
//#############################################################################

