/*
 * gFboVk.cpp
 *
 *  Created on: 23 Feb 2021
 *      Author: Noyan Culum
 *
 *  Refactored: Vulkan implementation
 */

#if defined(GLIST_VULKAN)

#include "gFboVk.h"
#include "gTracy.h"


uint32_t gFboVk::nextid = 1;

gFboVk::gFboVk() {
	id = 0;
}

gFboVk::~gFboVk() {
	G_PROFILE_ZONE_SCOPED_N("gFbo::~gFbo()");
	delete texture;
	texture = nullptr;
	delete depthtexture;
	depthtexture = nullptr;
}

void gFboVk::allocate(int width, int height, bool isDepthMap, bool useDepthTexture) {
	G_PROFILE_ZONE_SCOPED_N("gFbo::allocate()");
	if(isallocated) {
		delete texture;
		delete depthtexture;
		texture = nullptr;
		depthtexture = nullptr;
		isallocated = false;
	}

	this->width = width;
	this->height = height;
	isdepthmap = isDepthMap;
	usedepthtexture = useDepthTexture;

	id = nextid++;

	if(!isDepthMap) {
		texture = new gTexture(width, height, GL_RGBA, true);
		if(useDepthTexture) {
			depthtexture = new gTexture(width, height, GL_DEPTH_COMPONENT, true);
		}
	} else {
		texture = new gTexture(width, height, GL_DEPTH_COMPONENT, true);
	}

	isallocated = true;
}

unsigned int gFboVk::getId() const {
	return id;
}

void gFboVk::bind() {
	G_PROFILE_ZONE_SCOPED_N("gFbo::bind()");
	renderer->setViewport(0, 0, width, height);
}

void gFboVk::unbind() {
	G_PROFILE_ZONE_SCOPED_N("gFbo::unbind()");
	renderer->setViewport(0, 0, renderer->getScreenWidth(), renderer->getScreenHeight());
}

#endif // GLIST_VULKAN
