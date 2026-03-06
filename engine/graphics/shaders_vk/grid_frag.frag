#version 450

layout(location = 0) in vec3 nearpoint;
layout(location = 1) in vec3 farpoint;
layout(location = 2) in mat4 fragview;
layout(location = 6) in mat4 fragprojection;

layout(location = 0) out vec4 outColor;

layout(std140, set = 1, binding = 0) uniform MaterialData {
	vec4 matAmbient;
	vec4 matDiffuse;
	vec4 matSpecular;
	float nearClip;     // mapped from "near"
	float farClip;      // mapped from "far"
	int _pad0;
	int _pad1;
};

// draw grid lines
vec4 grid(vec3 fragpos, float linespacing, bool drawaxis) {
	vec2 coord = fragpos.xz * linespacing;

	// use screen-space partial derivatives for anti-aliasing
	vec2 derivative = fwidth(coord);
	vec2 g = abs(fract(coord - 0.5) - 0.5) / derivative;
	float line = min(g.x, g.y);

	// default line color
	vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));

	// x axis color
	float minz = min(derivative.y, 1.0);
	if (fragpos.z > -0.1 * minz && fragpos.z < 0.1 * minz) {
		color.x = 1.0;
	}

	// z axis color
	float minx = min(derivative.x, 1.0);
	if (fragpos.x > -0.1 * minx && fragpos.x < 0.1 * minx) {
		color.z = 1.0;
	}

	return color;
}

// calculating depth to determine if the fragment is behind an object
float computeDepth(vec3 position) {
	vec4 clip_space_position = fragprojection * fragview * vec4(position.xyz, 1.0);
	return (clip_space_position.z / clip_space_position.w) * 0.5 + 0.5;
}

// using the linear depth for fading / fog effect
float computeLinearDepth(vec3 pos) {
	vec4 clip_space_position = fragprojection * fragview * vec4(pos.xyz, 1.0);
	float clip_space_depth = (clip_space_position.z / clip_space_position.w) * 2.0 - 1.0;
	float lineardepth = (nearClip * farClip * 2.0) / (farClip + nearClip - clip_space_depth * (farClip - nearClip));
	return lineardepth / farClip;
}

void main() {
	// intersection with the floor (y = 0)
	float t = -nearpoint.y / (farpoint.y - nearpoint.y);
	vec3 fragpos = nearpoint + t * (farpoint - nearpoint);

	gl_FragDepth = computeDepth(fragpos);

	float lineardepth = computeLinearDepth(fragpos);
	float fading = max(0.0, (0.5 - lineardepth));

	// draw the 2d grid
	outColor = (grid(fragpos, 10.0, true) + grid(fragpos, 1.0, true)) * float(t > 0.0);
	outColor.a *= fading;
}
