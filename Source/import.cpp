//#############################################################################
//	File:		import.cpp
//	Module:		CPSC 453 - assignment #3 (rendering)
// 	Description: Model methods for importing Alias/Wavefront .obj scene files	
//
//#############################################################################

//#############################################################################
//	Headers
//#############################################################################

#include "Model.h"


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

static GLvoid midpoint(GLfloat* u, GLfloat* v, GLfloat* n)
{
    n[0] = (u[0] + v[0]) / 2;
    n[1] = (u[1] + v[1]) / 2;
    n[2] = (u[2] + v[2]) / 2;
}

static GLfloat dot_product(GLfloat* u, GLfloat* v)
{
	return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

static GLfloat distance(GLfloat* u, GLfloat* v)
{
	return (GLfloat)sqrt((v[0]-u[0])*(v[0]-u[0]) + (v[1]-u[1])*(v[1]-u[1]) + (v[2]-u[2])*(v[2]-u[2]));
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
// Find a group in the model
//#############################################################################

Group* Model::find_group(char* name)
{
    Group* group;
    
    group = m_groups;
    while(group) {
        if (!strcmp(name, group->name))
            return group;
        group = group->next;
    }
    return NULL;  
}

//#############################################################################
// Add a group to the model
//#############################################################################

Group* Model::add_group(char* name)
{
    Group* group;
    
    group = find_group(name);
    if (!group) {
        group = new Group;
        group->name = strdup(name);
        group->material = 0;
        group->numtriangles = 0;
        group->triangles = NULL;
        group->next = m_groups;
        m_groups = group;
        m_numgroups++;
    }
    
    return group;
}


//#############################################################################
// Find a material in the model
//#############################################################################

GLuint Model::find_material(char* name)
{
    GLuint i;

    for (i = 0; i < m_nummaterials; i++) {
        if (!strcmp(m_materials[i].inq_name(), name))
            return i;
    }
    
    printf("glmFindMaterial():  can't find material \"%s\".\n", name);
    return -1;
}

//#############################################################################
// read a wavefront material library file
//#############################################################################

// return the directory given a path
static char* dir_name(char* path)
{
    char* dir;
    char* s;
    
    dir = strdup(path);
    
    s = strrchr(dir, '/');
    if (s)
        s[1] = '\0';
    else
        dir[0] = '\0';
    
    return dir;
}


GLvoid Model::read_MTL(char* name)
{
    FILE* file;
    char* dir;
    char* filename;
    char    buf[128];
    GLuint nummaterials;
    
    dir = dir_name(m_pathname);
    filename = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(name) + 1));
    strcpy(filename, dir);
    strcat(filename, name);
    free(dir);
    
    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "read_MTL() failed: can't open material file \"%s\".\n",
            filename);
        exit(1);
    }
    free(filename);
    
    // count the number of materials in the file //
    nummaterials = 1;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               // comment //
            // eat up rest of line //
            fgets(buf, sizeof(buf), file);
            break;
        case 'n':               // newmtl //
            fgets(buf, sizeof(buf), file);
            nummaterials++;
            sscanf(buf, "%s %s", buf, buf);
            break;
        default:
            // eat up rest of line //
            fgets(buf, sizeof(buf), file);
            break;
        }
    }
    
    rewind(file);
    
    m_materials = new Material[nummaterials];
    m_nummaterials = nummaterials;
    
    m_materials[0].set_name(strdup("default"));
    
    // now, read in the data //
    nummaterials = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               // comment //
            // eat up rest of line //
            fgets(buf, sizeof(buf), file);
            break;
        case 'n':               // newmtl //
            fgets(buf, sizeof(buf), file);
            sscanf(buf, "%s %s", buf, buf);
            nummaterials++;
            m_materials[nummaterials].set_name(strdup(buf));
            break;
        case 'N':
			GLfloat shininess;
            fscanf(file, "%f", &shininess);
            // wavefront shininess is from [0, 1000], so scale for OpenGL //
            shininess /= 1000.0;
            shininess *= 128.0;
			m_materials[nummaterials].set_shininess(shininess);
            break;
        case 'K':
			GLfloat r, g, b;
            switch(buf[1]) {
            case 'd':
                fscanf(file, "%f %f %f", &r, &g, &b);
                m_materials[nummaterials].set_diffuse(r, g, b);
                break;
            case 's':
                fscanf(file, "%f %f %f", &r, &g, &b);
                m_materials[nummaterials].set_specular(r, g, b);
                break;
            case 'a':
                fscanf(file, "%f %f %f", &r, &g, &b);
                m_materials[nummaterials].set_ambient(r, g, b);
                break;
            default:
                // eat up rest of line //
                fgets(buf, sizeof(buf), file);
                break;
            }
            break;
            default:
                // eat up rest of line //
                fgets(buf, sizeof(buf), file);
                break;
        }
    }

    fclose(file);
}

