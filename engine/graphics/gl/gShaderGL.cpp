/*
 * gShaderGL.cpp
 *
 *  Created on: May 7, 2020
 *      Author: noyan
 *
 *  Refactored: OpenGL implementation
 */

#include "gShaderGL.h"
#include <cassert>

GLuint gShaderGL::currentprogram = 0;

gShaderGL::gShaderGL() : programid(0) {
}

gShaderGL::gShaderGL(const std::string& shaderFullPath) : programid(0) {
	load(shaderFullPath);
}

gShaderGL::gShaderGL(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath)
	: programid(0) {
	load(vertexFullPath, fragmentFullPath, geometryFullPath);
}

gShaderGL::~gShaderGL() {
	destroyProgram();
}

void gShaderGL::destroyProgram() {
	if (programid != 0) {
		if (currentprogram == programid) currentprogram = 0;
		G_CHECK_GL(glDeleteProgram(programid));
		programid = 0;
	}
}

void gShaderGL::loadShader(const std::string& vertexFileName, const std::string& fragmentFileName, const std::string& geometryFileName) {
	load(gGetShadersDir() + vertexFileName, gGetShadersDir() + fragmentFileName, gGetShadersDir() + geometryFileName);
}

void gShaderGL::loadShader(const std::string& shaderFileName) {
	load(gGetShadersDir() + shaderFileName);
}

