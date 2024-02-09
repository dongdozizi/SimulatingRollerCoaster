/*
    CSCI 420 Computer Graphics, Computer Science, USC
    Assignment 1: Height Fields with Shaders.
    C/C++ starter code

    Student username: Shidong Zhang
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <iostream>
#include <cstring>

#if defined(WIN32) || defined(_WIN32)
    #ifdef _DEBUG
        #pragma comment(lib, "glew32d.lib")
    #else
        #pragma comment(lib, "glew32.lib")
    #endif
#endif

#if defined(WIN32) || defined(_WIN32)
    char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
    char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

int renderType=3;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 1";

// Stores the image loaded from disk.
ImageIO * heightmapImage;

// Number of vertices in the single triangle (starter code).
int numVerticesPoint;
int numVerticesLine;
int numVerticesTriangle;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram * pipelineProgram = nullptr;
VBO * vboVerticesPoint = nullptr;
VBO * vboColorsPoint = nullptr;
VAO * vaoPoint = nullptr;

VBO * vboVerticesLine = nullptr;
VBO * vboColorsLine = nullptr;
VAO * vaoLine = nullptr;

VBO * vboVerticesTriangle = nullptr;
VBO * vboColorsTriangle = nullptr;
VAO * vaoTriangle = nullptr;

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
    unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

    ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

    if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
        cout << "File " << filename << " saved successfully." << endl;
    else cout << "Failed to save file " << filename << '.' << endl;

    delete [] screenshotData;
}

void idleFunc()
{
    // Do some stuff... 
    // For example, here, you can save the screenshots to disk (to make the animation).

    // Notify GLUT that it should call displayFunc.
    glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
    glViewport(0, 0, w, h);

    // When the window has been resized, we need to re-set our projection matrix.
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.LoadIdentity();
    // You need to be careful about setting the zNear and zFar. 
    // Anything closer than zNear, or further than zFar, will be culled.
    const float zNear = 0.1f;
    const float zFar = 10000.0f;
    const float humanFieldOfView = 60.0f;
    matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
    // Mouse has moved, and one of the mouse buttons is pressed (dragging).

    // the change in mouse position since the last invocation of this function
    int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

    switch (controlState)
    {
        // translate the terrain
        case TRANSLATE:
            if (leftMouseButton)
            {
                // control x,y translation via the left mouse button
                terrainTranslate[0] += mousePosDelta[0] * 0.01f;
                terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
            }
            if (middleMouseButton)
            {
                // control z translation via the middle mouse button
                terrainTranslate[2] += mousePosDelta[1] * 0.01f;
            }
            break;

        // rotate the terrain
        case ROTATE:
            if (leftMouseButton)
            {
                // control x,y rotation via the left mouse button
                terrainRotate[0] += mousePosDelta[1];
                terrainRotate[1] += mousePosDelta[0];
            }
            if (middleMouseButton)
            {
                // control z rotation via the middle mouse button
                terrainRotate[2] += mousePosDelta[1];
            }
            break;

        // scale the terrain
        case SCALE:
            if (leftMouseButton)
            {
                // control x,y scaling via the left mouse button
                terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
                terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
            }
            if (middleMouseButton)
            {
                // control z scaling via the middle mouse button
                terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
            }
            break;
    }

    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
    // Mouse has moved.
    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
    // A mouse button has has been pressed or depressed.

    // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
    switch (button)
    {
        case GLUT_LEFT_BUTTON:
            leftMouseButton = (state == GLUT_DOWN);
        break;

        case GLUT_MIDDLE_BUTTON:
            middleMouseButton = (state == GLUT_DOWN);
        break;

        case GLUT_RIGHT_BUTTON:
            rightMouseButton = (state == GLUT_DOWN);
        break;
    }

    // Keep track of whether CTRL and SHIFT keys are pressed.
    switch (glutGetModifiers())
    {
        case GLUT_ACTIVE_CTRL:
            controlState = TRANSLATE;
        break;

        case GLUT_ACTIVE_SHIFT:
            controlState = SCALE;
        break;

        // If CTRL and SHIFT are not pressed, we are in rotate mode.
        default:
            controlState = ROTATE;
        break;
    }

    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27: // ESC key
            exit(0); // exit the program
        break;

        case ' ':
            cout << "You pressed the spacebar." << endl;
        break;

        case 'x':
            // Take a screenshot.
            saveScreenshot("screenshot.jpg");
        break;

        case '1':
            renderType=1;
        break;

        case '2':
            renderType=2;
        break;

        case '3':
            renderType=3;
        break;
    }
}

int frameCount = 0;
double lastTime = 0;
double fps = 0;

void displayFPS() {
    double currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001;
    ++frameCount;

    double timeInterval = currentTime - lastTime;

    if (timeInterval > 1.0) {
        fps = frameCount / timeInterval;

        char title[80];
        sprintf(title, "FPS: %.2f", fps);
        glutSetWindowTitle(title);

        frameCount = 0;
        lastTime = currentTime;
    }
}

void displayFunc()
{
    // This function performs the actual rendering.

    // First, clear the screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the camera position, focus point, and the up vector.
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    matrix.LookAt(0.0, 0.7* heightmapImage->getWidth(), 2.0 * heightmapImage->getWidth() / 2.0,
                  0.0, 0.0, 0.0,
                  0.0, 1.0, 0.0);

    // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
    // ...

    // Read the current modelview and projection matrices from our helper class.
    // The matrices are only read here; nothing is actually communicated to OpenGL yet.
    float modelViewMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetMatrix(modelViewMatrix);

    float projectionMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(projectionMatrix);

    // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
    // Important: these matrices must be uploaded to *all* pipeline programs used.
    // In hw1, there is only one pipeline program, but in hw2 there will be several of them.
    // In such a case, you must separately upload to *each* pipeline program.
    // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
    pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

    // Execute the rendering.
    // Bind the VAO that we want to render. Remember, one object = one VAO. 

    if(renderType==1){
        vaoPoint->Bind();
        glDrawArrays(GL_POINTS, 0, numVerticesPoint); // Render the VAO, by rendering "numVertices", starting from vertex 0.
    }
    else if(renderType==2){
        vaoLine->Bind();
        glDrawArrays(GL_LINES, 0, numVerticesLine); // Render the VAO, by rendering "numVertices", starting from vertex 0.
    }
    else if(renderType==3){
        vaoTriangle->Bind();
        glDrawArrays(GL_TRIANGLES, 0, numVerticesTriangle);
    }

    displayFPS();

    // Swap the double-buffers.
    glutSwapBuffers();
}

void initPoint(float* positions,float* colors){
    for (int i = 0; i < heightmapImage->getHeight(); i++) {
        for (int j = 0,pos=i*heightmapImage->getWidth()*3; j < heightmapImage->getWidth(); j++,pos+=3) {
            positions[pos] = i-1.0*(heightmapImage->getHeight() - 1.0)/2.0;
            positions[pos + 1] = 1.0*heightmapImage->getPixel(i,j,0)/256.0*50.0-25.0;
            positions[pos + 2] = -j+1.0*(heightmapImage->getWidth() - 1.0)/2.0;
        }
    }

    for (int i = 0; i < heightmapImage->getHeight(); i++) {
        for (int j = 0, pos = i * heightmapImage->getWidth()*4; j < heightmapImage->getWidth(); j++, pos += 4) {
            colors[pos] = 1.0*heightmapImage->getPixel(i,j,0)/255.0;
            colors[pos + 1] = 1.0*heightmapImage->getPixel(i,j,0)/255.0;
            colors[pos + 2] = 1.0*heightmapImage->getPixel(i,j,0)/255.0;
            colors[pos + 3] = 1.0;
        }
    }

    vboVerticesPoint = new VBO(numVerticesPoint, 3, positions, GL_STATIC_DRAW); // 3 values per position
    vboColorsPoint = new VBO(numVerticesPoint, 4, colors, GL_STATIC_DRAW); // 4 values per color

    vaoPoint = new VAO();

    vaoPoint->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVerticesPoint, "position");

    vaoPoint->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsPoint, "color");

    // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.

}

void initLine(float* positions,float* colors){

    numVerticesLine = heightmapImage->getHeight() * (heightmapImage->getWidth() - 1)*2
                    + (heightmapImage->getHeight() - 1) * heightmapImage->getWidth()*2;

    float* positionsLine = (float*)malloc(numVerticesLine * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z

    int pos=0;

    for (int i = 0; i < heightmapImage->getHeight(); i++) {
        for (int j = 0; j < heightmapImage->getWidth()-1; j++) {
            for(int k=(i*heightmapImage->getWidth()+j)*3;k<(i*heightmapImage->getWidth()+j)*3+6;k++,pos++){
                positionsLine[pos]=positions[k];
            }
        }
    }

    for (int i = 0; i < heightmapImage->getHeight()-1; i++) {
        for (int j = 0; j < heightmapImage->getWidth(); j++,pos+=3) {
            for(int k=(i*heightmapImage->getWidth()+j)*3;k<(i*heightmapImage->getWidth()+j)*3+3;k++,pos++){
                positionsLine[pos] = positions[k];
                positionsLine[pos+3]=positions[k+heightmapImage->getWidth()*3];
            }
        }
    }

    float* colorsLine = (float*)malloc(numVerticesLine * 4 * sizeof(float));

    pos=0;

    // Vertex colors.
    for (int i = 0; i < heightmapImage->getHeight(); i++) {
        for (int j = 0; j < heightmapImage->getWidth()-1; j++) {
            for(int k=(i*heightmapImage->getWidth()+j)*4;k<(i*heightmapImage->getWidth()+j)*4+8;k++,pos++){
                colorsLine[pos]=colors[k];
            }
        }
    }

    for (int i = 0; i < heightmapImage->getHeight()-1; i++) {
        for (int j = 0; j < heightmapImage->getWidth(); j++,pos+=4) {
            for(int k=(i*heightmapImage->getWidth()+j)*4;k<(i*heightmapImage->getWidth()+j)*4+4;k++,pos++){
                colorsLine[pos] = colors[k];
                colorsLine[pos+4]=colors[k+heightmapImage->getWidth()*4];
            }
        }
    }

    vboVerticesLine = new VBO(numVerticesLine, 3, positionsLine, GL_STATIC_DRAW); // 3 values per position
    vboColorsLine = new VBO(numVerticesLine, 4, colorsLine, GL_STATIC_DRAW); // 4 values per color

    vaoLine = new VAO();

    vaoLine->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVerticesLine, "position");

    vaoLine->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsLine, "color");
}

void initTriangle(float* positions,float* colors){

    numVerticesTriangle = (heightmapImage->getHeight() - 1) * (heightmapImage->getWidth() - 1)*6;

    float* positionsTriangle = (float*)malloc(numVerticesTriangle * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z

    int dxy[6]={0,1,(int)heightmapImage->getWidth(),
                1,(int)heightmapImage->getWidth()+1,(int)heightmapImage->getWidth()};

    for (int i = 0; i < heightmapImage->getHeight()-1; i++) {
        for (int j = 0; j < heightmapImage->getWidth()-1; j++) {
            for(int k1=0;k1<6;k1++){
                for(int k2=0;k2<3;k2++){
                    int cur=(i*(heightmapImage->getWidth()-1)+j);
                    positionsTriangle[(i*(heightmapImage->getWidth()-1)+j)*6*3+k1*3+k2]=positions[(i*(heightmapImage->getWidth())+j)*3+dxy[k1]*3+k2];
                }
            }
        }
    }

    float* colorsTriangle = (float*)malloc(numVerticesTriangle * 4 * sizeof(float));

    for (int i = 0; i < heightmapImage->getHeight()-1; i++) {
        for (int j = 0; j < heightmapImage->getWidth()-1; j++) {
            for(int k1=0;k1<6;k1++){
                for(int k2=0;k2<4;k2++){
                    int cur=(i*(heightmapImage->getWidth()-1)+j);
                    colorsTriangle[(i*(heightmapImage->getWidth()-1)+j)*6*4+k1*4+k2]=colors[(i*(heightmapImage->getWidth())+j)*4+dxy[k1]*4+k2];
                }
            }
        }
    }

    vboVerticesTriangle = new VBO(numVerticesTriangle, 3, positionsTriangle, GL_STATIC_DRAW); // 3 values per position
    vboColorsTriangle = new VBO(numVerticesTriangle, 4, colorsTriangle, GL_STATIC_DRAW); // 4 values per color

    vaoTriangle = new VAO();

    vaoTriangle->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVerticesTriangle, "position");

    vaoTriangle->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsTriangle, "color");
}

void initScene(int argc, char *argv[])
{
    // Load the image from a jpeg disk file into main memory.
    heightmapImage = new ImageIO();
    if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
    {
        cout << "Error reading image " << argv[1] << "." << endl;
        exit(EXIT_FAILURE);
    }

    // Set the background color.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

    // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
    glEnable(GL_DEPTH_TEST);

    // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
    // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
    // In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
    // In hw2, we will need to shade different objects with different shaders, and therefore, we will have
    // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
    pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    // Load and set up the pipeline program, including its shaders.
    if (pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
    {
        cout << "Failed to build the pipeline program." << endl;
        throw 1;
    } 
    cout << "Successfully built the pipeline program." << endl;
        
    // Bind the pipeline program that we just created. 
    // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
    // any object rendered from that point on, will use those shaders.
    // When the application starts, no pipeline program is bound, which means that rendering is not set up.
    // So, at some point (such as below), we need to bind a pipeline program.
    // From that point on, exactly one pipeline program is bound at any moment of time.
    pipelineProgram->Bind();

    numVerticesPoint = heightmapImage->getHeight()* heightmapImage->getWidth(); // This must be a global variable, so that we know how many vertices to render in glDrawArrays.
    float* positions = (float*)malloc(numVerticesPoint * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
    float* colors = (float*)malloc(numVerticesPoint * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a

    initPoint(positions,colors);

    initLine(positions,colors);

    initTriangle(positions,colors);

    free(positions);
    free(colors);

    // Prepare the triangle position and color data for the VBO. 
    // The code below sets up a single triangle (3 vertices).
    // The triangle will be rendered using GL_TRIANGLES (in displayFunc()).

    // numVerticesPoint = 3 // This must be a global variable, so that we know how many vertices to render in glDrawArrays.

    // Vertex positions.
    //float * positions = (float*) malloc (numVertices * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
    //positions[0] = 0.0; positions[1] = 0.0; positions[2] = 0.0; // (x,y,z) coordinates of the first vertex
    //positions[3] = 0.0; positions[4] = 1.0; positions[5] = 0.0; // (x,y,z) coordinates of the second vertex
    //positions[6] = 1.0; positions[7] = 0.0; positions[8] = 0.0; // (x,y,z) coordinates of the third vertex

    //// Vertex colors.
    //float * colors = (float*) malloc (numVertices * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a
    //colors[0] = 0.0; colors[1] = 0.0;    colors[2] = 1.0;    colors[3] = 1.0; // (r,g,b,a) channels of the first vertex
    //colors[4] = 1.0; colors[5] = 0.0;    colors[6] = 0.0;    colors[7] = 1.0; // (r,g,b,a) channels of the second vertex
    //colors[8] = 0.0; colors[9] = 1.0; colors[10] = 0.0; colors[11] = 1.0; // (r,g,b,a) channels of the third vertex

    // Create the VBOs. 
    // We make a separate VBO for vertices and colors. 
    // This operation must be performed BEFORE we initialize any VAOs.
    // vboVertices = new VBO(numVertices, 3, positions, GL_STATIC_DRAW); // 3 values per position
    // vboColors = new VBO(numVertices, 4, colors, GL_STATIC_DRAW); // 4 values per color

    // Create the VAOs. There is a single VAO in this example.
    // Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
    // A VAO contains the geometry for a single object. There should be one VAO per object.
    // In this homework, "geometry" means vertex positions and colors. In homework 2, it will also include
    // vertex normal and vertex texture coordinates for texture mapping.
    // vao = new VAO();

    // Set up the relationship between the "position" shader variable and the VAO.
    // Important: any typo in the shader variable name will lead to malfunction.
    // vao->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVerticesPoint, "position");

    // Set up the relationship between the "color" shader variable and the VAO.
    // Important: any typo in the shader variable name will lead to malfunction.
    // vao->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColorsPoint, "color");

    // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
    // free(positions);
    // free(colors);

    // Check for any OpenGL errors.
    std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        cout << "The arguments are incorrect." << endl;
        cout << "usage: ./hw1 <heightmap file>" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Initializing GLUT..." << endl;
    glutInit(&argc,argv);

    cout << "Initializing OpenGL..." << endl;

    #ifdef __APPLE__
        glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    #else
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
    #endif

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(0, 0);    
    glutCreateWindow(windowTitle);

    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    #ifdef __APPLE__
        // This is needed on recent Mac OS X versions to correctly display the window.
        glutReshapeWindow(windowWidth - 1, windowHeight - 1);
    #endif

    // Tells GLUT to use a particular display function to redraw.
    glutDisplayFunc(displayFunc);
    // Perform animation inside idleFunc.
    glutIdleFunc(idleFunc);
    // callback for mouse drags
    glutMotionFunc(mouseMotionDragFunc);
    // callback for idle mouse movement
    glutPassiveMotionFunc(mouseMotionFunc);
    // callback for mouse button changes
    glutMouseFunc(mouseButtonFunc);
    // callback for resizing the window
    glutReshapeFunc(reshapeFunc);
    // callback for pressing the keys on the keyboard
    glutKeyboardFunc(keyboardFunc);

    // init glew
    #ifdef __APPLE__
        // nothing is needed on Apple
    #else
        // Windows, Linux
        GLint result = glewInit();
        if (result != GLEW_OK)
        {
            cout << "error: " << glewGetErrorString(result) << endl;
            exit(EXIT_FAILURE);
        }
    #endif

    // Perform the initialization.
    initScene(argc, argv);

    // Sink forever into the GLUT loop.
    glutMainLoop();
}