//#############################################################################
// first pass at a Wavefront OBJ file that gets all the
// statistics of the model (such as #vertices, #normals, etc)
//#############################################################################
GLvoid Model::first_pass(FILE* file) 
{
    GLuint  numvertices;        // number of vertices in model //
    GLuint  numnormals;         // number of normals in model //
    GLuint  numtexcoords;       // number of texcoords in model //
    GLuint  numtriangles;       // number of triangles in model //
    Group* group;               // current group //
    unsigned    v, n, t;
    char        buf[128];
    
    // make a default group //
    group = add_group("default");
    
    numvertices = numnormals = numtexcoords = numtriangles = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {

        case '#':               // comment //
            // eat up rest of line //
            fgets(buf, sizeof(buf), file);
            break;

        case 'v':               // v, vn, vt //
            switch(buf[1]) {

            case '\0':          // vertex //
                // eat up rest of line //
                fgets(buf, sizeof(buf), file);
                numvertices++;
                break;

            case 'n':           // normal //
                // eat up rest of line //
                fgets(buf, sizeof(buf), file);
                numnormals++;
                break;

            case 't':           // texcoord //
                // eat up rest of line 
                fgets(buf, sizeof(buf), file);
                numtexcoords++;
                break;

            default:
                printf("glmFirstPass(): Unknown token \"%s\".\n", buf);
                exit(1);
                break;
            }
            break;

            case 'm':
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                m_mtllibname = strdup(buf);
                read_MTL(buf);
                break;

            case 'u':
                // eat up rest of line 
                fgets(buf, sizeof(buf), file);
                break;

			case 'g':               // group 
                // eat up rest of line 
                fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
                sscanf(buf, "%s", buf);
#else
                buf[strlen(buf)-1] = '\0';  // nuke '\n' 
#endif
                group = add_group(buf);
                break;

            case 'f':               // face 
                v = n = t = 0;
                fscanf(file, "%s", buf);
                // can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d 
                if (strstr(buf, "//")) {
                    // v//n //
                    sscanf(buf, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
                    // v/t/n //
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    // v/t //
                    fscanf(file, "%d/%d", &v, &t);
                    fscanf(file, "%d/%d", &v, &t);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d", &v, &t) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else {
                    // v //
                    fscanf(file, "%d", &v);
                    fscanf(file, "%d", &v);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d", &v) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                }
                break;
                
            default:
                // eat up rest of line //
                fgets(buf, sizeof(buf), file);
                break;
        }
  }
  // set the stats in the model structure //
  m_numvertices  = numvertices;
  m_numnormals   = numnormals;
  m_numtexcoords = numtexcoords;
  m_numtriangles = numtriangles;
  
  // allocate memory for the triangles in each group //
  group = m_groups;
  while(group) {
      group->triangles = new GLuint[group->numtriangles];
      group->numtriangles = 0;
      group = group->next;
  }
}


//#############################################################################
// second pass at a Wavefront OBJ file that gets all the data.
//#############################################################################

#define T(x) (m_triangles[(x)])

GLvoid Model::second_pass(FILE* file) 
{
    GLuint  numvertices;        /* number of vertices in model */
    GLuint  numnormals;         /* number of normals in model */
    GLuint  numtexcoords;       /* number of texcoords in model */
    GLuint  numtriangles;       /* number of triangles in model */
    GLfloat*    vertices;           /* array of vertices  */
    GLfloat*    normals;            /* array of normals */
    GLfloat*    texcoords;          /* array of texture coordinates */
    Group* group;            /* current group pointer */
    GLuint  material;           /* current material */
    GLuint  v, n, t;
    char        buf[128];
    
    /* set the pointer shortcuts */
    vertices       = m_vertices;
    normals    = m_normals;
    texcoords    = m_texcoords;
    group      = m_groups;
    
    /* on the second pass through the file, read all the data into the
    allocated arrays */
    numvertices = numnormals = numtexcoords = 1;
    numtriangles = 0;
    material = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               /* v, vn, vt */
            switch(buf[1]) {
            case '\0':          /* vertex */
                fscanf(file, "%f %f %f", 
                    &vertices[3 * numvertices + 0], 
                    &vertices[3 * numvertices + 1], 
                    &vertices[3 * numvertices + 2]);
                numvertices++;
                break;
            case 'n':           /* normal */
                fscanf(file, "%f %f %f", 
                    &normals[3 * numnormals + 0],
                    &normals[3 * numnormals + 1], 
                    &normals[3 * numnormals + 2]);
                numnormals++;
                break;
            case 't':           /* texcoord */
                fscanf(file, "%f %f", 
                    &texcoords[2 * numtexcoords + 0],
                    &texcoords[2 * numtexcoords + 1]);
                numtexcoords++;
                break;
            }
            break;
            case 'u':
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                group->material = material = find_material(buf);
                break;
            case 'g':               /* group */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
                sscanf(buf, "%s", buf);
#else
                buf[strlen(buf)-1] = '\0';  /* nuke '\n' */
#endif
                group = find_group(buf);
                group->material = material;
                break;
            case 'f':               /* face */
                v = n = t = 0;
                fscanf(file, "%s", buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
                if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%d//%d", &v, &n);
                    m_triangles[numtriangles].set_vindex(0,v);
                    m_triangles[numtriangles].set_nindex(0,n);
                    fscanf(file, "%d//%d", &v, &n);
                    m_triangles[numtriangles].set_vindex(1,v);
                    m_triangles[numtriangles].set_nindex(1,n);
                    fscanf(file, "%d//%d", &v, &n);
                    m_triangles[numtriangles].set_vindex(2,v);
                    m_triangles[numtriangles].set_nindex(2,n);
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
                        m_triangles[numtriangles].set_vindex(0,m_triangles[numtriangles-1].inq_vindex(0));
                        m_triangles[numtriangles].set_nindex(0,m_triangles[numtriangles-1].inq_nindex(0));
                        m_triangles[numtriangles].set_vindex(1,m_triangles[numtriangles-1].inq_vindex(2));
                        m_triangles[numtriangles].set_nindex(1,m_triangles[numtriangles-1].inq_nindex(2));
                        m_triangles[numtriangles].set_vindex(2,v);
                        m_triangles[numtriangles].set_nindex(2,n);
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
                    /* v/t/n */
                    m_triangles[numtriangles].set_vindex(0,v);
                    m_triangles[numtriangles].set_tindex(0,t);
                    m_triangles[numtriangles].set_nindex(0,n);
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    m_triangles[numtriangles].set_vindex(1,v);
                    m_triangles[numtriangles].set_tindex(1,t);
                    m_triangles[numtriangles].set_nindex(1,n);
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    m_triangles[numtriangles].set_vindex(2,v);
                    m_triangles[numtriangles].set_tindex(2,t);
                    m_triangles[numtriangles].set_nindex(2,n);
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
                        m_triangles[numtriangles].set_vindex(0,m_triangles[numtriangles-1].inq_vindex(0));
                        m_triangles[numtriangles].set_tindex(0,m_triangles[numtriangles-1].inq_tindex(0));
                        m_triangles[numtriangles].set_nindex(0,m_triangles[numtriangles-1].inq_nindex(0));
                        m_triangles[numtriangles].set_vindex(1,m_triangles[numtriangles-1].inq_vindex(2));
                        m_triangles[numtriangles].set_tindex(1,m_triangles[numtriangles-1].inq_tindex(2));
                        m_triangles[numtriangles].set_nindex(1,m_triangles[numtriangles-1].inq_nindex(2));
                        m_triangles[numtriangles].set_vindex(2,v);
                        m_triangles[numtriangles].set_tindex(2,t);
                        m_triangles[numtriangles].set_nindex(2,n);
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    /* v/t */
                    m_triangles[numtriangles].set_vindex(0,v);
                    m_triangles[numtriangles].set_tindex(0,t);
                    fscanf(file, "%d/%d", &v, &t);
                    m_triangles[numtriangles].set_vindex(1,v);
                    m_triangles[numtriangles].set_tindex(1,t);
                    fscanf(file, "%d/%d", &v, &t);
                    m_triangles[numtriangles].set_vindex(2,v);
                    m_triangles[numtriangles].set_tindex(2,t);
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d/%d", &v, &t) > 0) {
                        m_triangles[numtriangles].set_vindex(0,m_triangles[numtriangles-1].inq_vindex(0));
                        m_triangles[numtriangles].set_tindex(0,m_triangles[numtriangles-1].inq_tindex(0));
                        m_triangles[numtriangles].set_vindex(1,m_triangles[numtriangles-1].inq_vindex(2));
                        m_triangles[numtriangles].set_tindex(1,m_triangles[numtriangles-1].inq_tindex(2));
                        m_triangles[numtriangles].set_vindex(2,v);
                        m_triangles[numtriangles].set_tindex(2,t);
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else {
                    /* v */
                    sscanf(buf, "%d", &v);
                    m_triangles[numtriangles].set_vindex(0,v);
                    fscanf(file, "%d", &v);
                    m_triangles[numtriangles].set_vindex(1,v);
                    fscanf(file, "%d", &v);
                    m_triangles[numtriangles].set_vindex(2,v);
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d", &v) > 0) {
                        m_triangles[numtriangles].set_vindex(0,m_triangles[numtriangles-1].inq_vindex(0));
                        m_triangles[numtriangles].set_vindex(1,m_triangles[numtriangles-1].inq_vindex(2));
                        m_triangles[numtriangles].set_vindex(2,v);
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                }
                break;
                
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
    }
  }
  
#if 0
  /* announce the memory requirements */
  printf(" Memory: %d bytes\n",
      numvertices  * 3*sizeof(GLfloat) +
      numnormals   * 3*sizeof(GLfloat) * (numnormals ? 1 : 0) +
      numtexcoords * 3*sizeof(GLfloat) * (numtexcoords ? 1 : 0) +
      numtriangles * sizeof(GLMtriangle));
#endif
}

