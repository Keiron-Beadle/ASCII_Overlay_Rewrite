#version 330 core

layout(location=0) in vec4 vertex;

out vec4 vColour;
out vec2 TexCoord;

void main(){
	gl_Position = vec4(vertex.xy, 0.0, 1.0);
	vColour = vec4(0.4,0.6,0.3,1.0);
	TexCoord = vertex.zw;
}