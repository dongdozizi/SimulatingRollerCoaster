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
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>

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

// Catmull-Rom Spline Matrix
glm::mat4 catmullMatrix;
double catmullS = 0.5;
double maxLength = 0.002; // The maximum length for drawing lines in Spline
vector<glm::mat4x3> mulMatrix; // Mult matrix vector for every curve

int screenShotCounter = 0;
int renderType = 1;
bool enableCameraMov = false; // true when enable moving camera

float lastTime=0.0; // last time render the window
float rollerMinSpeed=5.0; // minimum speed when start the roller coaster
float rollerSpeed=0.0; // speed of roller coaster (per second not per frame)
float rollerU = 0.0; // Initial
glm::vec3 rollerPos; // position of rollercoaster
glm::vec3 rollerTangent; // tangent vector
glm::vec3 rollerBinormal; // binormal vector
glm::vec3 rollerNormal; // normal vector

float camMaxSpeed=2.0; // minimum speed when moving camera (per second not per frame)
glm::vec2 camSpeed(0.0f,0.0f); // Speed of camera when enable moving camera
glm::vec3 cameraEye; // camera position
glm::vec3 cameraFocus; // focus vector
glm::vec3 cameraUp; // up vector, also normal vector for roller coaster, when enable moving camera it is (0,1,0)

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
char windowTitle[512] = "CSCI 420 Homework 2";

// CSCI 420 helper classes.
OpenGLMatrix matrix;

// Light property
glm::vec4 La(1.0f,1.0f,1.0f,1.0f); // light ambient
glm::vec4 Ld(1.0f,1.0f,1.0f,1.0f); // light diffuse
glm::vec4 Ls(1.0f, 1.0f, 1.0f, 1.0f); // light specular
glm::vec3 lightDirection(0.0f,1.0f,0.0f); // Suppose the sun is far away

// Wood property
glm::vec4 kaWood(0.2f, 0.2f, 0.2f, 1.0f); // mesh ambient
glm::vec4 kdWood(0.5f, 0.5f, 0.5f, 1.0f); // mesh diffuse
glm::vec4 ksWood(0.3f, 0.3f, 0.3f, 1.0f); // mesh specular
float alphaWood=1.0f; // shininess

// Metal property
glm::vec4 kaMetal(0.2f, 0.2f, 0.2f, 1.0f); // mesh ambient
glm::vec4 kdMetal(0.4f, 0.4f, 0.4f, 1.0f); // mesh diffuse
glm::vec4 ksMetal(0.9f, 0.9f, 0.9f, 1.0f); // mesh specular
float alphaMetal = 15.0f; // shininess

// 3D object class
struct ThreeDimensionObject{
    PipelineProgram* pipelineProgram = nullptr; // pipeline program
    int numVertices; // number of vertices
    int numElements; // number of elements
    GLuint texHandle; // texture handle
    VBO* vboVertices = nullptr; // VBO of vertices
    VBO* vboTexCoord = nullptr; // VBO of texture coordinates
    VBO* vboNormals = nullptr; // VBO of normals
    EBO* ebo = nullptr; // EBO of the object
    VAO* vao = nullptr; // VAO
};

// Rail properties
struct Rail : ThreeDimensionObject {
    float heightT = 0.03f; // T shape height
    float widthT = 0.015f; // T shape width
    float width = 0.1f; // Rail width
}rail;

// Rail tie properties
struct RailTie: ThreeDimensionObject{
    float height=0.05;
    float width;
    float length;
    float space=0.5f;
    int count;
}railTie;

// Ground properties
ThreeDimensionObject ground;

// Skybox properties
ThreeDimensionObject skyBox;

// Spline property
int numVerticesSpline; // number of points generated
vector<glm::vec3> splinePoints; // spline points
vector<float> uVec; // vector record each u for spline points
vector<float> pointDistance; // The distance of each points starting from the beginning
float splineLength; // Then total length of the spline
vector<int> uPosVec; // record the start position of each spline.
vector<glm::vec3> splineTangent; // Tangent of the spline
vector<glm::vec3> splineNormal; // Normal of Spline
vector<glm::vec3> splineBinormal; // Binormal of Spline

// Represents one spline control point.
struct Point {
    double x, y, z;
};

// Contains the control points of the spline.
struct Spline {
    int numControlPoints;
    Point* points;
} spline;

