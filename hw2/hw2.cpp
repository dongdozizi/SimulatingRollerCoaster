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
#include "ebo.h"

#include <iostream>
#include <cstring>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

int mousePos[2] = { -1,-1 }; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

int screenShotCounter = 0;
int renderType = 1;
bool enableCameraMov = false;
float speed[2] = { 0.0,0.0 };
float eyeVec[3];
float focusVec[3];
float upVec[3];

// Rotate the camera
float focusRotate[3] = { 0.0f,0.0f,0.0f };
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
ImageIO* heightmapImage = nullptr;
ImageIO* texturemapImage = nullptr;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram* pipelineProgram = nullptr;

// Number of Vertices, VBOs, VAOs and EBOs
int numVertices;
VBO* vboVertices = nullptr;
VBO* vboColors = nullptr;
EBO* ebo = nullptr;
VAO* vao = nullptr;

// Write a screenshot to the specified filename.
void saveScreenshot(const char* filename)
{
    unsigned char* screenshotData = new unsigned char[windowWidth * windowHeight * 3];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

    ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

    if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
        cout << "File " << filename << " saved successfully." << endl;
    else cout << "Failed to save file " << filename << '.' << endl;

    delete[] screenshotData;
}

// Save screenshot in increasing number 0001,0002,0003....
void autoSave() {
    if (screenShotCounter <= 299) {
        char filename[40];
        sprintf(filename, "../Examples/Heightmap-%04d.jpg", ++screenShotCounter);
        saveScreenshot(filename);
    }
}

int frameCount = 0;
double lastTime = 0;
double fps = 0;

// Represents one spline control point.
struct Point
{
    double x, y, z;
};

// Contains the control points of the spline.
struct Spline
{
    int numControlPoints;
    Point* points;
} spline;

void loadSpline(char* argv)
{
    FILE* fileSpline = fopen(argv, "r");
    if (fileSpline == NULL)
    {
        printf("Cannot open file %s.\n", argv);
        exit(1);
    }

    // Read the number of spline control points.
    fscanf(fileSpline, "%d\n", &spline.numControlPoints);
    printf("Detected %d control points.\n", spline.numControlPoints);

    // Allocate memory.
    spline.points = (Point*)malloc(spline.numControlPoints * sizeof(Point));
    // Load the control points.
    for (int i = 0; i < spline.numControlPoints; i++)
    {
        if (fscanf(fileSpline, "%lf %lf %lf",
            &spline.points[i].x,
            &spline.points[i].y,
            &spline.points[i].z) != 3)
        {
            printf("Error: incorrect number of control points in file %s.\n", argv);
            exit(1);
        }
    }
}

// Multiply C = A * B, where A is a m x p matrix, and B is a p x n matrix.
// All matrices A, B, C must be pre-allocated (say, using malloc or similar).
// The memory storage for C must *not* overlap in memory with either A or B. 
// That is, you **cannot** do C = A * C, or C = C * B. However, A and B can overlap, and so C = A * A is fine, as long as the memory buffer for A is not overlaping in memory with that of C.
// Very important: All matrices are stored in **column-major** format.
// Example. Suppose 
//        [ 1 8 2 ]
//    A = [ 3 5 7 ]
//        [ 0 2 4 ]
//    Then, the storage in memory is
//     1, 3, 0, 8, 5, 2, 2, 7, 4. 
void MultiplyMatrices(int m, int p, int n, const double* A, const double* B, double* C)
{
    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            double entry = 0.0;
            for (int k = 0; k < p; k++)
                entry += A[k * m + i] * B[j * p + k];
            C[m * j + i] = entry;
        }
    }
}

int initTexture(const char* imageFilename, GLuint textureHandle)
{
    // Read the texture image.
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);

    if (err != ImageIO::OK)
    {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }

    // Check that the number of bytes is a multiple of 4.
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }

    // Allocate space for an array of pixels.
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

    // Fill the pixelsRGBA array with the image pixels.
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }

    // Bind the texture.
    glBindTexture(GL_TEXTURE_2D, textureHandle);

    // Initialize the texture.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

    // Generate the mipmaps for this texture.
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set the texture parameters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Query support for anisotropic texture filtering.
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // Set anisotropic texture filtering.
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

    // Query for any errors.
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }

    // De-allocate the pixel array -- it is no longer needed.
    delete[] pixelsRGBA;

    return 0;
}

