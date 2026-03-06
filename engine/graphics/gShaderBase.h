/*
 * gShaderBase.h
 *
 *  Created on: May 7, 2020
 *      Author: noyan
 *
 *  Refactored: Base/GL/Vk split
 */

#ifndef ENGINE_GRAPHICS_GSHADERBASE_H_
#define ENGINE_GRAPHICS_GSHADERBASE_H_

#include "gRenderObject.h"
#include <string>
#include <unordered_map>

class gShaderBase : public gRenderObject {
public:
	bool loaded;
	GLuint id;

protected:
	gShaderBase();
	~gShaderBase();

	enum class ShaderType {
		VERTEX,
		FRAGMENT,
		GEOMETRY
	};

	std::string preprocessShader(const std::string& shaderCode, std::unordered_map<std::string, std::string> defines);
	std::unordered_map<std::string, std::string> generateDefines(ShaderType type);
	void clearUniformCaches();

	std::string readFile(const std::string& path);

	std::unordered_map<std::string, GLint> uniformlocations;
	std::unordered_map<std::string, GLuint> ubos;

	mutable std::unordered_map<GLint, int> cachedints;
	mutable std::unordered_map<GLint, float> cachedfloats;
	mutable std::unordered_map<GLint, glm::vec2> cachedvec2s;
	mutable std::unordered_map<GLint, glm::vec3> cachedvec3s;
	mutable std::unordered_map<GLint, glm::vec4> cachedvec4s;
	mutable std::unordered_map<GLint, glm::mat2> cachedmat2s;
	mutable std::unordered_map<GLint, glm::mat3> cachedmat3s;
	mutable std::unordered_map<GLint, glm::mat4> cachedmat4s;
};

#endif /* ENGINE_GRAPHICS_GSHADERBASE_H_ */
