#version 450

#ifndef GLIST_MAX_LIGHTS
#define GLIST_MAX_LIGHTS 8
#endif

layout(location = 0) in vec2 TexCoords;
layout(location = 1) in vec3 WorldPos;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec4 FragColor;

struct Light {
	int type; // 0-ambient, 1-directional, 2-point, 3-spot
	vec3 position;
	vec3 direction;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float constant;
	float linear;
	float quadratic;
	float cutOff;
	float outerCutOff;
};

layout(std140, set = 0, binding = 0) uniform Lights {
	int lightnum;
	int enabledlights;
	vec4 globalambientcolor;
	Light lights[GLIST_MAX_LIGHTS];
};

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
	int hasAlbedoMap;
	int hasNormalMap;
	int hasMetallicMap;
	vec4 matRenderColor;
	mat4 matViewMatrix;
	vec4 matViewPos;
};

layout(set = 1, binding = 1) uniform sampler2D albedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;

const float PI = 3.14159265359;

vec3 getNormalFromMap() {
	vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

	vec3 Q1 = dFdx(WorldPos);
	vec3 Q2 = dFdy(WorldPos);
	vec2 st1 = dFdx(TexCoords);
	vec2 st2 = dFdy(TexCoords);

	vec3 N = normalize(Normal);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main() {
	// material properties - use defaults when maps aren't available
	vec3 albedo = hasAlbedoMap > 0 ? pow(texture(albedoMap, TexCoords).rgb, vec3(2.2)) : vec3(0.5);
	float metallic = 0.0;
	float roughness = 0.5;
	float ao = 1.0;

	// input lighting data
	vec3 N = hasNormalMap > 0 ? getNormalFromMap() : normalize(Normal);
	vec3 V = normalize(matViewPos.xyz - WorldPos);

	// calculate reflectance at normal incidence
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// reflectance equation
	vec3 Lo = vec3(0.0);
	vec3 ambientSum = vec3(0.0);
	for (int i = 0; i < lightnum; ++i) {
		if ((enabledlights & (1 << i)) == 0) {
			continue;
		}

		int lightType = lights[i].type;

		if (lightType == 0) {
			ambientSum += lights[i].ambient.rgb;
			continue;
		}

		vec3 L;
		vec3 radiance;

		if (lightType == 1) {
			L = normalize(-lights[i].direction);
			radiance = lights[i].diffuse.rgb * PI;
		} else {
			L = normalize(lights[i].position - WorldPos);
			float distance = length(lights[i].position - WorldPos);
			float attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * (distance * distance));
			radiance = lights[i].diffuse.rgb * PI * attenuation;

			if (lightType == 3) {
				float theta = dot(L, normalize(-lights[i].direction));
				float epsilon = lights[i].cutOff - lights[i].outerCutOff;
				float intensity = clamp((theta - lights[i].outerCutOff) / epsilon, 0.0, 1.0);
				radiance *= intensity;
			}
		}

		vec3 H = normalize(V + L);

		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		vec3 nominator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
		vec3 specular = nominator / denominator;

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		float NdotL = max(dot(N, L), 0.0);

		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 ambientLight = ambientSum;
	if (dot(ambientLight, ambientLight) < 0.001 && lightnum == 0) {
		ambientLight = globalambientcolor.rgb;
	}
	vec3 ambient = ambientLight * albedo * ao;

	vec3 finalcolor = ambient + Lo;

	// HDR tonemapping
	finalcolor = finalcolor / (finalcolor + vec3(1.0));
	// gamma correct
	finalcolor = pow(finalcolor, vec3(1.0 / 2.2));

	FragColor = vec4(finalcolor, 1.0);
}
