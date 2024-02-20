# CSCI420-HW1-HeightFiedWithShaders

## Homework Website

https://viterbi-web.usc.edu/~jbarbic/cs420-s24/assign1/index.html

## Introduction
Height fields may be found in many applications of computer graphics. They are used to represent terrain in video games and simulations, and also often utilized to represent data in three dimensions. This assignment asks you to create a height field based on the data from an image which the user specifies at the command line, and to allow the user to manipulate the height field in three dimensions by rotating, translating, or scaling it. You also have to implement a vertex shader that performs smoothing of the geometry, and re-adjusts the geometry color. After the completion of your program, you will use it to create an animation. You will program the assignment using OpenGL's core profile.

This assignment is intended as a hands-on introduction to OpenGL and programming in three dimensions. It teaches the OpenGL's core profile and shader-based programming. The provided starter gives the functionality to initialize GLUT, read and write a JPEG image, handle mouse and keyboard input, and display one triangle to the screen. You must write code to handle camera transformations, transform the landscape (translate/rotate/scale), and render the heightfield. You must also write a shader to perform geometry smoothing and re-color the terrain accordingly. Please see the OpenGL Programming Guide for information, or OpenGL.org.

## Extra Credits



## Main Files Description
 - README.md - Introduction of the project
 - animations - Restore the JPEG frames required for animation
 - openGLHelper - Helper files for OpenGL
 - hw1/heightmap - File for the input images
 - hw1/hw1.cpp - Main opengl program for this homework
 - hw1/Makefile - Make file for MacOS and Linux.

## To run the program

### MacOS

For compile, enter the hw1 folder and make

    // Enter hw1
    cd hw1

    // make the file
    make

For only heightmap, excecute

    ./hw1 <heightmap_file>

For heightmap and enable texture mapping, excecute

    ./hw1 <heightmap_file> <texturemap_file>

Examples

    ./hw1 heightmap/Heightmap.jpg
    ./hw1 heightmap/Heightmap.jpg heightmap/Heightmap.jpg/color.jpg

### Windows

For windows, just open the hw1.sln with visual studio, and change the command arguments for custom input. The input is simillar to MacOS

## Instructions

- **Points Mode**: Press `1`
- **Lines Mode**: Press `2`
- **Triangles Mode**: Press `3`
- **Smooth Mode**: Press `4`. In this mode:
  - Press `+` to multiply "scale" by 2x
  - Press `-` to divide "scale" by 2x
  - Press `9` to multiply "exponent" by 2x
  - Press `0` to divide "exponent" by 2x

**Rotation**
- X and Y axes: Press and hold left mouse button and move the mouse.
- Z axis: Press and hold middle mouse button and move the mouse.

**Scaling**
- X and Y axes: Press and hold `Shift` + left mouse button and move.
- Z axis: Press and hold `Shift` + middle mouse button and move.

**Translation (Windows)**
- X and Y axes: Press and hold `Control` + left mouse button and move.
- Z axis: Press and hold `Control` + middle mouse button and move.

**Translation (MacOS)**
Press `t` to enable\disable the Translate mode
- X and Y axes: In translation mode, press and hold left mouse button and move.
- Z axis: In translation mode, press and hold middle mouse button and move.

## Instructions (For Extra Credit)

- **Smooth Mode With JetColorMap**: Press `5`
- **Triangles Mode with wireframe**: Press `6`
- **Texturemap the surface with an arbitrary image (If compatible)**: Press `7`
- **Texturemap the surface with an image with image at same size (If compatible)**: Press `&` (`shift`+`7`)
- **Reset the heightmap to initial view(Undo all the rotation, scaling and translation)**: Press `c`
- **Enable moving camera**: Press 'v'. In this mode:
  - 'w' to move the camera position ahead.
  - 's' to move the camera position backward.
  - 'a' to move the camera position left.
  - 'd' to move the camera position right.
  - Move the mouse to change the direction of the mouse (Look up, down, left, right).


## 8. Team Member
 - Shidong Zhang
 - Baijia Ye
 - Ziyue Feng
 - Changyue Su