// Calculate the morphometric variables and set A flags appropriately.  Specifically sets the slope steepness A flag (called from shapeMeasures)
// (can add calculations for more flags such as slope aspect here as well).
GLvoid Model::setMVFlags(GLuint i, GLuint j) {
	// set up DEM as described in paper

	GLuint v1T1i = m_triangles[m_edges[i][j].T1].inq_vindex(0);
	GLuint v2T1i = m_triangles[m_edges[i][j].T1].inq_vindex(1);
	GLuint v3T1i = m_triangles[m_edges[i][j].T1].inq_vindex(2);

	GLuint v1T2i = m_triangles[m_edges[i][j].T2].inq_vindex(0);
	GLuint v2T2i = m_triangles[m_edges[i][j].T2].inq_vindex(1);
	GLuint v3T2i = m_triangles[m_edges[i][j].T2].inq_vindex(2);

	GLuint point_ai = i;
	GLuint point_bi = m_edges[i][j].V;
	GLuint point_5i;
	GLuint point_6i;

	if (v1T1i != point_ai && v1T1i != point_bi)
		point_5i = v1T1i;
	else if (v2T1i != point_ai && v2T1i != point_bi)
		point_5i = v2T1i;
	else
		point_5i = v3T1i;

	if (v1T2i != point_ai && v1T2i != point_bi)
		point_6i = v1T2i;
	else if (v2T2i != point_ai && v2T2i != point_bi)
		point_6i = v2T2i;
	else
		point_6i = v3T2i;

	GLfloat point_a[3] = {m_vertices[3 * point_ai], m_vertices[3 * point_ai + 1], m_vertices[3 * point_ai + 2]};
	GLfloat point_b[3] = {m_vertices[3 * point_bi], m_vertices[3 * point_bi + 1], m_vertices[3 * point_bi + 2]};
	GLfloat point_5[3] = {m_vertices[3 * point_5i], m_vertices[3 * point_5i + 1], m_vertices[3 * point_5i + 2]};
	GLfloat point_6[3] = {m_vertices[3 * point_6i], m_vertices[3 * point_6i + 1], m_vertices[3 * point_6i + 2]};

	// Rotation (simulates a different z axis for the elevation)
	GLfloat mvMat[4][4];
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)mvMat);
	glPushMatrix();
	glLoadIdentity();
	glRotatef(slopeSteepnessRotateX, 1.f, 0.f, 0.f);
	glRotatef(slopeSteepnessRotateY, 0.f, 1.f, 0.f);
	glMultMatrixf((GLfloat*)mvMat);

	// points in camera space
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)mvMat);
	GLfloat point_aC[3] = {mvMat[0][0]*point_a[0]+mvMat[1][0]*point_a[1]+mvMat[2][0]*point_a[2]+mvMat[3][0], mvMat[0][1]*point_a[0]+mvMat[1][1]*point_a[1]+mvMat[2][1]*point_a[2]+mvMat[3][1], mvMat[0][2]*point_a[0]+mvMat[1][2]*point_a[1]+mvMat[2][2]*point_a[2]+mvMat[3][2]};
	GLfloat point_bC[3] = {mvMat[0][0]*point_b[0]+mvMat[1][0]*point_b[1]+mvMat[2][0]*point_b[2]+mvMat[3][0], mvMat[0][1]*point_b[0]+mvMat[1][1]*point_b[1]+mvMat[2][1]*point_b[2]+mvMat[3][1], mvMat[0][2]*point_b[0]+mvMat[1][2]*point_b[1]+mvMat[2][2]*point_b[2]+mvMat[3][2]};
	GLfloat point_5C[3] = {mvMat[0][0]*point_5[0]+mvMat[1][0]*point_5[1]+mvMat[2][0]*point_5[2]+mvMat[3][0], mvMat[0][1]*point_5[0]+mvMat[1][1]*point_5[1]+mvMat[2][1]*point_5[2]+mvMat[3][1], mvMat[0][2]*point_5[0]+mvMat[1][2]*point_5[1]+mvMat[2][2]*point_5[2]+mvMat[3][2]};
	GLfloat point_6C[3] = {mvMat[0][0]*point_6[0]+mvMat[1][0]*point_6[1]+mvMat[2][0]*point_6[2]+mvMat[3][0], mvMat[0][1]*point_6[0]+mvMat[1][1]*point_6[1]+mvMat[2][1]*point_6[2]+mvMat[3][1], mvMat[0][2]*point_6[0]+mvMat[1][2]*point_6[1]+mvMat[2][2]*point_6[2]+mvMat[3][2]};

	glPopMatrix();

	GLfloat point_0[3];
	GLfloat point_1[3];
	GLfloat point_2[3];
	GLfloat point_3[3];
	GLfloat point_4[3];

	// object space for debugging
	/*midpoint(point_a, point_b, point_0);
	midpoint(point_6, point_a, point_1);
	midpoint(point_a, point_5, point_2);
	midpoint(point_5, point_b, point_3);
	midpoint(point_b, point_6, point_4);*/

	midpoint(point_aC, point_bC, point_0);
	midpoint(point_6C, point_aC, point_1);
	midpoint(point_aC, point_5C, point_2);
	midpoint(point_5C, point_bC, point_3);
	midpoint(point_bC, point_6C, point_4);

	GLfloat h1 = distance(point_0, point_1);
	GLfloat h2 = distance(point_0, point_2);
	GLfloat h3 = distance(point_0, point_3);
	GLfloat h4 = distance(point_0, point_4);

	// calculate MVs
	GLfloat f_0x = (point_1[2]-point_3[2]) / (h1+h3);
	GLfloat f_0y = (point_2[2]-point_4[2]) / (h2+h4);

	// slope steepness
	GLfloat GA = sqrt(atan(f_0x*f_0x + f_0y*f_0y));
	GLfloat angle = (GA * 180.0f) / 3.1415926;

	if (slopeSteepnessMin <= angle && angle <= slopeSteepnessMax)
		m_edges[i][j].A_SlopeS = true;
	else
		m_edges[i][j].A_SlopeS = false;

}

