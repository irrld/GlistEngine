#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in vec3 color;

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 projection;
} pc;

struct Fog {
	vec3 color;
	float linearStart;
	float linearEnd;
	float density;
	float gradient;
	int mode;
};

layout(std140, set = 0, binding = 1) uniform Scene {
	vec4 renderColor;
	vec3 viewPos;
	mat4 viewMatrix;
	int flags;
	Fog fog;
};

layout(std140, set = 1, binding = 0) uniform MaterialData {
	vec4 matAmbient;
	vec4 matDiffuse;
	vec4 matSpecular;
	float matShininess;
	int matUseDiffuseMap;
	int matUseSpecularMap;
	int matUseNormalMap;
	vec4 matRenderColor;
	mat4 matViewMatrix;
};

layout(location = 0) out vec2 TexCoords;

void main() {
	TexCoords = aTexCoords;
	gl_Position = pc.projection * matViewMatrix * pc.model * vec4(aPos, 1.0);
}
