#version 450

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

layout(location = 0) out vec3 nearpoint;
layout(location = 1) out vec3 farpoint;
layout(location = 2) out mat4 fragview;
layout(location = 6) out mat4 fragprojection;

// grid positions are in xy clipped space
vec3 gridplane[6] = vec3[] (
	vec3( 1,  1, 0),
	vec3(-1, -1, 0),
	vec3(-1,  1, 0),
	vec3(-1, -1, 0),
	vec3( 1,  1, 0),
	vec3( 1, -1, 0)
);

// unproject the plane coordinates so that the points are always at infinity
vec3 unprojectPoint(float x, float y, float z, mat4 view, mat4 proj) {
	mat4 viewinv = inverse(view);
	mat4 projinv = inverse(proj);
	vec4 unprojectedpoint = viewinv * projinv * vec4(x, y, z, 1.0);
	return unprojectedpoint.xyz / unprojectedpoint.w;
}

void main() {
	vec3 point = gridplane[gl_VertexIndex].xyz;
	nearpoint = unprojectPoint(point.x, point.y, 0.0, matViewMatrix, pc.projection).xyz;
	farpoint = unprojectPoint(point.x, point.y, 1.0, matViewMatrix, pc.projection).xyz;
	fragview = matViewMatrix;
	fragprojection = pc.projection;
	gl_Position = vec4(point, 1.0);
}