int frameCount = 0;
double lastTimeSave = 0; // last time when saving images
double lastTimeFps = 0; // last time when calculating fps;
double fps = 0;

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
    if (screenShotCounter <= 999) {
        char filename[40];
        sprintf(filename, "../Examples/Heightmap-%04d.jpg", ++screenShotCounter);
        saveScreenshot(filename);
    }
}

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
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
    
    double timeInterval = currentTime - lastTimeSave;
    if (timeInterval > 0.1f) {
        cout << "Current time is : " << currentTime << "\n";
        cout << "Camera ";
        cout << " | cameraUp     "; for (int i = 0; i < 3; i++) cout << cameraUp[i] << " "; cout << " ";
        cout << " | FocuseVec "; for (int i = 0; i < 3; i++) cout << cameraFocus[i] << " ";
        cout << " | cameraEye    "; for (int i = 0; i < 3; i++) cout << cameraEye[i] << " ";

        cout << "\n";
        cout << "Roller ";
        cout << " | RollerPos "; for (int i = 0; i < 3; i++) cout << rollerPos[i] << " "; cout << " ";
        cout << " | tangenet  "; for (int i = 0; i < 3; i++) cout << rollerTangent[i] << " ";
        cout << " | normal    "; for (int i = 0; i < 3; i++) cout << rollerNormal[i] << " ";
        cout << " | binormal  "; for (int i = 0; i < 3; i++) cout << rollerBinormal[i] << " ";

        cout << "\n";
        cout << " u | currentU " << rollerU;
        cout << "\n";
        //autoSave();
        lastTimeSave = currentTime;
    }

    ++frameCount;
    timeInterval = currentTime - lastTimeFps;
    if (timeInterval > 1.0) {
        fps = frameCount / timeInterval;
        sprintf(windowTitle, "CSCI 420 Homework 2 | fps: %.2f", fps);
        glutSetWindowTitle(windowTitle);
        frameCount = 0;
        lastTimeFps = currentTime;
    }

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
    const float zNear = 0.001f;
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
    if (!leftMouseButton && !middleMouseButton && !rightMouseButton) {
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

void keyboardFunc(unsigned char key, int x, int y){
    switch (key)
    {
    case 27: // ESC key
        exit(0); // exit the program
        break;

    case 'w': //Going forward 
        camSpeed[0] = min(camSpeed[0] + 0.5f, 1.0f);
        break;

    case 's': //Going backward
        camSpeed[0] = max(camSpeed[0] - 0.5f, -1.0f);
        break;

    case 'a': //Going Left
        camSpeed[1] = max(camSpeed[1] - 0.5f, -1.0f);
        break;

    case 'd': //Going Right
        camSpeed[1] = min(camSpeed[1] + 0.5f, 1.0f);
        break;

    case 'v': //Enable Moving the camera
        camSpeed[0] = camSpeed[1] = 0.0;
        if (enableCameraMov){
            cameraFocus=rollerTangent;
            enableCameraMov = false;
        }
        else{
            cameraFocus=glm::vec3(-1.0f,0.0f,0.0f);
            cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
            enableCameraMov = true;
        }
        break;

    case ' ': // Start or Stop the roller coaster
        if(rollerSpeed>0.0) rollerSpeed=0.0;
        else rollerSpeed=rollerMinSpeed;
        break;

    case 'x':
        // Take a screenshot.
        autoSave();
        //saveScreenshot("screenshot.jpg");
        break;
    }
}

void modifyFocusAndCamera(float timeInterval,glm::vec3 cameraUp) {
    
    // Moving the camera
    if(enableCameraMov){
        glm::vec3 verticalVec(-cameraFocus[2],0,cameraFocus[0]);
        cameraEye+=camMaxSpeed*timeInterval*(camSpeed[0]*cameraFocus+camSpeed[1]*verticalVec);
    }
    glm::mat4 transformMat(1.0f);
    glm::vec3 normal=glm::cross(cameraUp,cameraFocus);

    transformMat=transformMat*glm::rotate(glm::radians(focusRotate[0]),normal);
    transformMat = transformMat * glm::rotate(glm::radians(focusRotate[1]), -cameraUp);
    focusRotate[0] = focusRotate[1] = 0.0f;
    
    // Multiply the rotate matrix with the focus vector to get the rotated vector
    cameraFocus=glm::vec3(transformMat*glm::vec4(cameraFocus,0.0f));
}

void calculateNewCameraRoller() {

    rollerU = rollerU + 0.001 * rollerSpeed;
    if (rollerU >= 1.0f*spline.numControlPoints) {
        rollerU -= 1.0f * spline.numControlPoints;
    }
    int splineCount = floor(rollerU);
    float tmpU = rollerU - 1.0f*splineCount;

    // Use binary search to find the next u position
    int l = uPosVec[splineCount], r = uPosVec[splineCount + 1];
    int uPos = lower_bound(uVec.begin() + l, uVec.begin() + r, tmpU) - uVec.begin();

    // Interpolate the position
    glm::vec3 rollerPosNew;
    glm::vec4 uCalc(tmpU*tmpU*tmpU,tmpU*tmpU,tmpU,1);
    rollerPosNew=mulMatrix[splineCount]*uCalc;

    glm::vec3 rollerTangentNew = splineTangent[uPos];
    glm::vec3 rollerNormalNew = splineNormal[uPos];
    glm::vec3 rollerBinormalNew = splineBinormal[uPos];

    if (!enableCameraMov) {
        cameraUp = rollerNormalNew;
        cameraEye = rollerPosNew + 0.1f * cameraUp;
        cameraFocus = glm::dot(cameraFocus,rollerTangent)*rollerTangentNew+
            glm::dot(cameraFocus,rollerNormal)*rollerNormalNew+
            glm::dot(cameraFocus,rollerBinormal)*rollerBinormalNew;
        cameraFocus=glm::normalize(cameraFocus);
    }
    rollerPos = rollerPosNew;
    rollerTangent = rollerTangentNew;
    rollerNormal = rollerNormalNew;
    rollerBinormal = rollerBinormalNew;
}

void displayFunc()
{
    // This function performs the actual rendering.

    // Calculate the time interval and refresh the last time
    float currentTime=glutGet(GLUT_ELAPSED_TIME) * 0.001;
    float timeInterval=currentTime-lastTime;
    lastTime=currentTime;

    // First, clear the screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    modifyFocusAndCamera(timeInterval,cameraUp);

    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    // Set up the camera position, focus point, and the up vector.
    matrix.LoadIdentity();
    // View
    matrix.LookAt(cameraEye[0], cameraEye[1], cameraEye[2],
                  cameraEye[0] + cameraFocus[0], cameraEye[1] + cameraFocus[1], cameraEye[2] + cameraFocus[2],
                  cameraUp[0], cameraUp[1], cameraUp[2]);

    // Get modelView matrix
    float modelViewMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetMatrix(modelViewMatrix);

    glm::vec3 viewLightDirection = glm::normalize(glm::vec3(glm::make_mat4(modelViewMatrix) * glm::vec4(lightDirection, 0.0f)));

    // Get projection matrix
    float projectionMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(projectionMatrix);

    // Get normal matrix
    float normalMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetNormalMatrix(normalMatrix);

    // Draw rail
    rail.pipelineProgram->Bind();
    // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
    rail.pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    rail.pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
    rail.pipelineProgram->SetUniformVariableMatrix4fv("normalMatrix", GL_FALSE, normalMatrix);
    rail.pipelineProgram->SetUniformVariable3fv("viewLightDirection", glm::value_ptr(viewLightDirection));
    rail.pipelineProgram->SetUniformVariable4fv("La", glm::value_ptr(La));
    rail.pipelineProgram->SetUniformVariable4fv("Ld", glm::value_ptr(Ld));
    rail.pipelineProgram->SetUniformVariable4fv("Ls", glm::value_ptr(Ls));
    rail.pipelineProgram->SetUniformVariable4fv("ka", glm::value_ptr(kaMetal));
    rail.pipelineProgram->SetUniformVariable4fv("kd", glm::value_ptr(kdMetal));
    rail.pipelineProgram->SetUniformVariable4fv("ks", glm::value_ptr(ksMetal));
    rail.pipelineProgram->SetUniformVariablef("alpha", alphaMetal);
    rail.vao->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rail.texHandle);
    glDrawElements(GL_TRIANGLES, rail.numElements, GL_UNSIGNED_INT, 0); // Render the VAO, by using element array, size is "numVerticesRailE", starting from vertex 0.

    // Draw ground
    ground.pipelineProgram->Bind();
    ground.pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    ground.pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
    ground.vao->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ground.texHandle);
    glDrawElements(GL_TRIANGLES, ground.numElements, GL_UNSIGNED_INT, 0); // Render the VAO, by using element array, size is "numVerticesGround", starting from vertex 0.

    // Draw skybox
    skyBox.pipelineProgram->Bind();
    skyBox.pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    skyBox.pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
    skyBox.vao->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyBox.texHandle);
    glDrawElements(GL_TRIANGLES, skyBox.numElements, GL_UNSIGNED_INT, 0); // Render the VAO, by using element array, size is "numVerticesGround", starting from vertex 0.


    // At the end, generate new u and other vector.
    calculateNewCameraRoller();
    
    // Swap the double-buffers.
    glutSwapBuffers();
}