// Edge-based shape measures to set A flags. Set the crease flags and slope steepness flag (A_CUser, A_CModel, and A_SlopeS) in edge buffer (m_edges) for an edge (index i,j)
GLvoid Model::shapeMeasures(GLuint i, GLuint j) {
	// compute normal of T1
	GLuint v1i = m_triangles[m_edges[i][j].T1].inq_vindex(0);
	GLuint v2i = m_triangles[m_edges[i][j].T1].inq_vindex(1);
	GLuint v3i = m_triangles[m_edges[i][j].T1].inq_vindex(2);

	GLfloat v1[3] = {m_vertices[3 * v1i], m_vertices[3 * v1i + 1], m_vertices[3 * v1i + 2]};
	GLfloat v2[3] = {m_vertices[3 * v2i], m_vertices[3 * v2i + 1], m_vertices[3 * v2i + 2]};
	GLfloat v3[3] = {m_vertices[3 * v3i], m_vertices[3 * v3i + 1], m_vertices[3 * v3i + 2]};

	GLfloat vec1[3] = {v1[0]-v2[0], v1[1]-v2[1], v1[2]-v2[2]};
	GLfloat vec2[3] = {v1[0]-v3[0], v1[1]-v3[1], v1[2]-v3[2]};
	GLfloat t1normal[3];
	cross_product(vec1, vec2, t1normal);
	normalize_vector(t1normal);

	// compute normal of T2
	v1i = m_triangles[m_edges[i][j].T2].inq_vindex(0);
	v2i = m_triangles[m_edges[i][j].T2].inq_vindex(1);
	v3i = m_triangles[m_edges[i][j].T2].inq_vindex(2);

	v1[0] = m_vertices[3 * v1i]; v1[1] = m_vertices[3 * v1i + 1]; v1[2] = m_vertices[3 * v1i + 2];
	v2[0] = m_vertices[3 * v2i]; v2[1] = m_vertices[3 * v2i + 1]; v2[2] = m_vertices[3 * v2i + 2];
	v3[0] = m_vertices[3 * v3i]; v3[1] = m_vertices[3 * v3i + 1]; v3[2] = m_vertices[3 * v3i + 2];

	vec1[0] = v1[0]-v2[0]; vec1[1] = v1[1]-v2[1]; vec1[2] = v1[2]-v2[2];
	vec2[0] = v1[0]-v3[0]; vec2[1] = v1[1]-v3[1]; vec2[2] = v1[2]-v3[2];
	GLfloat t2normal[3];
	cross_product(vec1, vec2, t2normal);
	normalize_vector(t2normal);

	// get angle between two triangle normals and set crease flags appropriately
	GLfloat n1DotN2 = dot_product(t1normal, t2normal);
	GLfloat angle = (acos(n1DotN2) * 180.0f) / 3.1415926;

	if (creaseModelMin <= angle && angle <= creaseModelMax)
		m_edges[i][j].A_CModel = true;
	else
		m_edges[i][j].A_CModel = false;

	if (m_edges[i][j].Fa && !m_edges[i][j].Ba) {
		if (creaseUserMin <= angle && angle <= creaseUserMax)
			m_edges[i][j].A_CUser = true;
		else
			m_edges[i][j].A_CUser = false;
		setMVFlags(i, j);
	} else {
		m_edges[i][j].A_CUser = false;
		m_edges[i][j].A_SlopeS = false;
	}
}

