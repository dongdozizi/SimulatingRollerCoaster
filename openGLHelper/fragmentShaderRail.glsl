#version 150

in vec3 viewPosition;
in vec3 viewNormal;
in vec2 coord;

out vec4 c; // output color

uniform vec4 La; // light ambient
uniform vec4 Ld; // light diffuse
uniform vec4 Ls; // light specular
uniform vec3 viewLightDirection;

uniform vec4 ka; // mesh ambient
uniform vec4 kd; // mesh diffuse
uniform vec4 ks; // mesh specular
uniform float alpha; // shininess

uniform sampler2D textureImg; // Texture image

void main(){ 
	// camera is at (0,0,0) after the modelview transformation
	vec3 eyedir = normalize(vec3(0.0f, 0.0f, 0.0f) - viewPosition);
	// reflected light direction
	vec3 reflectDir = -reflect(viewLightDirection, viewNormal);
	// Phong lighting
	float d = max(dot(viewLightDirection, viewNormal), 0.0f);
	float s = max(dot(reflectDir, eyedir), 0.0f);
	// compute the final color
	//c=vec4(viewNormal,1.0);
	c = ka * La + d * kd * Ld + pow(s, alpha) * ks * Ls;
	// Add texture
	c=c*texture(textureImg,coord);
}