void subdivideDrawSpline(double u0, double u1, double maxLengthSquare, glm::mat4x3 multMatrix, unsigned char depth) {
    // To make sure the depth of recursion not so deep.
    if (depth >= 30) return;

    glm::vec4 vu0(u0 * u0 * u0, u0 * u0, u0, 1.0f);
    glm::vec4 vu1(u1 * u1 * u1, u1 * u1, u1, 1.0f);
    glm::vec3 x0 = multMatrix * vu0;
    glm::vec3 x1 = multMatrix * vu1;

    // Calculate the square of the different between x0 and x1
    double squareSum = glm::length2((x0-x1));
    if (squareSum > maxLengthSquare) {
        double umid = (u0 + u1) / 2.0;
        subdivideDrawSpline(u0, umid, maxLengthSquare, multMatrix, depth + 1);
        subdivideDrawSpline(umid, u1, maxLengthSquare, multMatrix, depth + 1);
    }
    else {
        // only save the left point of a line
        splinePoints.push_back(x0);
        uVec.push_back(u0);
    }
}

void initSpline() {
    // Initialize the Catmull-Rom Spline Matrix
    catmullMatrix = glm::mat4(-catmullS, 2 - catmullS, catmullS - 2, catmullS,
        2 * catmullS, catmullS - 3, 3 - 2 * catmullS, -catmullS,
        -catmullS, 0, catmullS, 0,
        0, 1, 0, 0);

    // Create the spline points
    mulMatrix.resize(spline.numControlPoints);
    // Draw numControlPoints points to make the spline circular
    for (int i = 0; i < spline.numControlPoints; i++) {
        glm::mat4x3 controlMatrix;
        for (int j = 0; j < 4; j++) {
            controlMatrix[j][0] = spline.points[(i + j) % spline.numControlPoints].x;
            controlMatrix[j][1] = spline.points[(i + j) % spline.numControlPoints].y;
            controlMatrix[j][2] = spline.points[(i + j) % spline.numControlPoints].z;
        }
        mulMatrix[i] = controlMatrix * catmullMatrix;
        uPosVec.push_back(splinePoints.size()); // add the current size of spline point.
        subdivideDrawSpline(0.0, 1.0, maxLength * maxLength, mulMatrix[i], 0);
    }
    
    // To make the whole line circular, add the first point to the last.
    mulMatrix.push_back(mulMatrix[0]);
    uPosVec.push_back(splinePoints.size());
    uVec.push_back(0.0f);
    splinePoints.push_back(splinePoints[0]);
    numVerticesSpline = splinePoints.size(); // This must be a global variable, so that we know how many vertices to render in glDrawArrays.

    cout << "There are " << numVerticesSpline << " vertices generated.\n";
    // Calculate initial tangent, normal, binormal.
    splineTangent.resize(splinePoints.size());
    splineBinormal.resize(splinePoints.size());
    splineNormal.resize(splinePoints.size());
    pointDistance.resize(splinePoints.size());

    splineTangent[0] = glm::normalize(glm::vec3(mulMatrix[0] * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
    splineBinormal[0] = glm::normalize(glm::cross(splineTangent[0], glm::vec3(0.0f, 1.0f, 0.0f))); // Initially looking up
    splineNormal[0] = glm::normalize(glm::cross(splineBinormal[0], splineTangent[0]));
    pointDistance[0]=0.0f;

    // Calculate the following tangent, normal, binormal, distance by camera movement
    for (int i = 1,splineCount=0; i < splinePoints.size(); i++) {
        if (uVec[i] == 0.0f) {
            splineCount++;
        }
        splineTangent[i] = glm::normalize(glm::vec3(mulMatrix[splineCount] * glm::vec4(3.0f * uVec[i] * uVec[i], 2.0f * uVec[i], 1.0f, 0.0f)));
        splineNormal[i] = glm::normalize(glm::cross(splineBinormal[i - 1], splineTangent[i]));
        splineBinormal[i] = glm::normalize(glm::cross(splineTangent[i], splineNormal[i]));
        pointDistance[i]=pointDistance[i-1]+glm::distance(splinePoints[i-1],splinePoints[i]);
    }

    splineLength = pointDistance[pointDistance.size() - 1];
    cout << "The total length of the roller coaster is " << pointDistance[pointDistance.size() - 1] << " m.\n";
}

// Initialize the double T-shape rail with standard normal
void initRailStandardDoubleT() {

    rail.pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    // Load and set up the pipeline program, including its shaders.
    if (rail.pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShaderRail.glsl", "fragmentShaderRail.glsl") != 0) {
        cout << "Failed to build the pipeline program Rail." << endl;
        throw 1;
    }
    cout << "Successfully built the pipeline program Rail." << endl;

    rail.numVertices = (numVerticesSpline - 1) * 64;
    rail.numElements = (numVerticesSpline - 1) * 96;
    float* positions = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 3);
    float* normals = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 3);
    float* texCoord = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 2);
    unsigned int* elements = (unsigned int*)malloc(rail.numElements * sizeof(unsigned int));
    if (positions == NULL || normals == NULL || texCoord == NULL || elements == NULL) {
        cout << "Standard Double-T memory allocation fail.\n";
        exit(0);
    }

    // Get the positions of all vertex
    float dxyPoint[9][2] = { 0.5f,0.5f,
        -0.5f,0.5f,
        -0.5f,0.25f,
        -0.1f,0.25f,
        -0.1f,-0.5f,
        0.1f,-0.5,
        0.1f,0.25f,
        0.5f,0.25f,
        0.5f,0.5f
    };
    for (int i = 0; i < 9; i++) dxyPoint[i][0] *= rail.widthT, dxyPoint[i][1] *= rail.heightT;
    unsigned int dxyElement[6] = { 0,1,2, 0,2,3 };
    float dxyCoord[4][2] = { {1.0f,0.0f},{1.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f} };

    // pos: position of "positions","normals"
    // posE: position of "elements"
    // posTC: position of "texCoord"
    for (int i = 0, pos = 0, posE = 0, posTC=0; i < numVerticesSpline - 1; i++) {
        glm::vec3 cur[4];
        glm::vec3 p1, p2;
        // Left T
        for (int j = 0; j < 8; j++) {
            p1 = splinePoints[i] - 0.5f * rail.width * splineBinormal[i];
            p2 = splinePoints[i + 1] - 0.5f * rail.width * splineBinormal[i + 1];
            cur[0] = p1 + dxyPoint[j][0] * splineBinormal[i] + dxyPoint[j][1] * splineNormal[i];
            cur[1] = p2 + dxyPoint[j][0] * splineBinormal[i + 1] + dxyPoint[j][1] * splineNormal[i + 1];
            cur[2] = p2 + dxyPoint[j + 1][0] * splineBinormal[i + 1] + dxyPoint[j + 1][1] * splineNormal[i + 1];
            cur[3] = p1 + dxyPoint[j + 1][0] * splineBinormal[i] + dxyPoint[j + 1][1] * splineNormal[i];
            glm::vec3 normal = glm::normalize(glm::cross(cur[1] - cur[0], cur[3] - cur[0]));
            for (int k1 = 0; k1 < 4; k1++) {
                for (int k2 = 0; k2 < 3; k2++,pos++) {
                    positions[pos] = cur[k1][k2];
                    normals[pos] = normal[k2];
                }
            }
            for (int k = 0; k < 4; k++) {
                texCoord[posTC++] = dxyCoord[k][0];
                texCoord[posTC++] = dxyCoord[k][1];
            }
            int st = i * 64 + j * 4;
            for (int k = 0; k < 6; k++) {
                elements[posE++] = st + dxyElement[k];
            }
        }
        // Right T
        for (int j = 0; j < 8; j++) {
            p1 = splinePoints[i] + 0.5f * rail.width * splineBinormal[i];
            p2 = splinePoints[i + 1] + 0.5f * rail.width * splineBinormal[i + 1];
            cur[0] = p1 + dxyPoint[j][0] * splineBinormal[i] + dxyPoint[j][1] * splineNormal[i];
            cur[1] = p2 + dxyPoint[j][0] * splineBinormal[i + 1] + dxyPoint[j][1] * splineNormal[i + 1];
            cur[2] = p2 + dxyPoint[j + 1][0] * splineBinormal[i + 1] + dxyPoint[j + 1][1] * splineNormal[i + 1];
            cur[3] = p1 + dxyPoint[j + 1][0] * splineBinormal[i] + dxyPoint[j + 1][1] * splineNormal[i];
            glm::vec3 normal = glm::normalize(glm::cross(cur[1] - cur[0], cur[3] - cur[0]));
            // Set positions and normals
            for (int k1 = 0; k1 < 4; k1++) {
                for (int k2 = 0; k2 < 3; k2++) {
                    positions[pos] = cur[k1][k2];
                    normals[pos] = normal[k2];
                    pos++;
                }
            }
            // Set elements
            int st = i * 64 + 32 + j * 4;
            for (int k = 0; k < 6; k++) {
                elements[posE++] = st + dxyElement[k];
            }
        }
    }

    rail.vao = new VAO();
    rail.vao->Bind();

    rail.vboVertices = new VBO(rail.numVertices, 3, positions, GL_STATIC_DRAW); // 3 values per position, usinng number of rail point
    rail.vboNormals = new VBO(rail.numVertices, 3, normals, GL_STATIC_DRAW); // 3 values per position, usinng number of rail point
    rail.vboTexCoord = new VBO(rail.numVertices, 2, texCoord, GL_STATIC_DRAW); // 2 values per position, usinng number of rail point

    rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboVertices, "position");
    rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboNormals, "normal");
    rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboTexCoord, "texCoord");
    rail.ebo = new EBO(rail.numElements, elements, GL_STATIC_DRAW); //Bind the EBO
    glGenTextures(1, &rail.texHandle);
    initTexture("metal.jpg", rail.texHandle);
    free(positions);
    free(elements);
    free(texCoord);
}

