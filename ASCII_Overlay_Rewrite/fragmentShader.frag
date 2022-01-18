#version 330 core 

in vec2 TexCoords;
in vec4 Coords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main(){
	vec4 sampled = vec4(1.0,1.0,1.0, texture(text,TexCoords).r);
	FragColor = vec4(textColor, 1.0) * sampled;
	//if (Coords.x < 2460.0 && Coords.x > 0.0 && Coords.y < 500.0)
	//FragColor = vec4(1.0,0.0,0.0,1.0);
}