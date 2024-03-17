#version 150

in vec2 coord;
out vec4 c;

uniform sampler2D textureImg;

void main()
{
    // compute the final pixel color
    c=texture(textureImg,coord);
}