// Initialize the rail with pseudo normal
void initRailPseudo() {

    rail.pipelineProgram= new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    // Load and set up the pipeline program, including its shaders.
    if (rail.pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShaderRail.glsl", "fragmentShaderRail.glsl") != 0){
        cout << "Failed to build the pipeline program Rail." << endl;
        throw 1;
    }
    cout << "Successfully built the pipeline program Rail." << endl;

    rail.numVertices = numVerticesSpline * 4;
    rail.numElements = (numVerticesSpline - 1) * 24;
    float* positions = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 3);
    float* normals = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 3);
    unsigned int* elements = (unsigned int*)malloc(rail.numElements * sizeof(unsigned int));

    // Get the positions of all vertex
    int pos = 0;
    float dxyPoint[4][2] = { 0.5f*rail.widthT,0.5f * rail.heightT,
        -0.5f * rail.widthT,0.5f * rail.heightT,
        -0.5f * rail.widthT,-0.5f * rail.heightT,
        0.5f * rail.widthT,-0.5f * rail.heightT };
    for (int i = 0; i < numVerticesSpline;i++){
        for (int j = 0; j < 4; j++) {
            glm::vec3 cur = splinePoints[i] + dxyPoint[j][0] * splineBinormal[i]+ dxyPoint[j][1]*splineNormal[i];
            for (int k = 0; k < 3; k++) {
                positions[pos++] = cur[k];
            }
        }
    }

    int dxyNormal[8][3] = { {0,4,5}, {0,1,5},
        {1,5,6},{1,2,6},
        {2,6,7},{2,3,7},
        {3,7,4},{3,0,4} };

    // Calculate pseudoNormal
    for (int i = 0; i < numVerticesSpline - 1; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 3; k++) {
                glm::vec3 vP[3];
                for (int t1 = 0; t1 < 3; t1++) {
                    for (int t2 = 0; t2 < 3; t2++) {
                        vP[t1][t2] = positions[(i * 4 + dxyNormal[j][(k + t1) % 3]) * 3 + t2];
                    }
                }
                glm::vec3 vl = vP[0] - vP[1];
                glm::vec3 vr = vP[2] - vP[1];
                glm::vec3 partsum = glm::angle(vl, vr) * glm::cross(vr, vl);
                for (int p = 0, st = i * 4 + dxyNormal[j][(k + 1) % 3]; p < 3; p++) {
                    normals[st * 3 + p] += partsum[p];
                }
            }
        }
    }
    // Didn't fully calculate the first and last normal, so add it on.
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            normals[i * 3 + j] += normals[(numVerticesSpline - 1) * 4 * 3 + i * 3 + j];
            normals[(numVerticesSpline - 1) * 4 * 3 + i * 3 + j] = normals[i * 3 + j];
        }
    }
    // Normalize the normal
    for (int i = 0; i < rail.numVertices; i++) {
        float sum = 0;
        for (int j = 0; j < 3; j++) {
            sum += normals[i * 3 + j] * normals[i * 3 + j];
        }
        sum = sqrt(sum);
        for (int j = 0; j < 3; j++) {
            normals[i * 3 + j] /= sum;
        }
    }

    // Get the element array
    pos = 0;
    for (int i = 0; i < numVerticesSpline-1; i++){
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 3; k++) {
                elements[pos++] = i * 4 + dxyNormal[j][k];
            }
        }
    }

    rail.vao = new VAO();
    rail.vao->Bind();
    rail.vboVertices = new VBO(rail.numVertices, 3, positions, GL_STATIC_DRAW); // 3 values per position, usinng number of point
    rail.vboNormals = new VBO(rail.numVertices, 3, normals, GL_STATIC_DRAW); // 3 values per position, usinng number of vertex normal

    rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboVertices, "position");
    rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboNormals, "normal");
    rail.ebo = new EBO(rail.numElements, elements, GL_STATIC_DRAW); //Bind the EBO

    free(positions);
    free(elements);
}

