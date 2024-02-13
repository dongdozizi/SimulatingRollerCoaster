#version 150



in vec3 position;

in vec3 pleft;
in vec3 pright;
in vec3 pup;
in vec3 pdown;
in vec3 pcenter;

in vec4 color;
out vec4 col;

uniform int renderType;
uniform float exponent;
uniform float scale;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    // compute the transformed and projected vertex position (into gl_Position) 
    
    if(renderType==4){
        vec3 tmpPcenter=(pleft+pright+pup+pdown+pcenter)/5.0f;
        tmpPcenter.y=scale*pow(tmpPcenter.y,exponent);
        gl_Position = projectionMatrix * modelViewMatrix * vec4(tmpPcenter,1.0f);
    }
    else{
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
    }
    // compute the vertex color (into col)
    col = color;
}

