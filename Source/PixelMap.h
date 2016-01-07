
#ifndef _PixelMapH
#define _PixelMapH

//#############################################################################
//	File:		PixelMap.h
//	Module:		CPSC 453 - Assignment #3 
// 	Description: Simple class definition for a pixel map 
//               with support for 2D textures	
//
//#############################################################################

//#############################################################################
//	Headers
//#############################################################################

#ifdef _MSC_VER
#include <string>
#include <iostream>
#include <fstream>
#include <strstream>
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
#include "Pixel.h"

//#############################################################################
//	Global definitions
//#############################################################################

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned long  ulong;

//#############################################################################
//	Global declarations
//#############################################################################

class PixelMap
{
private: 
	// dimensions of the pixel map
	int m_rows, m_cols; 

	// array of pixels
	Pixel* m_pixel; 

	// Table of color quantization
	Component *m_quantize;
	
public:

	// constructors
	PixelMap();
	// destructor
	~PixelMap();
	// read BMP file into this pixmap
	int readBMPFile(string fname); 

	//
	void makeCheckerBoard();
	//
	void setTexture(GLuint textureName);
}; 

//#############################################################################
//	End
//#############################################################################

#endif /* PixelMapH */
