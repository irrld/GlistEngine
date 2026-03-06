/*
 * gShaderVk.cpp
 *
 *  Created on: May 7, 2020
 *      Author: noyan
 *
 *  Refactored: Vulkan implementation
 */

#if defined(GLIST_VULKAN)

#include "gShaderVk.h"
#include "gVulkanRenderEngine.h"

#include <cstring>
#include <cstddef>
#include <cassert>

gShaderVk::gShaderVk() : programindex(0) {
	engine = static_cast<gVulkanRenderEngine*>(renderer);
}

gShaderVk::gShaderVk(const std::string& shaderFullPath) : programindex(0) {
	engine = static_cast<gVulkanRenderEngine*>(renderer);
	load(shaderFullPath);
}

gShaderVk::gShaderVk(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath)
	: programindex(0) {
	engine = static_cast<gVulkanRenderEngine*>(renderer);
	load(vertexFullPath, fragmentFullPath, geometryFullPath);
}

gShaderVk::~gShaderVk() {
	destroyProgram();
}

void gShaderVk::destroyProgram() {
	if (programindex != 0) {
		VkDevice device = engine->getVkDevice();
		auto& sp = engine->getShaderProgram(programindex);
		if (sp.vertmodule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device, sp.vertmodule, nullptr);
			sp.vertmodule = VK_NULL_HANDLE;
		}
		if (sp.fragmodule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device, sp.fragmodule, nullptr);
			sp.fragmodule = VK_NULL_HANDLE;
		}
		programindex = 0;
	}
}

void gShaderVk::loadShader(const std::string& vertexFileName, const std::string& fragmentFileName, const std::string& geometryFileName) {
	load(gGetShadersDir() + vertexFileName, gGetShadersDir() + fragmentFileName, gGetShadersDir() + geometryFileName);
}

void gShaderVk::loadShader(const std::string& shaderFileName) {
	load(gGetShadersDir() + shaderFileName);
}

void gShaderVk::load(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath) {
	if (loaded) {
		destroyProgram();
		id = 0;
		loaded = false;
	}
	std::string vertexCode = readFile(vertexFullPath);
	std::string fragmentCode = readFile(fragmentFullPath);
	std::string geometryCode;
	if (!geometryFullPath.empty()) {
		geometryCode = readFile(geometryFullPath);
	}
	loadProgram(vertexCode, fragmentCode, geometryCode);
}

void gShaderVk::load(const std::string& shaderFullPath) {
	if (loaded) {
		destroyProgram();
		id = 0;
		loaded = false;
	}
	std::string shaderCode = readFile(shaderFullPath);
	loadProgram(shaderCode);
}

void gShaderVk::loadProgram(const std::string& vertexSource, const std::string& fragmentSource, const std::string& geometrySource) {
	const std::string preprocessedVertexSource = preprocessShader(vertexSource, generateDefines(ShaderType::VERTEX));
	const std::string preprocessedFragmentSource = preprocessShader(fragmentSource, generateDefines(ShaderType::FRAGMENT));

	gVulkanRenderEngine::VulkanShaderProgram sp;

	std::string vertid = engine->parseVkIdentifier(preprocessedVertexSource.c_str());
	std::string fragid = engine->parseVkIdentifier(preprocessedFragmentSource.c_str());

	sp.vertid = vertid;
	sp.fragid = fragid;

	const char* spirvdata = nullptr;
	size_t spirvsize = 0;

	if (!vertid.empty() && engine->getSpirvData(vertid, &spirvdata, &spirvsize)) {
		sp.vertmodule = engine->createShaderModule(spirvdata, spirvsize);
	} else {
		gLogi("gShaderVk") << "No SPIR-V found for vertex shader: " << vertid;
	}

	if (!fragid.empty() && engine->getSpirvData(fragid, &spirvdata, &spirvsize)) {
		sp.fragmodule = engine->createShaderModule(spirvdata, spirvsize);
	} else {
		gLogi("gShaderVk") << "No SPIR-V found for fragment shader: " << fragid;
	}

	programindex = engine->addShaderProgram(sp);
	id = programindex;
	loaded = true;
	clearUniformCaches();
}

void gShaderVk::loadProgram(const std::string& shaderSource) {
	loadProgram(shaderSource, shaderSource, "");
}

