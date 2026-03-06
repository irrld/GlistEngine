#version 450

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform sampler2D screenTexture;

void main() {
	FragColor = vec4(texture(screenTexture, TexCoords).rgb, 1.0);
}
