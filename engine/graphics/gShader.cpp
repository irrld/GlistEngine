/*
 * gShader.cpp
 *
 *  Created on: May 7, 2020
 *      Author: noyan
 */

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "gShader.h"
#include "gRenderer.h"


gShader::gShader() {
	id = 0;
	loaded = false;
}

gShader::~gShader() {
    if(loaded) {
        glDeleteShader(id);
    }
}

gShader::gShader(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath) {
	load(vertexFullPath, fragmentFullPath, geometryFullPath);
}

void gShader::loadShader(const std::string& vertexFileName, const std::string& fragmentFileName, const std::string& geometryFileName) {
	load(gGetShadersDir() + vertexFileName, gGetShadersDir() + fragmentFileName, gGetShadersDir() + geometryFileName);
}

void gShader::loadShader(const std::string& shaderFileName) {
	 load(gGetShadersDir() + shaderFileName);
}

void gShader::load(const std::string& vertexFullPath, const std::string& fragmentFullPath, const std::string& geometryFullPath) {
    if(loaded) {
        glDeleteShader(id);
    }
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try  {
        // open files
        vShaderFile.open(vertexFullPath);
        fShaderFile.open(fragmentFullPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        // if geometry shader path is present, also load a geometry shader
        geometryCode = "";
        if(geometryFullPath != "") {
            gShaderFile.open(geometryFullPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    loadProgram(vertexCode, fragmentCode, geometryCode);
}

void gShader::load(const std::string& shaderFullPath) {
	if(loaded) {
		glDeleteShader(id);
	}
	// 1. retrieve the vertex/fragment source code from filePath
	std::string shaderCode;
	std::ifstream vShaderFile;
	// ensure ifstream objects can throw exceptions:
	vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
	try  {
		// open files
		vShaderFile.open(shaderFullPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		// convert stream into string
		shaderCode = vShaderStream.str();
	} catch (std::ifstream::failure& e) {
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
	loadProgram(shaderCode);
}

void gShader::loadProgram(const std::string& vertexSource, const std::string& fragmentSource, const std::string& geometrySource) {
	const char* preprocessedVertexSourceStr = nullptr;
	const char* preprocessedFragmentSourceStr = nullptr;
	const char* preprocessedGeometrySourceStr = nullptr;

	// preprocess shaders
	const std::string preprocessedVertexSource = preprocessShader(vertexSource, generateDefines(ShaderType::VERTEX));
	preprocessedVertexSourceStr = preprocessedVertexSource.c_str();

	const std::string preprocessedFragmentSource = preprocessShader(fragmentSource, generateDefines(ShaderType::FRAGMENT));
	preprocessedFragmentSourceStr = preprocessedFragmentSource.c_str();

#if defined(WIN32) || defined(LINUX)
	const std::string preprocessedGeometrySource = preprocessShader(geometrySource, generateDefines(ShaderType::GEOMETRY));
	preprocessedGeometrySourceStr = preprocessedGeometrySource.c_str();
	if (preprocessedGeometrySource.empty()) {
		preprocessedGeometrySourceStr = nullptr;
	}
#endif

	// call the internal load function
	loadProgramInternal(preprocessedVertexSourceStr, preprocessedFragmentSourceStr, preprocessedGeometrySourceStr);
}

void gShader::loadProgram(const std::string& shaderSource) {
	const char* preprocessedVertexSourceStr = nullptr;
	const char* preprocessedFragmentSourceStr = nullptr;
	const char* preprocessedGeometrySourceStr = nullptr;

	// preprocess shaders
	const std::string preprocessedVertexSource = preprocessShader(shaderSource, generateDefines(ShaderType::VERTEX));
	preprocessedVertexSourceStr = preprocessedVertexSource.c_str();

	const std::string preprocessedFragmentSource = preprocessShader(shaderSource, generateDefines(ShaderType::FRAGMENT));
	preprocessedFragmentSourceStr = preprocessedFragmentSource.c_str();

#if defined(WIN32) || defined(LINUX)
	const std::string preprocessedGeometrySource = preprocessShader(shaderSource, generateDefines(ShaderType::GEOMETRY));
	preprocessedGeometrySourceStr = preprocessedGeometrySource.c_str();
	if (preprocessedGeometrySource.empty()) {
		preprocessedGeometrySourceStr = nullptr;
	}
#endif

	// call the internal load function
	loadProgramInternal(preprocessedVertexSourceStr, preprocessedFragmentSourceStr, preprocessedGeometrySourceStr);
}

// This function loads shaders without preproccesing them. Geometry source can be nullptr.
void gShader::loadProgramInternal(const char* vertexSource, const char* fragmentSource, const char* geometrySource) {
	unsigned int vertex = GL_NONE;
	unsigned int fragment = GL_NONE;
#if defined(WIN32) || defined(LINUX)
	unsigned int geometry = GL_NONE;
#endif
	// vertex shader
	G_CHECK_GL2(vertex, glCreateShader(GL_VERTEX_SHADER));
	glShaderSource(vertex, 1, &vertexSource, nullptr);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");

	// fragment Shader
	G_CHECK_GL2(fragment, glCreateShader(GL_FRAGMENT_SHADER));
	glShaderSource(fragment, 1, &fragmentSource, nullptr);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");

	// if geometry shader is given, compile geometry shader
#if defined(WIN32) || defined(LINUX)
	if(geometrySource != nullptr) {
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &geometrySource, nullptr);
		glCompileShader(geometry);
		checkCompileErrors(geometry, "GEOMETRY");
	}
#endif

	// shader Program
	id = glCreateProgram();
	loaded = true;
	glAttachShader(id, vertex);
	glAttachShader(id, fragment);
#if defined(WIN32) || defined(LINUX)
	if(geometrySource != nullptr) glAttachShader(id, geometry);
#endif
	glLinkProgram(id);
	checkCompileErrors(id, "PROGRAM");
	// delete the shaders as they're linked into our program now and no longer necessery
	glDeleteShader(vertex);
	glDeleteShader(fragment);
#if defined(WIN32) || defined(LINUX)
	if(geometrySource != nullptr) glDeleteShader(geometry);
#endif
}

void gShader::checkCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];
    if(type != "PROGRAM") {
        G_CHECK_GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
        if(!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            gLoge("gShader") << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if(!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            gLoge("gShader") << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
#ifdef DEBUG
    assert(success);
#endif
}

// Helper function to check if a string starts with a given prefix
bool startsWith(const std::string& str, const std::string& prefix) {
	return str.rfind(prefix, 0) == 0;
}

std::array<std::string, 2> splitString(const std::string& str) {
	std::array<std::string, 2> result;
	size_t pos = str.find(' ');

	if (pos != std::string::npos) {
		result[0] = str.substr(0, pos);       // Part before the first space
		result[1] = str.substr(pos + 1);       // Part after the first space
	} else {
		result[0] = str;  // If no space, the whole string is the first part
		result[1].clear();  // Clear the second part
	}

	return result;
}

std::string gShader::preprocessShader(const std::string& shaderCode, std::unordered_map<std::string, std::string> defines) {
	std::stringstream buffer;
	std::string line;
	bool in_if = false;
	bool in_else = false;
	bool is_match = false;

	std::stringstream shader_stream(shaderCode);
	std::vector<std::string> lines;
	while (std::getline(shader_stream, line)) {
		if (startsWith(line, "#if")) {
			std::string condition = line.substr(4);
			is_match = defines.find(condition) != defines.end();
			in_if = true;
		} else if (startsWith(line, "#else")) {
			in_else = true;
		} else if (in_if && startsWith(line, "#endif")) {
			for (std::string& out_line : lines) {
				buffer << out_line << '\n';
			}
			in_if = false;
			in_else = false;
			is_match = false;
			lines.clear();
		} else if (in_if) {
			if (is_match != in_else) {
				lines.push_back(line);
			}
		} else if (startsWith(line, "#define")) {
			std::string text = line.substr(8);
			std::array<std::string, 2> split = splitString(text);
			defines[split[0]] = split[1];
		} else {
			for (auto& [from, to] : defines) {
				line = gReplaceAll(line, from, to);
			}
			buffer << line << '\n';
		}
	}
	return buffer.str();
}

std::unordered_map<std::string, std::string> gShader::generateDefines(ShaderType type) {
	std::unordered_map<std::string, std::string> map;
	if (type == ShaderType::VERTEX) {
		map.insert(std::pair<std::string, std::string>("VERTEX", ""));
	} else if (type == ShaderType::FRAGMENT) {
		map.insert(std::pair<std::string, std::string>("FRAGMENT", ""));
	} else if (type == ShaderType::GEOMETRY) {
		map.insert(std::pair<std::string, std::string>("GEOMETRY", ""));
	}
#if defined(GLIST_MOBILE)
	map.insert(std::pair("GLES", ""));
#endif
	int max_lights = GLIST_MAX_LIGHTS;
	map.insert(std::pair<std::string, std::string>("GLIST_MAX_LIGHTS", gToStr(max_lights)));
	return map;
}

// activate the shader
// ------------------------------------------------------------------------
void gShader::use() const {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUseProgram(id));
}

// utility uniform functions
// ------------------------------------------------------------------------
void gShader::setBool(const std::string &name, bool value) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform1i(getUniformLocation(name), (int)value));
}

// ------------------------------------------------------------------------
void gShader::setInt(const std::string &name, int value) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform1i(getUniformLocation(name), value));
}

// ------------------------------------------------------------------------
void gShader::setFloat(const std::string &name, float value) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform1f(getUniformLocation(name), value));
}