void gShaderVk::use() const {
#ifdef DEBUG
	assert(loaded);
#endif
	engine->setActiveShaderProgram(programindex);
	engine->getMaterialData() = gVulkanRenderEngine::MaterialUBOData{};
	// Clear value caches — all shaders share the same push constant and UBO staging data
	cachedints.clear();
	cachedfloats.clear();
	cachedvec2s.clear();
	cachedvec3s.clear();
	cachedvec4s.clear();
	cachedmat2s.clear();
	cachedmat3s.clear();
	cachedmat4s.clear();
}

void gShaderVk::writeToTarget(uint32_t loc, const void* data, size_t size) {
	uint32_t target = loc & gVulkanRenderEngine::ULOC_TARGET_MASK;
	uint32_t offset = loc & gVulkanRenderEngine::ULOC_OFFSET_MASK;

	if (target == gVulkanRenderEngine::ULOC_IGNORE) {
		return;
	}

	if (target == gVulkanRenderEngine::ULOC_PUSH_CONST) {
		if (offset + size <= gVulkanRenderEngine::PUSH_CONSTANT_SIZE) {
			memcpy(engine->getPushConstantData() + offset, data, size);
		}
	} else if (target == gVulkanRenderEngine::ULOC_MATERIAL) {
		if (offset + size <= sizeof(gVulkanRenderEngine::MaterialUBOData)) {
			memcpy(reinterpret_cast<uint8_t*>(&engine->getMaterialData()) + offset, data, size);
		}
	}
}

void gShaderVk::setBool(const std::string& name, bool value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	int intval = value ? 1 : 0;
	auto it = cachedints.find(loc);
	if (it != cachedints.end() && it->second == intval) return;
	cachedints[loc] = intval;
	writeToTarget(loc, &intval, sizeof(intval));
}

