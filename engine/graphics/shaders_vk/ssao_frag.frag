#version 450

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform sampler2D colorTexture;
layout(set = 1, binding = 2) uniform sampler2D depthTexture;

layout(push_constant) uniform PushConstants {
	mat4 invProjection;  // model slot reused as invProjection
	mat4 projection;
} pc;

layout(std140, set = 1, binding = 0) uniform MaterialData {
	vec4 screenSizeAndClips; // xy = screenSize, z = nearClip, w = farClip
	vec4 ssaoParams;         // x = ssaoRadius, y = ssaoBias
	vec4 matSpecular;
	float matShininess;
	int _pad0;
	int _pad1;
	int _pad2;
};

// Kernel samples passed via Scene UBO or as constants
// For now, use a fixed Hammersley-based kernel
const int KERNEL_SIZE = 32;

// Simple hash for procedural kernel generation
vec3 getKernelSample(int i) {
	float fi = float(i);
	float x = fract(sin(fi * 12.9898) * 43758.5453) * 2.0 - 1.0;
	float y = fract(sin(fi * 78.233) * 43758.5453) * 2.0 - 1.0;
	float z = fract(sin(fi * 45.164) * 43758.5453);
	vec3 s = normalize(vec3(x, y, z));
	s *= fract(sin(fi * 93.9898) * 43758.5453);
	float scale = fi / float(KERNEL_SIZE);
	scale = 0.1 + scale * scale * (1.0 - 0.1);
	return s * scale;
}

vec3 getViewPos(vec2 uv) {
	float depth = texture(depthTexture, uv).r;
	vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 viewPos = pc.invProjection * clipPos;
	return viewPos.xyz / viewPos.w;
}

float linearizeDepth(float d) {
	float nearClip = screenSizeAndClips.z;
	float farClip = screenSizeAndClips.w;
	return nearClip * farClip / (farClip - d * (farClip - nearClip));
}

vec3 reconstructNormal(vec3 posC, vec2 uv) {
	vec2 texelSize = 1.0 / screenSizeAndClips.xy;

	vec3 posL = getViewPos(uv - vec2(texelSize.x, 0.0));
	vec3 posR = getViewPos(uv + vec2(texelSize.x, 0.0));
	vec3 posT = getViewPos(uv - vec2(0.0, texelSize.y));
	vec3 posB = getViewPos(uv + vec2(0.0, texelSize.y));

	float wL = 1.0 / (abs(posL.z - posC.z) + 0.0001);
	float wR = 1.0 / (abs(posR.z - posC.z) + 0.0001);
	float wT = 1.0 / (abs(posT.z - posC.z) + 0.0001);
	float wB = 1.0 / (abs(posB.z - posC.z) + 0.0001);

	vec3 dx = (wR * (posR - posC) + wL * (posC - posL)) / (wR + wL);
	vec3 dy = (wB * (posB - posC) + wT * (posC - posT)) / (wT + wB);

	vec3 normal = normalize(cross(dy, dx));
	if (dot(normal, normalize(-posC)) < 0.0) {
		normal = -normal;
	}
	return normal;
}

void main() {
	float depth = texture(depthTexture, TexCoords).r;

	if (depth >= 1.0) {
		FragColor = vec4(1.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 fragPos = getViewPos(TexCoords);
	vec3 normal = reconstructNormal(fragPos, TexCoords);

	vec2 noiseScale = screenSizeAndClips.xy / 4.0;
	// Use a simple noise based on screen position
	float noiseAngle = fract(sin(dot(floor(TexCoords * noiseScale), vec2(12.9898, 78.233))) * 43758.5453) * 6.2831853;
	vec3 randomVec = normalize(vec3(cos(noiseAngle), sin(noiseAngle), 0.0));

	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float ssaoRadius = ssaoParams.x;
	float ssaoBias = ssaoParams.y;

	float occlusion = 0.0;
	int validSamples = 0;
	float fragLinearDepth = linearizeDepth(depth);

	for (int i = 0; i < KERNEL_SIZE; i++) {
		vec3 samplePos = TBN * getKernelSample(i);
		samplePos = fragPos + samplePos * ssaoRadius;

		vec4 offset = pc.projection * vec4(samplePos, 1.0);
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5 + 0.5;

		if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) {
			continue;
		}

		validSamples++;

		vec3 geometryPos = getViewPos(offset.xy);
		vec3 diff = geometryPos - fragPos;

		float heightAbovePlane = dot(diff, normal);

		float sampleRawDepth = texture(depthTexture, offset.xy).r;
		float sampleLinearDepth = linearizeDepth(sampleRawDepth);
		float depthDiff = abs(fragLinearDepth - sampleLinearDepth);
		float rangeCheck = smoothstep(0.0, 1.0, ssaoRadius / (depthDiff + 0.0001));

		occlusion += smoothstep(0.0, ssaoBias * 2.0, heightAbovePlane) * rangeCheck;
	}

	float ao = 1.0;
	if (validSamples > 0) {
		ao = 1.0 - (occlusion / float(validSamples));
	}

	ao = clamp(ao, 0.0, 1.0);

	FragColor = vec4(ao, 0.0, 0.0, 1.0);
}