// ------------------------------------------------------------------------
void gShader::setVec2(const std::string &name, const glm::vec2 &value) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform2fv(getUniformLocation(name), 1, &value[0]));
}

void gShader::setVec2(const std::string &name, float x, float y) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform2f(getUniformLocation(name), x, y));
}

// ------------------------------------------------------------------------
void gShader::setVec3(const std::string &name, const glm::vec3 &value) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform3fv(getUniformLocation(name), 1, &value[0]));
}

void gShader::setVec3(const std::string &name, float x, float y, float z) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform3f(getUniformLocation(name), x, y, z));
}

// ------------------------------------------------------------------------
void gShader::setVec4(const std::string &name, const glm::vec4 &value) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform4fv(getUniformLocation(name), 1, &value[0]));
}

void gShader::setVec4(const std::string &name, float x, float y, float z, float w) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniform4f(getUniformLocation(name), x, y, z, w));
}

// ------------------------------------------------------------------------
void gShader::setMat2(const std::string &name, const glm::mat2 &mat) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]));
}

// ------------------------------------------------------------------------
void gShader::setMat3(const std::string &name, const glm::mat3 &mat) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]));
}

// ------------------------------------------------------------------------
void gShader::setMat4(const std::string &name, const glm::mat4 &mat) {
#ifdef DEBUG
    assert(loaded);
#endif
    G_CHECK_GL(glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]));
}

GLint gShader::getUniformLocation(const std::string& name) {
	// Check if the location is already in the map
	auto it = uniformlocations.find(name);
	if (it != uniformlocations.end()) {
		return it->second;
	}

	// If not, get the location and store it in the map
	GLint location = glGetUniformLocation(id, name.c_str());
	uniformlocations[name] = location;
	return location;
}

