#version 450

layout(location = 0) in vec4 vertex; // <vec2 position (or vec3), vec2 texCoords>

layout(location = 0) out vec2 TexCoords;

void main() {
	TexCoords = vertex.zw;
	gl_Position = vec4(vertex.xyz, 1.0);
}
