#version 450

#ifndef GLIST_MAX_LIGHTS
#define GLIST_MAX_LIGHTS 8
#endif

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

struct Fog {
	vec3 color;
	float linearStart;
	float linearEnd;
	float density;
	float gradient;
	int mode;
};

layout(std140, set = 0, binding = 0) uniform Lights {
	int lightnum;
	int enabledlights;
	vec4 globalambientcolor;
	Light lights[GLIST_MAX_LIGHTS];
};

layout(std140, set = 0, binding = 1) uniform Scene {
	vec4 renderColor;
	vec3 viewPos;
	mat4 viewMatrix;
	int flags;
	Fog fog;
};

layout(push_constant) uniform PushConstants {
	mat4 model;
	mat4 projection;
} pc;

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
	vec4 matViewPos;
};

layout(set = 1, binding = 1) uniform sampler2D diffusemap;
layout(set = 1, binding = 2) uniform sampler2D specularmap;

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 FragPos;
layout(location = 2) in vec2 TexCoords;
layout(location = 3) in vec4 EyePosition;
layout(location = 4) in vec3 incolor;

layout(location = 0) out vec4 FragColor;

int ENABLE_FOG_FLAG = 1 << 1;
int ENABLE_GAMMA_FLAG = 1 << 2;
int ENABLE_HDR_FLAG = 1 << 3;

vec4 calcAmbLight(Light light, vec4 materialAmbient) {
	return light.ambient * materialAmbient;
}

vec4 calcDirLight(Light light, vec3 normal, vec3 viewDir, vec4 mAmbient, vec4 mDiffuse, vec4 mSpecular) {
	vec3 lightDir = normalize(-light.direction);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), matShininess);
	vec4 ambient = light.ambient * mAmbient;
	vec4 diffuse = light.diffuse * vec4(diff) * mDiffuse;
	vec4 specular = light.specular * vec4(spec) * mSpecular;
	return ambient + diffuse + specular;
}

vec4 calcPointLight(Light light, vec3 normal, vec3 viewDir, vec4 mAmbient, vec4 mDiffuse, vec4 mSpecular) {
	vec3 lightDir = normalize(light.position - FragPos);
	float distance = length(light.position - FragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), matShininess);
	vec4 ambient = light.ambient * mAmbient;
	vec4 diffuse = light.diffuse * diff * mDiffuse;
	vec4 specular = light.specular * spec * mSpecular;
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	return ambient + diffuse + specular;
}

vec4 calcSpotLight(Light light, vec3 normal, vec3 viewDir, vec4 mAmbient, vec4 mDiffuse, vec4 mSpecular) {
	vec3 lightDir = normalize(light.position - FragPos);
	float distance = length(light.position - FragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), matShininess);
	vec4 ambient = light.ambient * mAmbient;
	vec4 diffuse = light.diffuse * diff * mDiffuse;
	vec4 specular = light.specular * spec * mSpecular;
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.cutOff - light.outerCutOff;
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
	ambient *= attenuation;
	diffuse *= attenuation * intensity;
	specular *= attenuation * intensity;
	return ambient + diffuse + specular;
}

float getFogVisibility(float distance) {
	float visibility = 0.0;
	if (fog.mode == 0) { // linear
		float fogLength = fog.linearEnd - fog.linearStart;
		visibility = (fog.linearEnd - distance) / fogLength;
	} else if (fog.mode == 1) { // exp
		visibility = exp(-pow((distance * fog.density), fog.gradient));
	}
	return clamp(visibility, 0.0, 1.0);
}

void main() {
	vec4 result = vec4(0.0);
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(matViewPos.xyz - FragPos);

	vec4 materialAmbient;
	vec4 materialDiffuse;
	if (matUseDiffuseMap > 0) {
		materialAmbient = texture(diffusemap, TexCoords).rgba;
		materialDiffuse = texture(diffusemap, TexCoords).rgba;
	} else {
		materialAmbient = matAmbient;
		materialDiffuse = matDiffuse;
	}
	if (matUseDiffuseMap > 0 && materialDiffuse.a < 0.5) {
		discard;
	}
	vec4 materialSpecular;
	if (matUseSpecularMap > 0) {
		materialSpecular = texture(specularmap, TexCoords).rgba;
	} else {
		materialSpecular = matSpecular;
	}

	bool haslight = false;
	for (int i = 0; i < lightnum; i++) {
		if ((enabledlights & (1 << i)) == 0) {
			continue;
		}
		if (lights[i].type == 0) {
			result += calcAmbLight(lights[i], materialAmbient);
		} else if (lights[i].type == 1) {
			result += calcDirLight(lights[i], norm, viewDir, materialAmbient, materialDiffuse, materialSpecular);
		} else if (lights[i].type == 2) {
			result += calcPointLight(lights[i], norm, viewDir, materialAmbient, materialDiffuse, materialSpecular);
		} else if (lights[i].type == 3) {
			result += calcSpotLight(lights[i], norm, viewDir, materialAmbient, materialDiffuse, materialSpecular);
		}
		haslight = true;
	}
	if (!haslight) {
		result = globalambientcolor * materialAmbient;
	}

	FragColor = result * matRenderColor * vec4(incolor, 1.0);

	if ((flags & ENABLE_FOG_FLAG) > 0) {
		float distance = abs(EyePosition.z / EyePosition.w);
		FragColor = mix(vec4(fog.color, 1.0), FragColor, getFogVisibility(distance));
	}

	// HDR tone mapping (Reinhard)
	if ((flags & ENABLE_HDR_FLAG) > 0) {
		vec3 hdrColor = FragColor.rgb;
		vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
		FragColor = vec4(mapped, FragColor.a);
	}

	// Gamma correction
	if ((flags & ENABLE_GAMMA_FLAG) > 0) {
		float gamma = 2.2;
		FragColor.rgb = pow(FragColor.rgb, vec3(1.0 / gamma));
	}
}