GLvoid Model::createEdgeBuffer(void) {
	// get max number of adjacent vertices
	maxAdjVertices=0;
	GLuint *verts = new GLuint[m_numvertices+1];
	for(GLuint i = 0; i < m_numvertices+1 ; i++)
		verts[i]=0;
	for (GLuint i=0; i < m_numtriangles; i++) {
		GLuint v1 = m_triangles[i].inq_vindex(0);
		GLuint v2 = m_triangles[i].inq_vindex(1);
		GLuint v3 = m_triangles[i].inq_vindex(2);
		verts[v1]+= 2;
		verts[v2]+= 2;
		verts[v3]+= 2;
		if (verts[v1] > maxAdjVertices) maxAdjVertices = verts[v1];
		if (verts[v2] > maxAdjVertices) maxAdjVertices = verts[v2];
		if (verts[v3] > maxAdjVertices) maxAdjVertices = verts[v3];
	}
	maxAdjVertices = maxAdjVertices/2 + 1;
	// printf("maxadjv: %d\n", maxAdjVertices);

	// create edge buffer
	m_edges = new Edge*[m_numvertices];
	for(GLuint i = 0; i < m_numvertices ; i++)
		m_edges[i] = new Edge[maxAdjVertices];

	
	for (GLuint i=0; i < m_numtriangles; i++) {
		GLuint v1 = m_triangles[i].inq_vindex(0);
		GLuint v2 = m_triangles[i].inq_vindex(1);
		GLuint v3 = m_triangles[i].inq_vindex(2);

		// instead of sorting I just compare the vertices.

		if (v1 < v2) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1][j].V == 0 || m_edges[v1][j].V == v2) {
					if (m_edges[v1][j].V != 0) {
						m_edges[v1][j].T2 = i;
						shapeMeasures(v1, j);
					} else {
						m_edges[v1][j].V = v2;
						m_edges[v1][j].T1 = i;
					}
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2][j].V == 0 || m_edges[v2][j].V == v1) {
					if (m_edges[v2][j].V != 0) {
						m_edges[v2][j].T2 = i;
						shapeMeasures(v2, j);
					} else {
						m_edges[v2][j].V = v1;
						m_edges[v2][j].T1 = i;
					}
					break;
				}
			}
		}

		if (v1 < v3) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1][j].V == 0 || m_edges[v1][j].V == v3) {
					if (m_edges[v1][j].V != 0) {
						m_edges[v1][j].T2 = i;
						shapeMeasures(v1, j);
					} else {
						m_edges[v1][j].V = v3;
						m_edges[v1][j].T1 = i;
					}
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3][j].V == 0 || m_edges[v3][j].V == v1) {
					if (m_edges[v3][j].V != 0) {
						m_edges[v3][j].T2 = i;
						shapeMeasures(v3, j);
					} else {
						m_edges[v3][j].V = v1;
						m_edges[v3][j].T1 = i;
					}
					break;
				}
			}
		}

		if (v2 < v3) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2][j].V == 0 || m_edges[v2][j].V == v3) {
					if (m_edges[v2][j].V != 0) {
						m_edges[v2][j].T2 = i;
						shapeMeasures(v2, j);
					} else {
						m_edges[v2][j].V = v3;
						m_edges[v2][j].T1 = i;
					}
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3][j].V == 0 || m_edges[v3][j].V == v2) {
					if (m_edges[v3][j].V != 0) {
						m_edges[v3][j].T2 = i;
						shapeMeasures(v3, j);
					} else {
						m_edges[v3][j].V = v2;
						m_edges[v3][j].T1 = i;
					}
					break;
				}
			}
		}
	}

}