void idleFunc()
{
    // Do some stuff... 
    // For example, here, you can save the screenshots to disk (to make the animation).

    // Notify GLUT that it should call displayFunc.
    double currentTime = glutGet(GLUT_ELAPSED_TIME) * 0.001;
    ++frameCount;
    double timeInterval = currentTime - lastTime;

    if (timeInterval > 1.0 / 10.0) {
        //autoSave();
        lastTime = currentTime;
    }

    //if (timeInterval > 1.0) {
    //    fps = frameCount / timeInterval;
    //    cout<<"FPS: "<<fps;

    //    frameCount = 0;

    //     cout<<"\n Translate ";
    //     for(int i=0;i<3;i++) cout<<terrainTranslate[i]<<" ";cout<<" | Rotate ";
    //     for(int i=0;i<3;i++) cout<<terrainRotate[i]<<" ";cout<<" | Scale ";
    //     for(int i=0;i<3;i++) cout<<terrainScale[i]<<" ";cout<<" | Focus Rotate ";
    //     for (int i = 0; i < 3; i++) cout << focusRotate[i] << " "; cout << " | FocuseVec ";
    //     for (int i = 0; i < 3; i++) cout << focusVec[i] << " "; cout << " | EyeVec ";
    //     for(int i=0;i<3;i++) cout<<eyeVec[i]<<" ";cout<<"\n";
    //    lastTime = currentTime;
    //}

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
    const float zNear = 0.01f;
    const float zFar = 1000.0f;
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
    // the change in mouse position since the last invocation of this function
    int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

    // In case the first move of mouse will not be display so sharp.
    if (mousePos[0] == -1 && mousePos[1] == -1) {
        mousePosDelta[0] = mousePosDelta[1] = 0;
    }
    if (!leftMouseButton && !middleMouseButton && !rightMouseButton && enableCameraMov) {
        focusRotate[0] = mousePosDelta[1] * 0.3f;
        focusRotate[1] = mousePosDelta[0] * 0.3f;
    }
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
#ifdef __APPLE__
        // nothing is needed on Apple
        if (controlState == SCALE) controlState = ROTATE;
#else
        // Windows, Linux
        controlState = ROTATE;
#endif

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

    case 'w': //Going forward
        speed[0] = min(speed[0] + 0.002, 0.004);
        break;

    case 's': //Going backward
        speed[0] = max(speed[0] - 0.002, -0.004);
        break;

    case 'a': //Going Left
        speed[1] = max(speed[1] - 0.002, -0.004);
        break;

    case 'd': //Going Right
        speed[1] = min(speed[1] + 0.002, 0.004);
        break;

    case 'c': //Set to initial view.
        terrainRotate[0] = terrainRotate[1] = terrainRotate[2] = 0.0;
        terrainScale[0] = terrainScale[1] = terrainScale[2] = 1.0;
        terrainTranslate[0] = terrainTranslate[1] = terrainTranslate[2] = 0.0;
        break;

    case 'v': //Enable Moving the camera
        speed[0] = speed[1] = 0.0;
        if (enableCameraMov) enableCameraMov = false;
        else enableCameraMov = true;
        break;

    case ' ':
        cout << "You pressed the spacebar." << endl;
        break;

    case 'x':
        // Take a screenshot.
        autoSave();
        //saveScreenshot("screenshot.jpg");
        break;
    }
}

void modifyFocusAndCamera() {

    float verticalVec[3] = { -focusVec[2],0,focusVec[0] };

    // change the camera position
    for (int i = 0; i < 3; i++) {
        eyeVec[i] += 1.0 * speed[0] * focusVec[i];
        eyeVec[i] += 1.0 * speed[1] * verticalVec[i];
    }

    matrix.SetMatrixMode(OpenGLMatrix::ModelView);

    // Modify the focus vector.
    matrix.LoadIdentity();
    matrix.Rotate(focusRotate[0], focusVec[2], 0.0, -focusVec[0]);
    matrix.Rotate(focusRotate[1], 0.0, -1.0, 0.0);

    float tmpMatrix[16];
    matrix.GetMatrix(tmpMatrix);
    glm::mat4 rotateMatrix = glm::make_mat4(tmpMatrix);
    glm::vec4 tmpFocus(focusVec[0], focusVec[1], focusVec[2], 1.0f);

    // Multiply the rotate matrix with the focus vector to get the rotated vector
    tmpFocus = rotateMatrix * tmpFocus;

    for (int i = 0; i < 3; i++) {
        focusVec[i] = tmpFocus[i];
        focusRotate[i] = 0;
    }
}

