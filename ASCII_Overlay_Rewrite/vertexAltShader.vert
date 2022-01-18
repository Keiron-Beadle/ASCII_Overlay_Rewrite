#version 330 core

layout(location=0) in vec3 vPos;
layout(location=1) in vec2 vTexCoord;

out vec4 vColour;
out vec2 TexCoord;

void main(){
	gl_Position = vec4(vPos, 1.0);
	vColour = vec4(0.4,0.6,0.3,1.0);
	TexCoord = vTexCoord;
}