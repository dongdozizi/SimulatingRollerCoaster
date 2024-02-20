# CSCI420-HW1-HeightFiedWithShaders

## Homework Website

https://viterbi-web.usc.edu/~jbarbic/cs420-s24/assign1/index.html

## Introduction
Height fields may be found in many applications of computer graphics. They are used to represent terrain in video games and simulations, and also often utilized to represent data in three dimensions. This assignment asks you to create a height field based on the data from an image which the user specifies at the command line, and to allow the user to manipulate the height field in three dimensions by rotating, translating, or scaling it. You also have to implement a vertex shader that performs smoothing of the geometry, and re-adjusts the geometry color. After the completion of your program, you will use it to create an animation. You will program the assignment using OpenGL's core profile.

This assignment is intended as a hands-on introduction to OpenGL and programming in three dimensions. It teaches the OpenGL's core profile and shader-based programming. The provided starter gives the functionality to initialize GLUT, read and write a JPEG image, handle mouse and keyboard input, and display one triangle to the screen. You must write code to handle camera transformations, transform the landscape (translate/rotate/scale), and render the heightfield. You must also write a shader to perform geometry smoothing and re-color the terrain accordingly. Please see the OpenGL Programming Guide for information, or OpenGL.org.

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

- **Points Mode**: Press '1'
- **Lines Mode**: Press '2'
- **Triangles Mode**: Press '3'
- **Smooth Mode**: Press '4'. In this mode:
  - '+' to multiply "scale" by 2x
  - '-' to divide "scale" by 2x
  - '9' to multiply "exponent" by 2x
  - '0' to divide "exponent" by 2x

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
- Activate: Press `t` (do not hold).
  - X and Y axes: Press and hold left mouse button and move.
  - Z axis: Press and hold middle mouse button and move.

## Instructions (For Extra Credit)

- **Sca Mode**: Press `1` to activate.
- **Lines Mode**: Press `2` to activate.
- **Triangles Mode**: Press `3` to activate.

## Todo


## Create and Initialize a "graph" object

To create a graph of any type on host or device
    
    graph<weight_t, graph_view_t> somegraph;

    // weight_t: type of edge weight
    // graph_view_t: "coo_d" to store graph on GPU, "coo_h" to store graph on CPU

For expample, you can create a graph whose edge weight is of type "int" on GPU by

    graph<int, coo_d> g; 


To initialize a graph  

    void init(size_t* v_list,         // vertex list
              size_t v_num,           // number of vertex list
              size_t* row_idx,        // row index (source of each edge)
              size_t* col_idx,        // col index (target of each edge)
              weight_t* value,        // weight for each edge
              size_t e_num,           // number of edges
              size_t number_of_blocks,  // girdsize for kernel launch
              size_t threads_per_block, // blocksize for kernel launch
              size_t MAX);             // max memory limit

If you put the graph in the host, the number_of_blocks and threads_per_block can be any number and not influence the result.

For example, you can initialize a graph like this 

    g.init( {1,2,3,4,5,6,8},    // vertex list     
            7,                  // 7 vertices in total
            {1,1,4,3,5},        // sources of each edge
            {2,3,5,4,6},        // target of each edge
            {5,7,2,4,6},        // weight of each edge
            5,                  // 5 edges in total
            50,                 // 50 blocks
            256,                // 256 threads per block
            10);                // 10 vertices at most


### Change the graph
To insert a new vertex
    
    bool insert_vertex(size_t vertex);

    // if vertex is not in the graph, insert it and return true
    // otherwise, vertex is already in the graph, return false

To insert a new edge

    bool insert_edge(size_t row,
                    size_t col,
                    weight_t value);

    // if vertex row and col are already in the graph and edge row->col is not in the graph, insert it and return true
    // otherwise, return false

To delete a vertex

    bool delete_vertex(size_t x);

    // if the vertex is in the graph, delete it and all related edges, and return true
    // otherwise, return false

To delete an edge

    bool delete_edge(size_t row_h,
                    size_t col_h);

    // if edge is already in the graph, delete it and return true
    // otherwise, return false


### Change the Configuration of Kernel Launch
    void modify_config(size_t number_of_blocks,
                        size_t threads_per_block);

If the graph is on host, calling this method will not influence anything.