void gShaderGL::load(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath) {
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

void gShaderGL::load(const std::string& shaderFullPath) {
	if (loaded) {
		destroyProgram();
		id = 0;
		loaded = false;
	}
	std::string shaderCode = readFile(shaderFullPath);
	loadProgram(shaderCode);
}

void gShaderGL::loadProgram(const std::string& vertexSource, const std::string& fragmentSource, const std::string& geometrySource) {
	const std::string preprocessedVertexSource = preprocessShader(vertexSource, generateDefines(ShaderType::VERTEX));
	const std::string preprocessedFragmentSource = preprocessShader(fragmentSource, generateDefines(ShaderType::FRAGMENT));

	const char* vSrc = preprocessedVertexSource.c_str();
	const char* fSrc = preprocessedFragmentSource.c_str();
	const char* gSrc = nullptr;

#if defined(WIN32) || defined(LINUX)
	const std::string preprocessedGeometrySource = preprocessShader(geometrySource, generateDefines(ShaderType::GEOMETRY));
	if (!preprocessedGeometrySource.empty()) {
		gSrc = preprocessedGeometrySource.c_str();
	}
#endif

	// Compile and link
	unsigned int vertex = GL_NONE;
	unsigned int fragment = GL_NONE;
#if defined(WIN32) || defined(LINUX)
	unsigned int geometry = GL_NONE;
#endif

	G_CHECK_GL2(vertex, glCreateShader(GL_VERTEX_SHADER));
	G_CHECK_GL(glShaderSource(vertex, 1, &vSrc, nullptr));
	G_CHECK_GL(glCompileShader(vertex));
	checkCompileErrors(vertex, "VERTEX");

	G_CHECK_GL2(fragment, glCreateShader(GL_FRAGMENT_SHADER));
	G_CHECK_GL(glShaderSource(fragment, 1, &fSrc, nullptr));
	G_CHECK_GL(glCompileShader(fragment));
	checkCompileErrors(fragment, "FRAGMENT");

	G_CHECK_GL2(programid, glCreateProgram());
	G_CHECK_GL(glAttachShader(programid, vertex));
	G_CHECK_GL(glAttachShader(programid, fragment));

#if defined(WIN32) || defined(LINUX)
	if (gSrc != nullptr) {
		G_CHECK_GL2(geometry, glCreateShader(GL_GEOMETRY_SHADER));
		if (geometry != 0) {
			G_CHECK_GL(glShaderSource(geometry, 1, &gSrc, nullptr));
			G_CHECK_GL(glCompileShader(geometry));
			checkCompileErrors(geometry, "GEOMETRY");
			G_CHECK_GL(glAttachShader(programid, geometry));
		} else {
			gLoge("Geometry Shader") << "Failed to create geometry shader: not supported by this context";
		}
	}
#endif

	G_CHECK_GL(glLinkProgram(programid));
	checkCompileErrors(programid, "PROGRAM");

	G_CHECK_GL(glDeleteShader(vertex));
	G_CHECK_GL(glDeleteShader(fragment));
#if defined(WIN32) || defined(LINUX)
	if (gSrc != nullptr) G_CHECK_GL(glDeleteShader(geometry));
#endif

	id = programid;
	loaded = true;
	clearUniformCaches();
}

void gShaderGL::loadProgram(const std::string& shaderSource) {
	const std::string preprocessedVertexSource = preprocessShader(shaderSource, generateDefines(ShaderType::VERTEX));
	const std::string preprocessedFragmentSource = preprocessShader(shaderSource, generateDefines(ShaderType::FRAGMENT));

	const char* vSrc = preprocessedVertexSource.c_str();
	const char* fSrc = preprocessedFragmentSource.c_str();
	const char* gSrc = nullptr;

#if defined(WIN32) || defined(LINUX)
	const std::string preprocessedGeometrySource = preprocessShader(shaderSource, generateDefines(ShaderType::GEOMETRY));
	if (!preprocessedGeometrySource.empty()) {
		gSrc = preprocessedGeometrySource.c_str();
	}
#endif

	unsigned int vertex = GL_NONE;
	unsigned int fragment = GL_NONE;
#if defined(WIN32) || defined(LINUX)
	unsigned int geometry = GL_NONE;
#endif

	G_CHECK_GL2(vertex, glCreateShader(GL_VERTEX_SHADER));
	G_CHECK_GL(glShaderSource(vertex, 1, &vSrc, nullptr));
	G_CHECK_GL(glCompileShader(vertex));
	checkCompileErrors(vertex, "VERTEX");

	G_CHECK_GL2(fragment, glCreateShader(GL_FRAGMENT_SHADER));
	G_CHECK_GL(glShaderSource(fragment, 1, &fSrc, nullptr));
	G_CHECK_GL(glCompileShader(fragment));
	checkCompileErrors(fragment, "FRAGMENT");

	G_CHECK_GL2(programid, glCreateProgram());
	G_CHECK_GL(glAttachShader(programid, vertex));
	G_CHECK_GL(glAttachShader(programid, fragment));

#if defined(WIN32) || defined(LINUX)
	if (gSrc != nullptr) {
		G_CHECK_GL2(geometry, glCreateShader(GL_GEOMETRY_SHADER));
		if (geometry != 0) {
			G_CHECK_GL(glShaderSource(geometry, 1, &gSrc, nullptr));
			G_CHECK_GL(glCompileShader(geometry));
			checkCompileErrors(geometry, "GEOMETRY");
			G_CHECK_GL(glAttachShader(programid, geometry));
		} else {
			gLoge("Geometry Shader") << "Failed to create geometry shader: not supported by this context";
		}
	}
#endif

	G_CHECK_GL(glLinkProgram(programid));
	checkCompileErrors(programid, "PROGRAM");

	G_CHECK_GL(glDeleteShader(vertex));
	G_CHECK_GL(glDeleteShader(fragment));
#if defined(WIN32) || defined(LINUX)
	if (gSrc != nullptr) G_CHECK_GL(glDeleteShader(geometry));
#endif

	id = programid;
	loaded = true;
	clearUniformCaches();
}

void gShaderGL::checkCompileErrors(GLuint shader, const std::string& type) {
	GLint success;
	GLchar infoLog[1024];
	if (type != "PROGRAM") {
		G_CHECK_GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
		if (!success) {
			G_CHECK_GL(glGetShaderInfoLog(shader, 1024, nullptr, infoLog));
			gLoge("gShader") << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog <<
					"\n -- --------------------------------------------------- -- " << std::endl;
		}
	} else {
		G_CHECK_GL(glGetProgramiv(shader, GL_LINK_STATUS, &success));
		if (!success) {
			G_CHECK_GL(glGetProgramInfoLog(shader, 1024, nullptr, infoLog));
			gLoge("gShader") << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog <<
					"\n -- --------------------------------------------------- -- " << std::endl;
		}
	}
#ifdef DEBUG
	assert(success);
#endif
}

void gShaderGL::use() const {
#ifdef DEBUG
	assert(loaded);
#endif
	if (currentprogram == programid) return;
	currentprogram = programid;
	G_CHECK_GL(glUseProgram(programid));
}

void gShaderGL::setBool(const std::string& name, bool value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	int intval = (int)value;
	auto it = cachedints.find(loc);
	if (it != cachedints.end() && it->second == intval) return;
	cachedints[loc] = intval;
	G_CHECK_GL(glUniform1i(loc, intval));
}

void gShaderGL::setInt(const std::string& name, int value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedints.find(loc);
	if (it != cachedints.end() && it->second == value) return;
	cachedints[loc] = value;
	G_CHECK_GL(glUniform1i(loc, value));
}

void gShaderGL::setUnsignedInt(const std::string& name, unsigned int value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	int intval = static_cast<int>(value);
	auto it = cachedints.find(loc);
	if (it != cachedints.end() && it->second == intval) return;
	cachedints[loc] = intval;
	G_CHECK_GL(glUniform1ui(loc, value));
}

void gShaderGL::setFloat(const std::string& name, float value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedfloats.find(loc);
	if (it != cachedfloats.end() && it->second == value) return;
	cachedfloats[loc] = value;
	G_CHECK_GL(glUniform1f(loc, value));
}

void gShaderGL::setVec2(const std::string& name, const glm::vec2& value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedvec2s.find(loc);
	if (it != cachedvec2s.end() && it->second == value) return;
	cachedvec2s[loc] = value;
	G_CHECK_GL(glUniform2fv(loc, 1, &value[0]));
}

void gShaderGL::setVec2(const std::string& name, float x, float y) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	glm::vec2 value(x, y);
	auto it = cachedvec2s.find(loc);
	if (it != cachedvec2s.end() && it->second == value) return;
	cachedvec2s[loc] = value;
	G_CHECK_GL(glUniform2f(loc, x, y));
}

