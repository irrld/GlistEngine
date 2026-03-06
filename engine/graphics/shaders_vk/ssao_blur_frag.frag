#version 450

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform sampler2D colorTexture;
layout(set = 1, binding = 2) uniform sampler2D aoTexture;

layout(std140, set = 1, binding = 0) uniform MaterialData {
	vec4 screenSizeAndClips; // xy = screenSize, z = nearClip, w = farClip
	vec4 ssaoParams;         // x = ssaoStrength, y = debugMode
	vec4 matSpecular;
	float matShininess;
	int _pad0;
	int _pad1;
	int _pad2;
};

float linearizeDepth(float d) {
	float nearClip = screenSizeAndClips.z;
	float farClip = screenSizeAndClips.w;
	return nearClip * farClip / (farClip - d * (farClip - nearClip));
}

void main() {
	vec2 texelSize = 1.0 / screenSizeAndClips.xy;
	vec3 fragColor = texture(colorTexture, TexCoords).rgb;

	// Depth is not available as a separate texture here, approximate using AO texture alpha
	// or skip bilateral filtering if depth texture is not bound
	float ssaoStrength = ssaoParams.x;
	int debugMode = int(ssaoParams.y);

	// Simple box blur on AO
	float aoSum = 0.0;
	float weightSum = 0.0;

	for (int x = -3; x <= 2; x++) {
		for (int y = -3; y <= 2; y++) {
			vec2 offset = vec2(float(x), float(y)) * texelSize;
			float sampleAO = texture(aoTexture, TexCoords + offset).r;
			float weight = 1.0;

			aoSum += sampleAO * weight;
			weightSum += weight;
		}
	}

	float ao = aoSum / weightSum;
	ao = mix(1.0, ao, ssaoStrength);

	if (debugMode > 0) {
		FragColor = vec4(vec3(ao), 1.0);
	} else {
		FragColor = vec4(fragColor * ao, 1.0);
	}
}
