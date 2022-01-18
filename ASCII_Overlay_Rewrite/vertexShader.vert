#version 330 core

layout(location=0) in vec4 vertex;
out vec2 TexCoords;
out vec4 Coords;

uniform mat4 projection;

void main(){
	gl_Position = projection * vec4(vertex.xy, 0.0,1.0);
	TexCoords = vertex.zw;
	Coords = vertex;
}