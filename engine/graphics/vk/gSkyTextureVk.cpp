/*
 * gSkyTextureVk.cpp
 *
 *  Created on: 2026-03-01
 *      Author: Metehan Gezer
 *
 *  Refactored: Vulkan implementation
 */

#if defined(GLIST_VULKAN)

#include "gSkyTextureVk.h"


uint32_t gSkyTextureVk::nextid = 1;

gSkyTextureVk::gSkyTextureVk() {
}

gSkyTextureVk::~gSkyTextureVk() {
	destroy();
}

uint32_t gSkyTextureVk::create() {
	id = nextid++;
	return id;
}

void gSkyTextureVk::bind() {}
void gSkyTextureVk::bind(int textureSlot) {}
void gSkyTextureVk::unbind() {}
void gSkyTextureVk::unbind(int textureSlotNo) {}
void gSkyTextureVk::texImage2D(int faceIndex, int internalFormat, int width, int height,
		int format, int type, void* data) {}
void gSkyTextureVk::setWrappingAndFiltering(int wrapS, int wrapT, int wrapR,
		int minFilter, int magFilter) {}
void gSkyTextureVk::generateMipMap() {}

void gSkyTextureVk::destroy() {
	id = 0;
}

#endif // GLIST_VULKAN