// Initialize the rail tie
void initRailTie() {

    railTie.pipelineProgram = new PipelineProgram(); //Load and set up the pipeline program, including its shaders.
    // Load and set up the pipeline program, including its shaders.
    if (railTie.pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShaderRail.glsl", "fragmentShaderRail.glsl") != 0) {
        cout << "Failed to build the pipeline program Rail Tie." << endl;
        throw 1;
    }
    cout << "Successfully built the pipeline program Rail Tie." << endl;

    railTie.count = floor(splineLength / railTie.space);
    railTie.space = splineLength / (1.0f * railTie.count);
    railTie.numVertices = railTie.count * 24;
    railTie.numElements = railTie.count * 36;

    //rail.numVertices = (numVerticesSpline - 1) * 64;
    //rail.numElements = (numVerticesSpline - 1) * 96;
    //float* positions = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 3);
    //float* normals = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 3);
    //float* texCoord = (float*)malloc(rail.numVertices * sizeof(unsigned int) * 2);
    //unsigned int* elements = (unsigned int*)malloc(rail.numElements * sizeof(unsigned int));
    //if (positions == NULL || normals == NULL || texCoord == NULL || elements == NULL) {
    //    cout << "Standard Double-T memory allocation fail.\n";
    //    exit(0);
    //}

    //// Get the positions of all vertex
    //float dxyPoint[9][2] = { 0.25f,0.5f,
    //    -0.25f,0.5f,
    //    -0.25f,0.25f,
    //    -0.05f,0.25f,
    //    -0.05f,-0.5f,
    //    0.05f,-0.5,
    //    0.05f,0.25f,
    //    0.25f,0.25f,
    //    0.25f,0.5f
    //};
    //for (int i = 0; i < 9; i++) dxyPoint[i][0] *= rail.height, dxyPoint[i][1] *= rail.height;
    //unsigned int dxyElement[6] = { 0,1,2, 0,2,3 };
    //float dxyCoord[4][2] = { {1.0f,0.0f},{1.0f,1.0f},{0.0f,1.0f},{0.0f,0.0f} };

    //// pos: position of "positions","normals"
    //// posE: position of "elements"
    //// posTC: position of "texCoord"
    //for (int i = 0, pos = 0, posE = 0, posTC = 0; i < numVerticesSpline - 1; i++) {
    //    glm::vec3 cur[4];
    //    glm::vec3 p1, p2;
    //    // Left T
    //    for (int j = 0; j < 8; j++) {
    //        p1 = splinePoints[i] - 0.5f * rail.width * splineBinormal[i];
    //        p2 = splinePoints[i + 1] - 0.5f * rail.width * splineBinormal[i + 1];
    //        cur[0] = p1 + dxyPoint[j][0] * splineBinormal[i] + dxyPoint[j][1] * splineNormal[i];
    //        cur[1] = p2 + dxyPoint[j][0] * splineBinormal[i + 1] + dxyPoint[j][1] * splineNormal[i + 1];
    //        cur[2] = p2 + dxyPoint[j + 1][0] * splineBinormal[i + 1] + dxyPoint[j + 1][1] * splineNormal[i + 1];
    //        cur[3] = p1 + dxyPoint[j + 1][0] * splineBinormal[i] + dxyPoint[j + 1][1] * splineNormal[i];
    //        glm::vec3 normal = glm::normalize(glm::cross(cur[1] - cur[0], cur[3] - cur[0]));
    //        for (int k1 = 0; k1 < 4; k1++) {
    //            for (int k2 = 0; k2 < 3; k2++, pos++) {
    //                positions[pos] = cur[k1][k2];
    //                normals[pos] = normal[k2];
    //            }
    //        }
    //        for (int k = 0; k < 4; k++) {
    //            texCoord[posTC++] = dxyCoord[k][0];
    //            texCoord[posTC++] = dxyCoord[k][1];
    //        }
    //        int st = i * 64 + j * 4;
    //        for (int k = 0; k < 6; k++) {
    //            elements[posE++] = st + dxyElement[k];
    //        }
    //    }
    //    // Right T
    //    for (int j = 0; j < 8; j++) {
    //        p1 = splinePoints[i] + 0.5f * rail.width * splineBinormal[i];
    //        p2 = splinePoints[i + 1] + 0.5f * rail.width * splineBinormal[i + 1];
    //        cur[0] = p1 + dxyPoint[j][0] * splineBinormal[i] + dxyPoint[j][1] * splineNormal[i];
    //        cur[1] = p2 + dxyPoint[j][0] * splineBinormal[i + 1] + dxyPoint[j][1] * splineNormal[i + 1];
    //        cur[2] = p2 + dxyPoint[j + 1][0] * splineBinormal[i + 1] + dxyPoint[j + 1][1] * splineNormal[i + 1];
    //        cur[3] = p1 + dxyPoint[j + 1][0] * splineBinormal[i] + dxyPoint[j + 1][1] * splineNormal[i];
    //        glm::vec3 normal = glm::normalize(glm::cross(cur[1] - cur[0], cur[3] - cur[0]));
    //        // Set positions and normals
    //        for (int k1 = 0; k1 < 4; k1++) {
    //            for (int k2 = 0; k2 < 3; k2++) {
    //                positions[pos] = cur[k1][k2];
    //                normals[pos] = normal[k2];
    //                pos++;
    //            }
    //        }
    //        // Set elements
    //        int st = i * 64 + 32 + j * 4;
    //        for (int k = 0; k < 6; k++) {
    //            elements[posE++] = st + dxyElement[k];
    //        }
    //    }
    //}

    //rail.vao = new VAO();
    //rail.vao->Bind();

    //rail.vboVertices = new VBO(rail.numVertices, 3, positions, GL_STATIC_DRAW); // 3 values per position, usinng number of rail point
    //rail.vboNormals = new VBO(rail.numVertices, 3, normals, GL_STATIC_DRAW); // 3 values per position, usinng number of rail point
    //rail.vboTexCoord = new VBO(rail.numVertices, 2, texCoord, GL_STATIC_DRAW); // 2 values per position, usinng number of rail point

    //rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboVertices, "position");
    //rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboNormals, "normal");
    //rail.vao->ConnectPipelineProgramAndVBOAndShaderVariable(rail.pipelineProgram, rail.vboTexCoord, "texCoord");
    //rail.ebo = new EBO(rail.numElements, elements, GL_STATIC_DRAW); //Bind the EBO
    //glGenTextures(1, &rail.texHandle);
    //initTexture("metal.jpg", rail.texHandle);
    //free(positions);
    //free(elements);
    //free(texCoord);
}