void displayFunc()
{
    // This function performs the actual rendering.

    // First, clear the screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (enableCameraMov) {
        modifyFocusAndCamera();
    }

    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    // Set up the camera position, focus point, and the up vector.
    matrix.LoadIdentity();
    // View
    matrix.LookAt(eyeVec[0], eyeVec[1], eyeVec[2],
        eyeVec[0] + focusVec[0], eyeVec[1] + focusVec[1], eyeVec[2] + focusVec[2],
        upVec[0], upVec[1], upVec[2]);

    // Model
    //matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
    //matrix.Rotate(terrainRotate[0], 1.0, 0.0, 0.0);
    //matrix.Rotate(terrainRotate[1], 0.0, 1.0, 0.0);
    //matrix.Rotate(terrainRotate[2], 0.0, 0.0, 1.0);
    //matrix.Scale(terrainScale[0], terrainScale[1] / 3.0, terrainScale[2]);

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

    vao->Bind();
    glDrawElements(GL_POINTS, numVertices, GL_UNSIGNED_INT, 0); // Render the VAO, by using element array, size is "numVertices", starting from vertex 0.

    // Swap the double-buffers.
    glutSwapBuffers();
}

void initPoint(float* positions, float* colors) {

    unsigned int* elements = (unsigned int*)malloc(numVertices * sizeof(unsigned int)); // initialize elements array

    //The elements for point is just 0,1,2,...,numVerticesPoint-1.
    for (int i = 0; i < heightmapImage->getHeight(); i++) {
        for (int j = 0; j < heightmapImage->getWidth(); j++) {
            int pos = i * heightmapImage->getWidth() + j;
            elements[pos] = pos;
        }
    }

    vboVertices = new VBO(numVertices, 3, positions, GL_STATIC_DRAW); // 3 values per position
    vboColors = new VBO(numVertices, 4, colors, GL_STATIC_DRAW); // 4 values per color    

    vao = new VAO();

    vao->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVertices, "position");
    vao->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColors, "color");
    ebo = new EBO(numVertices, elements, GL_STATIC_DRAW); //Bind the EBO

    //Free the elements;
    free(elements);
}

void initScene(int argc, char* argv[])
{

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

    numVertices = ; // This must be a global variable, so that we know how many vertices to render in glDrawArrays.
    float* positions = (float*)malloc(numVertices * 3 * sizeof(float)); // 3 floats per vertex, i.e., x,y,z
    float* colors = (float*)malloc(numVertices * 4 * sizeof(float)); // 4 floats per vertex, i.e., r,g,b,a

    initPoint(positions, colors); //Create the point VAO,VBO,EBO

    // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
    free(positions);
    free(colors);

    // Initialize variables in LookAt function
    // eyeVec[0]=0.5,eyeVec[1]=0.5,eyeVec[2]=0.5;
    // focusVec[0]=0.0,focusVec[1]=-0.5,focusVec[2]=-1.0;
    // upVec[0]=0.0,upVec[1]=1.0,upVec[2]=0.0;

    // Initialize variables in LookAt function
    eyeVec[0] = 0.0, eyeVec[1] = 1.3, eyeVec[2] = 1.7;
    focusVec[0] = 0.0, focusVec[1] = -1.3, focusVec[2] = -1.7;
    upVec[0] = 0.0, upVec[1] = 1.0, upVec[2] = 0.0;

    // Normalize the focusVec
    float sum = 0.0;
    for (int i = 0; i < 3; i++) sum += focusVec[i] * focusVec[i];
    sum = sqrt(sum);
    for (int i = 0; i < 3; i++) focusVec[i] /= sum;
    // Check for any OpenGL errors.
    std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char* argv[])
{

    if (argc < 2)
    {
        printf("Usage: %s <spline file>\n", argv[0]);
        exit(0);
    }

    // Load spline from the provided filename.
    loadSpline(argv[1]);

    printf("Loaded spline with %d control point(s).\n", spline.numControlPoints);

    cout << "Initializing GLUT..." << endl;
    glutInit(&argc, argv);

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
