#include "ebo.h"

#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

EBO::EBO(int numElements_, const unsigned int * data,  const GLenum usage) : numElements(numElements_)
{
    // Create the EBO handle 
    glGenBuffers(1, &handle);

    // Initialize the EBO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
    const int numBytes = numElements * sizeof(unsigned int);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBytes, data, usage);
}

EBO::~EBO()
{   
    // Delete the EBO.
    glDeleteBuffers(1, &handle);
}

void EBO::Bind()
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
}