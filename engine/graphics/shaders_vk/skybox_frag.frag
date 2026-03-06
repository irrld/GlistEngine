#version 450

layout(location = 0) in vec3 TexCoords;
layout(location = 1) flat in int mIsHDR;

layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform samplerCube skymap;

void main() {
	vec4 fc;
	if (mIsHDR == 0) {
		fc = texture(skymap, TexCoords);
	} else if (mIsHDR == 1) {
		vec3 envColor = textureLod(skymap, TexCoords, 0.0).rgb;
		envColor = envColor / (envColor + vec3(1.0));
		envColor = pow(envColor, vec3(1.0 / 2.2));
		fc = vec4(envColor, 1.0);
	} else {
		fc = vec4(1.0, 0.0, 0.0, 1.0);
	}
	FragColor = fc;
}
