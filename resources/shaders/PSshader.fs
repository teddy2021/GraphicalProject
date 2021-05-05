#version 330 core

in vec2 UV;

layout(location=0)out vec4 color;

uniform sampler2D Texture;


void main(){
		color = texture(Texture, UV);
}
