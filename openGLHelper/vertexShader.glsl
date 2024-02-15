#version 150

in vec3 position;
in vec3 pleft;
in vec3 pright;
in vec3 pup;
in vec3 pdown;
in vec3 pcenter;

in vec4 color;
in vec2 texCoord;

out vec4 col;
out vec2 coord;

uniform int renderType;
uniform float exponent;
uniform float scale;
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void JetColorMap(in float x, out vec4 tmpColor)
{
    float a; //alpha

    if (x < 0)
    {
        tmpColor[0] = 0;
        tmpColor[1] = 0;
        tmpColor[2] = 0;
        tmpColor[3] = 0;
        return;
    }
    else if (x < 0.125) 
    {
        a = x / 0.125;
        tmpColor[0] = 0;
        tmpColor[1] = 0;
        tmpColor[2] = 0.5 + 0.5 * a;
        tmpColor[3] = a;
        return;
    }
    else if (x < 0.375)
    {
        a = (x - 0.125) / 0.25;
        tmpColor[0] = 0;
        tmpColor[1] = a;
        tmpColor[2] = 1;
        tmpColor[3] = a;
        return;
    }
    else if (x < 0.625)
    {
        a = (x - 0.375) / 0.25;
        tmpColor[0] = a;
        tmpColor[1] = 1;
        tmpColor[2] = 1 - a;
        tmpColor[3] = a;
        return;
    }
    else if (x < 0.875)
    {
        a = (x - 0.625) / 0.25;
        tmpColor[0] = 1;
        tmpColor[1] = 1 - a;
        tmpColor[2] = 0;
        tmpColor[3] = a;
        return;
    }
    else if (x <= 1.0)
    {
        a = (x - 0.875) / 0.125;
        tmpColor[0] = 1 - 0.5 * a;
        tmpColor[1] = 0;
        tmpColor[2] = 0;
        tmpColor[3] = a;
        return;
    }
    else
    {
        tmpColor[0] = 1;
        tmpColor[1] = 1;
        tmpColor[2] = 1;
        tmpColor[3] = 1;
        return;
    }
}

void myJetcolor(in float x,out vec4 tmpColor){
    float a; //alpha

    if (x < 0.3)
    {
        a=x/0.3;
        tmpColor[0]=(0.0+(173.0f-0.0f)*a)/255.f;
        tmpColor[1]=(0.0f+(158.0f-0.0f)*a)/255.0f;
        tmpColor[2]=(0.0f+(85.0f-0.0f)*a)/255.0f;
        tmpColor[3]=1.0f;
        return;
    }
    else if(x<0.6){
        a=(x-0.3)/0.3;
        tmpColor[0]=(173.0f-(173.0f-110.0f)*a)/255.f;
        tmpColor[1]=(158.0f-(158.0f-139.0f)*a)/255.0f;
        tmpColor[2]=(85.0f-(85.0f-116.0f)*a)/255.0f;
        tmpColor[3]=1.0f;
        return;
    }
    else{
        a=(x-0.6)/0.6;
        tmpColor[0]=(110.0f-(110.0f-21.0f)*a)/255.0f;
        tmpColor[1]=(139.0f-(139.0f-35.0f)*a)/255.0f;
        tmpColor[2]=(116.0f-(116.0f-27.0f)*a)/255.0f;
        tmpColor[3]=1.0f;
        return;
    }

}

void main()
{
    // compute the transformed and projected vertex position (into gl_Position) 

    if(renderType==4){
        vec3 tmpPcenter=(pleft+pright+pup+pdown+pcenter)/5.0f;
        tmpPcenter.y=pow(tmpPcenter.y,exponent);
        col=vec4(tmpPcenter.y,tmpPcenter.y,tmpPcenter.y,1.0f);
        tmpPcenter.y=scale*tmpPcenter.y;
        gl_Position = projectionMatrix * modelViewMatrix * vec4(tmpPcenter,1.0f);
    }
    else if(renderType==5){
        vec3 tmpPcenter=(pleft+pright+pup+pdown+pcenter)/5.0f;
        tmpPcenter.y=pow(tmpPcenter.y,exponent);
        JetColorMap(tmpPcenter.y,col);
        tmpPcenter.y=scale*tmpPcenter.y;
        gl_Position = projectionMatrix * modelViewMatrix * vec4(tmpPcenter,1.0f);
    }
    else if(renderType==70){
        //myJetcolor(position.y,col);
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
        coord=texCoord;
    }
    else{
        //myJetcolor(position.y,col);
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
        // compute the vertex color (into col)
        col = color;
    }
}

