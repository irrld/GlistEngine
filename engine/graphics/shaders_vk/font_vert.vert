#version 450

layout(location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 projection;
} pc;

layout(location = 0) out vec2 TexCoords;

void main() {
	gl_Position = pc.projection * vec4(vertex.xy, 0.0, 1.0);
	TexCoords = vertex.zw;
}
