/*
 * gShaderBase.cpp
 *
 *  Created on: May 7, 2020
 *      Author: noyan
 *
 *  Refactored: Base/GL/Vk split
 */

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include "gShaderBase.h"

gShaderBase::gShaderBase() : id(0), loaded(false) {
}

gShaderBase::~gShaderBase() {
}

// Helper function to check if a string starts with a given prefix
static bool startsWith(const std::string& str, const std::string& prefix) {
	return str.rfind(prefix, 0) == 0;
}

static std::array<std::string, 2> splitString(const std::string& str) {
	std::array<std::string, 2> result;
	size_t pos = str.find(' ');

	if (pos != std::string::npos) {
		result[0] = str.substr(0, pos);
		result[1] = str.substr(pos + 1);
	} else {
		result[0] = str;
		result[1].clear();
	}

	return result;
}

std::string gShaderBase::preprocessShader(const std::string& shaderCode, std::unordered_map<std::string, std::string> defines) {
	std::stringstream buffer;
	std::string line;
	bool in_if = false;
	bool in_else = false;
	bool is_match = false;

	std::stringstream shader_stream(shaderCode);
	std::vector<std::string> lines;
	while (std::getline(shader_stream, line)) {
        gStringReplace(line, "\r", "");
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
			for (auto& entry : defines) {
				auto& from = entry.first;
				auto& to = entry.second;
				line = gReplaceAll(line, from, to);
			}
			buffer << line << '\n';
		}
	}
	return buffer.str();
}

std::unordered_map<std::string, std::string> gShaderBase::generateDefines(ShaderType type) {
	std::unordered_map<std::string, std::string> map;
	if (type == ShaderType::VERTEX) {
		map.insert(std::pair<std::string, std::string>("VERTEX", ""));
	} else if (type == ShaderType::FRAGMENT) {
		map.insert(std::pair<std::string, std::string>("FRAGMENT", ""));
	} else if (type == ShaderType::GEOMETRY) {
		map.insert(std::pair<std::string, std::string>("GEOMETRY", ""));
	}
#if defined(GLIST_OPENGLES)
	map.insert(std::pair<std::string, std::string>("GLES", ""));
#endif
	int max_lights = GLIST_MAX_LIGHTS;
	map.insert(std::pair<std::string, std::string>("GLIST_MAX_LIGHTS", gToStr(max_lights)));
	return map;
}

void gShaderBase::clearUniformCaches() {
	cachedints.clear();
	cachedfloats.clear();
	cachedvec2s.clear();
	cachedvec3s.clear();
	cachedvec4s.clear();
	cachedmat2s.clear();
	cachedmat3s.clear();
	cachedmat4s.clear();
	uniformlocations.clear();
}

std::string gShaderBase::readFile(const std::string& path) {
	std::string code;
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		file.open(path);
		std::stringstream stream;
		stream << file.rdbuf();
		file.close();
		code = stream.str();
	} catch (std::ifstream::failure& e) {
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
	return code;
}