GLvoid Model::resetEdgeBuffer(void) {
	//reset
	for (GLuint i=0; i < m_numtriangles; i++) {
		GLuint v1i = m_triangles[i].inq_vindex(0);
		GLuint v2i = m_triangles[i].inq_vindex(1);
		GLuint v3i = m_triangles[i].inq_vindex(2);

		if (v1i < v2i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1i][j].V == v2i) {
					m_edges[v1i][j].F = false;
					m_edges[v1i][j].B = false;
					m_edges[v1i][j].Fa = false;
					m_edges[v1i][j].Ba = false;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2i][j].V == v1i) {
					m_edges[v2i][j].F = false;
					m_edges[v2i][j].B = false;
					m_edges[v2i][j].Fa = false;
					m_edges[v2i][j].Ba = false;
					break;
				}
			}
		}

		if (v1i < v3i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1i][j].V == v3i) {
					m_edges[v1i][j].F = false;
					m_edges[v1i][j].B = false;
					m_edges[v1i][j].Fa = false;
					m_edges[v1i][j].Ba = false;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3i][j].V == v1i) {
					m_edges[v3i][j].F = false;
					m_edges[v3i][j].B = false;
					m_edges[v3i][j].Fa = false;
					m_edges[v3i][j].Ba = false;
					break;
				}
			}
		}

		if (v2i < v3i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2i][j].V == v3i) {
					m_edges[v2i][j].F = false;
					m_edges[v2i][j].B = false;
					m_edges[v2i][j].Fa = false;
					m_edges[v2i][j].Ba = false;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3i][j].V == v2i) {
					m_edges[v3i][j].F = false;
					m_edges[v3i][j].B = false;
					m_edges[v3i][j].Fa = false;
					m_edges[v3i][j].Ba = false;
					break;
				}
			}
		}
	}
}

