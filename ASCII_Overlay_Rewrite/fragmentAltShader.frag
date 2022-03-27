#version 330 core 

uniform sampler2D myTex;
in vec4 vColour;
in vec2 TexCoord;
out vec4 FragColor;

void main(){
    //vec4 sampled = texture(myTex,TexCoord);
    //FragColor = vec4(sampled.z,sampled.y,sampled.x,sampled.w);
    FragColor = vColour;

}