// Initialize the ground
void initGround(){

    ground.pipelineProgram= new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    // Load and set up the pipeline program, including its shaders.
    if (ground.pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShaderGround.glsl", "fragmentShaderGround.glsl") != 0){
        cout << "Failed to build the pipeline program Ground." << endl;
        throw 1;
    }
    cout << "Successfully built the pipeline program Ground." << endl;

    ground.numVertices=6;
    ground.numElements = ground.numVertices;
    float width=200.0f,height=200.0f,tall=-4.0f;
    float texCoord[8]={0.0f,0.0f,100.0f,0.0f,100.0f,100.0f,0.0f,100.0f};
    float positions[12]={-width,tall,-height,
        width,tall,-height,
        width,tall,height,
        -width,tall,height};
    unsigned int elements[6]={0,1,2,0,2,3};
    
    ground.vao=new VAO();
    ground.vao->Bind();
    ground.vboVertices=new VBO(4, 3, positions, GL_STATIC_DRAW);
    ground.vboTexCoord=new VBO(4, 2, texCoord, GL_STATIC_DRAW);

    ground.ebo=new EBO(ground.numElements,elements,GL_STATIC_DRAW);
    ground.vao->ConnectPipelineProgramAndVBOAndShaderVariable(ground.pipelineProgram, ground.vboVertices, "position");
    ground.vao->ConnectPipelineProgramAndVBOAndShaderVariable(ground.pipelineProgram, ground.vboTexCoord, "texCoord");
    glGenTextures(1,&ground.texHandle);
    initTexture("grassGround.jpg", ground.texHandle);
}

