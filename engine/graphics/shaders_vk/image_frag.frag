#version 450

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

layout(std140, set = 1, binding = 0) uniform MaterialData {
	vec4 spriteColor;
	vec4 subPosScale;     // xy = subPos, zw = subScale
	vec4 _pad0;
	float _pad1;
	int isAlphaMasking;
	int isSubPart;
	int _pad2;
};

layout(set = 1, binding = 1) uniform sampler2D image;
layout(set = 1, binding = 2) uniform sampler2D maskimage;

void main() {
	FragColor = spriteColor * texture(image, TexCoords);
	if (isAlphaMasking != 0) {
		vec4 mask = texture(maskimage, TexCoords);
		mask.a = 1.0 - mask.r;
		mask.r = 1.0;
		mask.g = 1.0;
		mask.b = 1.0;
		FragColor *= mask;
	}
}
