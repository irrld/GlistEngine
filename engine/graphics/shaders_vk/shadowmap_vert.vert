#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in vec3 color;

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 projection; // lightMatrix stored here
} pc;

void main() {
	gl_Position = pc.projection * pc.model * vec4(aPos, 1.0);
}