// set F, B, Fa, and Ba flags in edge buffer (m_edges) for 3 edges corresponing to a triangle with index i, normal, and vertex indices v1i, v2i, and v3i.
// nDotE is the pre-computed dot product between the eye vector and the normal
GLvoid Model::updateTriangleEdges(GLfloat nDotE, GLuint i, GLfloat* normal, GLuint v1i, GLuint v2i, GLuint v3i) {
	//front facing
	if (nDotE <= 0.0f) {
		if (v1i < v2i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1i][j].V == v2i) {
					m_edges[v1i][j].F = m_edges[v1i][j].F != true;
					m_edges[v1i][j].Fa = true;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2i][j].V == v1i) {
					m_edges[v2i][j].F = m_edges[v2i][j].F != true;
					m_edges[v2i][j].Fa = true;
					break;
				}
			}
		}

		if (v1i < v3i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1i][j].V == v3i) {
					m_edges[v1i][j].F = m_edges[v1i][j].F != true;
					m_edges[v1i][j].Fa = true;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3i][j].V == v1i) {
					m_edges[v3i][j].F = m_edges[v3i][j].F != true;
					m_edges[v3i][j].Fa = true;
					break;
				}
			}
		}

		if (v2i < v3i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2i][j].V == v3i) {
					m_edges[v2i][j].F = m_edges[v2i][j].F != true;
					m_edges[v2i][j].Fa = true;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3i][j].V == v2i) {
					m_edges[v3i][j].F = m_edges[v3i][j].F != true;
					m_edges[v3i][j].Fa = true;
					break;
				}
			}
		}

	// back facing
	} else {
		if (v1i < v2i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1i][j].V == v2i) {
					m_edges[v1i][j].B = m_edges[v1i][j].B != true;
					m_edges[v1i][j].Ba = true;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2i][j].V == v1i) {
					m_edges[v2i][j].B = m_edges[v2i][j].B != true;
					m_edges[v2i][j].Ba = true;
					break;
				}
			}
		}

		if (v1i < v3i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v1i][j].V == v3i) {
					m_edges[v1i][j].B = m_edges[v1i][j].B != true;
					m_edges[v1i][j].Ba = true;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3i][j].V == v1i) {
					m_edges[v3i][j].B = m_edges[v3i][j].B != true;
					m_edges[v3i][j].Ba = true;
					break;
				}
			}
		}

		if (v2i < v3i) {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v2i][j].V == v3i) {
					m_edges[v2i][j].B = m_edges[v2i][j].B != true;
					m_edges[v2i][j].Ba = true;
					break;
				}
			}
		} else {
			for (int j = 0; j < maxAdjVertices; j++) {
				if (m_edges[v3i][j].V == v2i) {
					m_edges[v3i][j].B = m_edges[v3i][j].B != true;
					m_edges[v3i][j].Ba = true;
					break;
				}
			}
		}
	}
}