void gShaderVk::setInt(const std::string& name, int value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedints.find(loc);
	if (it != cachedints.end() && it->second == value) return;
	cachedints[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setUnsignedInt(const std::string& name, unsigned int value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	int intval = static_cast<int>(value);
	auto it = cachedints.find(loc);
	if (it != cachedints.end() && it->second == intval) return;
	cachedints[loc] = intval;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setFloat(const std::string& name, float value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedfloats.find(loc);
	if (it != cachedfloats.end() && it->second == value) return;
	cachedfloats[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setVec2(const std::string& name, const glm::vec2& value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedvec2s.find(loc);
	if (it != cachedvec2s.end() && it->second == value) return;
	cachedvec2s[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setVec2(const std::string& name, float x, float y) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	glm::vec2 value(x, y);
	auto it = cachedvec2s.find(loc);
	if (it != cachedvec2s.end() && it->second == value) return;
	cachedvec2s[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setVec3(const std::string& name, const glm::vec3& value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedvec3s.find(loc);
	if (it != cachedvec3s.end() && it->second == value) return;
	cachedvec3s[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setVec3(const std::string& name, float x, float y, float z) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	glm::vec3 value(x, y, z);
	auto it = cachedvec3s.find(loc);
	if (it != cachedvec3s.end() && it->second == value) return;
	cachedvec3s[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setVec4(const std::string& name, const glm::vec4& value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedvec4s.find(loc);
	if (it != cachedvec4s.end() && it->second == value) return;
	cachedvec4s[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setVec4(const std::string& name, float x, float y, float z, float w) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	glm::vec4 value(x, y, z, w);
	auto it = cachedvec4s.find(loc);
	if (it != cachedvec4s.end() && it->second == value) return;
	cachedvec4s[loc] = value;
	writeToTarget(loc, &value, sizeof(value));
}

void gShaderVk::setMat2(const std::string& name, const glm::mat2& mat) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedmat2s.find(loc);
	if (it != cachedmat2s.end() && it->second == mat) return;
	cachedmat2s[loc] = mat;
	writeToTarget(loc, &mat, sizeof(mat));
}

void gShaderVk::setMat3(const std::string& name, const glm::mat3& mat) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedmat3s.find(loc);
	if (it != cachedmat3s.end() && it->second == mat) return;
	cachedmat3s[loc] = mat;
	writeToTarget(loc, &mat, sizeof(mat));
}

void gShaderVk::setMat4(const std::string& name, const glm::mat4& mat) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedmat4s.find(loc);
	if (it != cachedmat4s.end() && it->second == mat) return;
	cachedmat4s[loc] = mat;
	writeToTarget(loc, &mat, sizeof(mat));
}

GLint gShaderVk::getUniformLocation(const std::string& name) {
	auto it = uniformlocations.find(name);
	if (it != uniformlocations.end()) {
		return it->second;
	}

	// Push constant offsets
	if (name == "model") { uniformlocations[name] = gVulkanRenderEngine::ULOC_PUSH_CONST | 0; return uniformlocations[name]; }
	if (name == "projection") { uniformlocations[name] = gVulkanRenderEngine::ULOC_PUSH_CONST | 64; return uniformlocations[name]; }
	if (name == "invProjection") { uniformlocations[name] = gVulkanRenderEngine::ULOC_PUSH_CONST | 0; return uniformlocations[name]; }
	if (name == "lightMatrix") { uniformlocations[name] = gVulkanRenderEngine::ULOC_PUSH_CONST | 64; return uniformlocations[name]; }

	// Material UBO fields
	if (name == "material.ambient") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, ambient)); return uniformlocations[name]; }
	if (name == "material.diffuse") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, diffuse)); return uniformlocations[name]; }
	if (name == "material.specular") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, specular)); return uniformlocations[name]; }
	if (name == "material.shininess") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, shininess)); return uniformlocations[name]; }
	if (name == "material.useDiffuseMap") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usediffusemap)); return uniformlocations[name]; }
	if (name == "material.useSpecularMap") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usespecularmap)); return uniformlocations[name]; }
	if (name == "material.useNormalMap") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usenormalmap)); return uniformlocations[name]; }

	// Image shader uniforms
	if (name == "spriteColor") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, ambient)); return uniformlocations[name]; }
	if (name == "isAlphaMasking") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usediffusemap)); return uniformlocations[name]; }
	if (name == "isSubPart") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usespecularmap)); return uniformlocations[name]; }
	if (name == "subPos") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, diffuse)); return uniformlocations[name]; }
	if (name == "subScale") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | (static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, diffuse)) + 8); return uniformlocations[name]; }

	// Font shader
	if (name == "textColor") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, ambient)); return uniformlocations[name]; }

	// Grid shader
	if (name == "near") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, shininess)); return uniformlocations[name]; }
	if (name == "far") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usediffusemap)); return uniformlocations[name]; }

	// Skybox shader
	if (name == "aIsHDR") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usediffusemap)); return uniformlocations[name]; }

	// Prefilter shader
	if (name == "roughness") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, shininess)); return uniformlocations[name]; }

	// PBR shader flags
	if (name == "hasAlbedoMap") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usediffusemap)); return uniformlocations[name]; }
	if (name == "hasNormalMap") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usespecularmap)); return uniformlocations[name]; }
	if (name == "hasMetallicMap") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, usenormalmap)); return uniformlocations[name]; }

	// SSAO uniforms
	if (name == "screenSize") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, ambient)); return uniformlocations[name]; }
	if (name == "nearClip") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | (static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, ambient)) + 8); return uniformlocations[name]; }
	if (name == "farClip") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | (static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, ambient)) + 12); return uniformlocations[name]; }
	if (name == "ssaoRadius") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, diffuse)); return uniformlocations[name]; }
	if (name == "ssaoBias") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | (static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, diffuse)) + 4); return uniformlocations[name]; }
	if (name == "ssaoStrength") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, diffuse)); return uniformlocations[name]; }
	if (name == "debugMode") { uniformlocations[name] = gVulkanRenderEngine::ULOC_MATERIAL | (static_cast<uint32_t>(offsetof(gVulkanRenderEngine::MaterialUBOData, diffuse)) + 4); return uniformlocations[name]; }

	// SSAO kernel samples — handled procedurally in the shader
	if (name.size() >= 8 && name.substr(0, 8) == "samples[") {
		uniformlocations[name] = gVulkanRenderEngine::ULOC_IGNORE;
		return gVulkanRenderEngine::ULOC_IGNORE;
	}

	// All ignored uniforms (view, camPos, texture samplers, etc.)
	uniformlocations[name] = gVulkanRenderEngine::ULOC_IGNORE;
	return gVulkanRenderEngine::ULOC_IGNORE;
}

#endif // GLIST_VULKAN
