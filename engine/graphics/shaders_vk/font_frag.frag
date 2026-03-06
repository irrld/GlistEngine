#version 450

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(std140, set = 1, binding = 0) uniform MaterialData {
	vec4 matAmbient;      // textColor stored in rgb
	vec4 matDiffuse;
	vec4 matSpecular;
	float matShininess;
	int matUseDiffuseMap;
	int matUseSpecularMap;
	int matUseNormalMap;
};

layout(set = 1, binding = 1) uniform sampler2D text;

void main() {
	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
	FragColor = vec4(matAmbient.rgb, 1.0) * sampled;
}
