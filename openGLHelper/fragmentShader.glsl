#version 150

in vec2 coord;
in vec4 col;
out vec4 c;

uniform int renderType;
uniform sampler2D textureImg;

void main()
{
    // compute the final pixel color
    if(renderType==70){
        c=texture(textureImg,coord);
    }
    else{
        c=col;
    }
}