GLvoid Model::updateEdgeBuffer(GLdouble cameraX, GLdouble cameraY, GLdouble cameraZ) {
	//reset (for debugging) (slow)
	/*for (GLuint i = 0; i<m_numvertices; i++) {
		for (GLuint j=0; j<maxAdjVertices; j++) {
			//m_edges[i][j].A = false; 
			m_edges[i][j].F = false;
			m_edges[i][j].B = false; 
			m_edges[i][j].Fa = false; 
			m_edges[i][j].Ba = false;
		}
	}*/
	
	resetEdgeBuffer();

	//int i=0;
	for (GLuint i=0; i < m_numtriangles; i++) {
		GLuint v1i = m_triangles[i].inq_vindex(0);
		GLuint v2i = m_triangles[i].inq_vindex(1);
		GLuint v3i = m_triangles[i].inq_vindex(2);

		GLfloat v1[3] = {m_vertices[3 * v1i], m_vertices[3 * v1i + 1], m_vertices[3 * v1i + 2]};
		GLfloat v2[3] = {m_vertices[3 * v2i], m_vertices[3 * v2i + 1], m_vertices[3 * v2i + 2]};
		GLfloat v3[3] = {m_vertices[3 * v3i], m_vertices[3 * v3i + 1], m_vertices[3 * v3i + 2]};

		//center of triangle
		GLfloat center[3] = {(v1[0]+v2[0]+v3[0])/3, (v1[1]+v2[1]+v3[1])/3, (v1[2]+v2[2]+v3[2])/3};

		// get eye vector (normalize(triangle center - camera position))
		GLfloat eye[3] = {center[0]-cameraX, center[1]-cameraY, center[2]-cameraZ};
		normalize_vector(eye);

		// get normal (normalize(vec1 x vec2))
		GLfloat vec1[3] = {v1[0]-v2[0], v1[1]-v2[1], v1[2]-v2[2]};
		GLfloat vec2[3] = {v1[0]-v3[0], v1[1]-v3[1], v1[2]-v3[2]};
		GLfloat normal[3];
		cross_product(vec1, vec2, normal);
		normalize_vector(normal);

		//N.E
		GLfloat nDotE = dot_product(normal, eye);

		// set A, F, B, Fa, and Ba flags in edge buffer (m_edges)
		updateTriangleEdges(nDotE, i, normal, v1i, v2i, v3i);
	}
}

GLvoid Model::printEdgeBuffer(void) {
	// print triangles first (for debugging)
	printf("Triangles:\n");
	for (GLuint i=0; i < m_numtriangles; i++) {
		printf("Triangle %d: v1 = %d, v2 = %d, v3 = %d\n", i, m_triangles[i].inq_vindex(0), m_triangles[i].inq_vindex(1), m_triangles[i].inq_vindex(2));
	}

	printf("\n");

	// print edge buffer
	printf("Edge Buffer (order - V,T1,T2,Acmodel,Acuser,A_SlopeS,F,B,Fa,Ba):\n");
	for (GLuint i=0; i < m_numvertices; i++) {
		printf("Vertex %d: ", i);
		for (GLuint j=0; j < maxAdjVertices; j++) {
			printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d  ", m_edges[i][j].V, m_edges[i][j].T1, m_edges[i][j].T2, m_edges[i][j].A_CModel, m_edges[i][j].A_CUser, m_edges[i][j].A_SlopeS, m_edges[i][j].F, m_edges[i][j].B, m_edges[i][j].Fa, m_edges[i][j].Ba);
		}
		printf("\n");
	}
}

//#############################################################################
// Reads a model description from a Wavefront .OBJ file.
//#############################################################################
GLvoid Model::read_obj(char* filename)
{
    FILE* file;
    
    // open the file //
    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "read_obj() failed: can't open data file \"%s\".\n",
            filename);
        exit(1);
    }
    
    // allocate a new model //
    m_pathname    = strdup(filename);
    m_mtllibname    = NULL;
    m_numvertices   = 0;
    m_vertices    = NULL;
    m_numnormals    = 0;
    m_normals     = NULL;
    m_numtexcoords  = 0;
    m_texcoords       = NULL;
    m_numfacetnorms = 0;
    m_facetnorms    = NULL;
    m_numtriangles  = 0;
    m_triangles       = NULL;
    m_nummaterials  = 0;
    m_materials       = NULL;
    m_numgroups       = 0;
	m_groups = NULL;
    m_position[0]   = 0.0;
    m_position[1]   = 0.0;
    m_position[2]   = 0.0;
    
    // make a first pass through the file to get a count of the number
    // of vertices, normals, texcoords & triangles //
    first_pass(file);

    // allocate memory //
    
	m_vertices = new GLfloat [3 * (m_numvertices + 1)];
    m_triangles = new Triangle [m_numtriangles];
    if (m_numnormals) {
        m_normals = new GLfloat [3 * (m_numnormals + 1)];
    }
    if (m_numtexcoords) {
        m_texcoords = new GLfloat [2 * (m_numtexcoords + 1)];
    }
    
    // rewind to beginning of file and read in the data this pass //
	rewind(file);
    
    second_pass(file);

    // close the file //
    fclose(file);

	createEdgeBuffer();
}

//#############################################################################
//	End
//#############################################################################

