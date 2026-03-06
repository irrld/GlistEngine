/*
 * gSkyboxVk.cpp
 *
 *  Created on: 18 Mar 2021
 *      Author: Acer
 *
 *  Refactored: Vulkan implementation
 */

#if defined(GLIST_VULKAN)

#include "gSkyboxVk.h"
#include "gImage.h"


gSkyboxVk::gSkyboxVk() {
}

gSkyboxVk::~gSkyboxVk() {
}

unsigned int gSkyboxVk::loadTextures(const std::vector<std::string>& paths) {
	std::vector<std::string> fullpaths;
	fullpaths.resize(6);
	for (size_t i = 0; i < paths.size(); i++) {
		fullpaths[i] = gGetTexturesDir() + paths[i];
	}
	return load(fullpaths);
}

unsigned int gSkyboxVk::load(const std::vector<std::string>& fullPaths) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;
	id = skyhandle.create();
	return id;
}

void gSkyboxVk::loadSkybox(gImage* images) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;
	id = skyhandle.create();
}

void gSkyboxVk::loadFromData(std::array<int, 6> widths, std::array<int, 6> heights, std::array<void*, 6> rawdata, std::array<bool, 6> ishdr) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;
	id = skyhandle.create();
}

void gSkyboxVk::loadDataSkybox(std::string* data, int width, int height) {
	skymapslot = GL_TEXTURE0;
	skymapint = 0;
	id = skyhandle.create();
}

unsigned int gSkyboxVk::loadTextureEquirectangular(const std::string& texturePath) {
	return loadEquirectangular(gGetTexturesDir() + texturePath);
}

unsigned int gSkyboxVk::loadEquirectangular(const std::string& fullPath) {
	ishdr = true;
	skymapslot = GL_TEXTURE0;
	skymapint = 0;
	id = skyhandle.create();
	return id;
}

void gSkyboxVk::generatePbrMaps() {
}

void gSkyboxVk::bindPbrMaps() {
}

void gSkyboxVk::draw() {
	vbo.bind();
	vbo.unbind();
}

#endif // GLIST_VULKAN
