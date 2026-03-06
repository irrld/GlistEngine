#version 450

layout(location = 0) in vec3 WorldPos;
layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform samplerCube environmentMap;

const float PI = 3.14159265359;

// Mirror binary digits about the decimal point
float radicalInverse_VdC(int bits) {
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
	bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
	bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
	return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(int i, int N) {
	return vec2(float(i) / float(N), radicalInverse_VdC(i));
}

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

void main() {
	vec3 normal = normalize(WorldPos);
	vec3 N = normal;

	vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, normal);
	up = cross(normal, right);

	const int SAMPLE_COUNT = 16384;
	float totalWeight = 0.0;
	for (int i = 0; i < SAMPLE_COUNT; ++i) {
		vec2 Xi = hammersley(i, SAMPLE_COUNT);
		vec3 H = importanceSampleGGX(Xi, N, 1.0);

		float NdotH = max(dot(N, H), 0.0);
		float D = 1.0 / PI;
		float pdf = (D * NdotH / (4.0)) + 0.0001;

		float resolution = 1024.0;
		float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
		float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

		float mipLevel = 0.5 * log2(saSample / saTexel);

		irradiance += textureLod(environmentMap, H, mipLevel).rgb * NdotH;
		totalWeight += NdotH;
	}
	irradiance = (PI * irradiance) / totalWeight;

	FragColor = vec4(irradiance, 1.0);
}
