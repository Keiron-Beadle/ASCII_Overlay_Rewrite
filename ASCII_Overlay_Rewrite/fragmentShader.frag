#version 330 core 

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main(){
//    float samplered = texture(text,TexCoords).r;
//	float samplegreen = texture(text,TexCoords).g;
//	float sampleblue = texture(text,TexCoords).b;
//	float samplealpha = texture(text,TexCoords).w;
	float sampled = texture2D(text,TexCoords).r;
	FragColor = vec4(textColor, 1.0) * sampled;
//
//	if (samplered > 0){
//		FragColor=vec4(1.0,0.0,0.0,1.0);
//	}
//	else if (samplegreen > 0){
//		FragColor = vec4(0.0,1.0,0.0,1.0);
//	}
//	else if (sampleblue > 0){
//		FragColor = vec4(0.0,0.0,1.0,1.0);
//	}
//	else if (samplealpha > 0){
//		FragColor = vec4(1.0,0.5,0.2,1.0);
//	}
	//if (Coords.x < 2460.0 && Coords.x > 0.0 && Coords.y < 500.0)
	//FragColor = vec4(1.0,0.0,0.0,1.0);
}