#version 450

layout(location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 projection;
} pc;

layout(std140, set = 1, binding = 0) uniform MaterialData {
	vec4 spriteColor;
	vec4 subPosScale;     // xy = subPos, zw = subScale
	vec4 _pad0;
	float _pad1;
	int isAlphaMasking;
	int isSubPart;
	int _pad2;
};

layout(location = 0) out vec2 TexCoords;

void main() {
	if (isSubPart != 0) {
		TexCoords = (vertex.zw + subPosScale.xy) / subPosScale.zw;
	} else {
		TexCoords = vertex.zw;
	}
	gl_Position = pc.projection * pc.model * vec4(vertex.xy, 0.0, 1.0);
}