### Get Information of the Graph
To print the graph 

    void print();

To list the information, like number of edges, of graph 

    void print_config();

To get the number of vertices in the graph

    size_t get_number_of_vertices();

To get the number of edges in the graph

    size_t get_number_of_edges();

To check whether a vertex is in the graph

    bool check_vertex(size_t vertex) ;

    // return true if vertex a is in the graph

To check whether an edge is in the graph

    bool check_edge(size_t row, size_t col);

    // return true if row -> col is in the graph

To get the weight of an edge

    weight_t get_weight(size_t row, size_t col, weight_t not_found);

    // if row -> col is in the graph, return its weight
    // otherwise, return not_found

To get number of neighbors of a vertex

    size_t get_num_neighbors(size_t x);

    // return 0 if vertex is not in the graph 

To get in-degree of a vertex

    size_t get_in_degree(size_t vertex);

    // return 0 if vertex is not in the graph 
    
To get out-degree of a vertex

    size_t get_out_degree(size_t vertex);

    // return 0 if vertex is not in the graph 

To get the list of in-neightbors of a vertex 

    std::vector<size_t> get_source_vertex(size_t vertex);

    // return empty vector if vertex is not in the graph

To get the list of out-neighbors of a vertex 

    std::vector<size_t> get_destination_vertex(size_t vertex);

    // return empty vector if vertex is not in the graph


## 4. Evironment Setting

###Demo code (exp.cu)

Compile and run the code

    nvcc -o exp exp.cu && ./exp // run the cuda code

The output should be shown as below which is the configuration you should input respectively

    GPU(0)\CPU(1) | MAX_V | v_num | e_num | test_times | MAX | grid_size | block_size

- GPU(0)\CPU(1) - To run it on GPU should input 0, on CPU should input 1
- MAX_V - The maximum vertex index in the graph (no vertex can larger than MAX_V)
- v_num - The number of vertices for initial graph
- e_num - The number of edges for initial graph
- test_times - The number of testing each funcitons
- MAX - The memory limit for graph
- grid_size - grid size when choosing GPU
- block_size - block size when choosing GPU

For example if you want to run a GPU graph on a initialized 10000 vertices 1000000 edges and want test the functions 1000 times, the maximum number of vertices is 20000 and memory limit is 2000000 with grid size 80 and block size 256. You should input

    0 20000 10000 1000000 1000 2000000 80 256

The sample result should be like

    ----------------------------Test begin------------------------------------
    Graph initialzed in GPU, grid size: 80, block size: 256
    initialization: 0.40 s
    insert 1000 vertices: 0.01 s
    insert 1000 edges: 0.03 s
    check 1000 edges: 0.02 s
    check 1000 vertices: 0.01 s
    get weight of 1000 edges: 0.03 s
    get in degree of 1000 vertices: 0.04 s
    get out degree of 1000 vertices: 0.05 s
    get num of neighbors of 1000 vertices: 0.10 s
    get source of 1000 vertices: 0.09 s
    get destination of 1000 vertices: 0.10 s
    delete 1000 edges: 0.02 s
    delete 1000 vertices: 0.04 s
    Total time: 0.94 s
    ----------------------------Test end---------------------------------------


Our code could be compiled with cuda-11.4 and gcc-4.8.5 and run correctly on the NYU cims server - cuda1, cuda3, cuda4, and cuda5. HOWEVER, could not test our code on cuda2 because someone have occupied the memory of the GPU on cuda2 and not used the computation resources for more than 4 DAYS!

<img src='some_is_sleeping_on_cuda2.jpg' width='600'>
<img src='person_sleeping_on_cuda2.png' width='600'>


## 5. To Do
- [x] Finish header file for GPU version
- [x] Fill the gap 
- [x] Finish the CPU version
- [x] Evaluate the correctness
- [x] Compare performance of GPU and CPU version
- [x] Finish README
- [x] Finish final report

## 6. Project Report
[final report](https://github.com/fzyyy0601/gpu-final-project-ds/blob/main/Project8_GPU_Final_Report.pdf)

## 7. Deadline: 12.13
one report, source code and readme.txt


## 8. Team Member
 - Shidong Zhang
 - Baijia Ye
 - Ziyue Feng
 - Changyue Su