void gShaderGL::setVec3(const std::string& name, const glm::vec3& value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedvec3s.find(loc);
	if (it != cachedvec3s.end() && it->second == value) return;
	cachedvec3s[loc] = value;
	G_CHECK_GL(glUniform3fv(loc, 1, &value[0]));
}

void gShaderGL::setVec3(const std::string& name, float x, float y, float z) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	glm::vec3 value(x, y, z);
	auto it = cachedvec3s.find(loc);
	if (it != cachedvec3s.end() && it->second == value) return;
	cachedvec3s[loc] = value;
	G_CHECK_GL(glUniform3f(loc, x, y, z));
}

void gShaderGL::setVec4(const std::string& name, const glm::vec4& value) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedvec4s.find(loc);
	if (it != cachedvec4s.end() && it->second == value) return;
	cachedvec4s[loc] = value;
	G_CHECK_GL(glUniform4fv(loc, 1, &value[0]));
}

void gShaderGL::setVec4(const std::string& name, float x, float y, float z, float w) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	glm::vec4 value(x, y, z, w);
	auto it = cachedvec4s.find(loc);
	if (it != cachedvec4s.end() && it->second == value) return;
	cachedvec4s[loc] = value;
	G_CHECK_GL(glUniform4f(loc, x, y, z, w));
}

void gShaderGL::setMat2(const std::string& name, const glm::mat2& mat) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedmat2s.find(loc);
	if (it != cachedmat2s.end() && it->second == mat) return;
	cachedmat2s[loc] = mat;
	G_CHECK_GL(glUniformMatrix2fv(loc, 1, GL_FALSE, &mat[0][0]));
}

void gShaderGL::setMat3(const std::string& name, const glm::mat3& mat) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedmat3s.find(loc);
	if (it != cachedmat3s.end() && it->second == mat) return;
	cachedmat3s[loc] = mat;
	G_CHECK_GL(glUniformMatrix3fv(loc, 1, GL_FALSE, &mat[0][0]));
}

void gShaderGL::setMat4(const std::string& name, const glm::mat4& mat) {
#ifdef DEBUG
	assert(loaded);
#endif
	GLint loc = getUniformLocation(name);
	auto it = cachedmat4s.find(loc);
	if (it != cachedmat4s.end() && it->second == mat) return;
	cachedmat4s[loc] = mat;
	G_CHECK_GL(glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]));
}

GLint gShaderGL::getUniformLocation(const std::string& name) {
	auto it = uniformlocations.find(name);
	if (it != uniformlocations.end()) {
		return it->second;
	}

	G_CHECK_GL2(GLuint location, glGetUniformLocation(programid, name.c_str()));
	uniformlocations[name] = location;
	return location;
}
