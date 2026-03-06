/*
 * gShaderVk.h
 *
 *  Created on: May 7, 2020
 *      Author: noyan
 *
 *  Refactored: Vulkan implementation
 */

#ifndef ENGINE_GRAPHICS_VK_GSHADERVK_H_
#define ENGINE_GRAPHICS_VK_GSHADERVK_H_

#if defined(GLIST_VULKAN)

#include "gShaderBase.h"

class gVulkanRenderEngine;

class gShaderVk : public gShaderBase {
public:
	gShaderVk();
	gShaderVk(const std::string& shaderFullPath);
	gShaderVk(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath = "");
	~gShaderVk();

	void load(const std::string& shaderFullPath);
	void load(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath = "");

	void loadShader(const std::string& shaderFilename);
	void loadShader(const std::string& vertexFileName, const std::string& fragmentFileName, const std::string& geometryFileName = "");

	void loadProgram(const std::string& vertexSource, const std::string& fragmentSource, const std::string& geometrySource = "");
	void loadProgram(const std::string& shaderSource);

	template<typename T>
	void attachUbo(const std::string& uboName, const gUbo<T>* ubo) {
		ubos[uboName] = ubo->getBindingPoint();
		// No-op in Vulkan — descriptor layout is fixed
	}

	void use() const;
	void setBool(const std::string& name, bool value);
	void setInt(const std::string& name, int value);
	void setUnsignedInt(const std::string& name, unsigned int value);
	void setFloat(const std::string& name, float value);
	void setVec2(const std::string& name, const glm::vec2& value);
	void setVec2(const std::string& name, float x, float y);
	void setVec3(const std::string& name, const glm::vec3& value);
	void setVec3(const std::string& name, float x, float y, float z);
	void setVec4(const std::string& name, const glm::vec4& value);
	void setVec4(const std::string& name, float x, float y, float z, float w);
	void setMat2(const std::string& name, const glm::mat2& mat);
	void setMat3(const std::string& name, const glm::mat3& mat);
	void setMat4(const std::string& name, const glm::mat4& mat);

	GLint getUniformLocation(const std::string& name);

private:
	void writeToTarget(uint32_t loc, const void* data, size_t size);
	void destroyProgram();

	gVulkanRenderEngine* engine;
	uint32_t programindex;
};

#endif // GLIST_VULKAN
#endif /* ENGINE_GRAPHICS_VK_GSHADERVK_H_ */
