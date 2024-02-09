#ifndef _EBO_H_
#define _EBO_H_

#include "openGLHeader.h"

/*
  CSCI 420 Computer Graphics, University of Southern California
  Shidong Zhang, 2024

  "Element Buffer Object" (EBO) helper class. 
  It creates and manages EBOs.
*/

class EBO
{
public:

    // Initialize the EBO.
    // "numElements": the number of elements
    // "usage" must be either GL_STATIC_DRAW or GL_DYNAMIC_DRAW
    EBO(int numElements, const unsigned int * data, const GLenum usage = GL_STATIC_DRAW);
    virtual ~EBO();

    // Binds (activates) this EBO.
    void Bind();

    // Get handle to this EBO.
    GLuint GetHandle() { return handle; }
    // Get the number of elements in this EBO.
    int GetNumElements() { return numElements; }

protected:
    GLuint handle; // the handle to the VBO
    int numElements;
};

#endif