void initSkybox() {

    skyBox.pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
    // Load and set up the pipeline program, including its shaders.
    if (skyBox.pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShaderSkybox.glsl", "fragmentShaderSkybox.glsl") != 0) {
        cout << "Failed to build the pipeline program SkyBox." << endl;
        throw 1;
    }
    cout << "Successfully built the pipeline program SkyBox." << endl;

    skyBox.numVertices =30;
    skyBox.numElements = skyBox.numVertices;
    float width = 150.0f, tall = 0.0f;
    float positions[42] = { width,width + tall,-width,width,width + tall,width,
        width,tall+width,-width,-width,tall + width,-width,-width,tall + width,width,width,tall + width,width,width,tall + width,-width,
        width,tall-width,-width,-width,tall - width,-width,-width,tall - width,width,width,tall - width,width,width,tall - width,-width,
        width,tall - width,-width ,width,tall - width,width
    };
    float texCoord[28] = { 0.25f,1.0f,0.5f,1.0f,
        0.0f,2.0f / 3.0f,0.25f,2.0f / 3.0f,0.5f,2.0f / 3.0f,0.75f,2.0f / 3.0f,1.0f,2.0f / 3.0f,
        0.0f,1.0f / 3.0f,0.25f,1.0f / 3.0f,0.5f,1.0f / 3.0f,0.75f,1.0f / 3.0f,1.0f,1.0f / 3.0f,
        0.25f,0.0f,0.5f,0.0f
    };
    unsigned int elements[36] = { 0,1,4,0,4,3,
        8,7,2,8,2,3,9,8,3,9,3,4,10,9,4,10,4,5,11,10,5,11,5,6,
        8,9,13,8,13,12
    };
    int posP = 0, posT = 0;

    skyBox.vao = new VAO();
    skyBox.vao->Bind();
    skyBox.vboVertices = new VBO(14, 3, positions, GL_STATIC_DRAW);
    skyBox.vboTexCoord = new VBO(14, 2, texCoord, GL_STATIC_DRAW);

    skyBox.ebo = new EBO(skyBox.numElements, elements, GL_STATIC_DRAW);
    skyBox.vao->ConnectPipelineProgramAndVBOAndShaderVariable(skyBox.pipelineProgram, skyBox.vboVertices, "position");
    skyBox.vao->ConnectPipelineProgramAndVBOAndShaderVariable(skyBox.pipelineProgram, skyBox.vboTexCoord, "texCoord");
    glGenTextures(1, &skyBox.texHandle);
    initTexture("skyBoxDessert.jpg", skyBox.texHandle);
}


// Initialize the roller coaster status and camera status
void setDefaultRollerCamera(){
    rollerPos = splinePoints[0];
    rollerTangent = cameraFocus = splineTangent[0];
    rollerNormal = cameraUp = splineNormal[0];

    // offset of camera
    cameraEye = rollerPos + 0.1f * cameraUp;

    // The speed is 0 at the begining
    rollerSpeed=0.0;
}

void initScene(int argc, char* argv[])
{
    // Set the background color.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

    // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
    glEnable(GL_DEPTH_TEST);

    initSpline();

    initRailStandardDoubleT();

    initRailTie();

    initGround();

    initSkybox();

    setDefaultRollerCamera();

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