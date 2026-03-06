/*
 * gFboBase.cpp
 *
 *  Created on: 23 Feb 2021
 *      Author: Noyan Culum
 *
 *  Refactored: Base/GL/Vk split
 */

#include "gFboBase.h"
#include "gTracy.h"
#include <cassert>


int gFboBase::defaultfbo = 0;

gFboBase::gFboBase() {
	width = 0;
	height = 0;
	isdepthmap = false;
	usedepthtexture = false;
	texture = nullptr;
	depthtexture = nullptr;
	isallocated = false;
}

gFboBase::~gFboBase() {
}

int gFboBase::getWidth() const {
	return width;
}

int gFboBase::getHeight() const {
	return height;
}

unsigned int gFboBase::getTextureId() const {
#ifdef DEBUG
	assert(texture);
#endif
	return texture->getId();
}

unsigned int gFboBase::getDepthTextureId() const {
#ifdef DEBUG
	assert(depthtexture);
#endif
	return depthtexture->getId();
}

gTexture& gFboBase::getTexture() {
#ifdef DEBUG
	assert(texture);
#endif
	return *texture;
}

gTexture& gFboBase::getDepthTexture() {
#ifdef DEBUG
	assert(depthtexture);
#endif
	return *depthtexture;
}

void gFboBase::draw(int x, int y) {
	if(isdepthmap) return;
	texture->draw(x, y);
}

void gFboBase::draw(int x, int y, int w, int h) {
	if(isdepthmap) return;
	texture->draw(x, y, w, h);
}

void gFboBase::draw(int x, int y, int w, int h, float rotate) {
	if(isdepthmap) return;
	texture->draw(x, y, w, h, rotate);
}

void gFboBase::draw(glm::vec2 position, glm::vec2 size, float rotate) {
	if(isdepthmap) return;
	texture->draw(position, size, rotate);
}

void gFboBase::drawSub(int x, int y, int sx, int sy, int sw, int sh) {
	if(isdepthmap) return;
	texture->drawSub(x, y, sx, sy, sw, sh);
}

void gFboBase::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh) {
	if(isdepthmap) return;
	texture->drawSub(x, y, w, h, sx, sy, sw, sh);
}

void gFboBase::drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, float rotate) {
	if(isdepthmap) return;
	texture->drawSub(x, y, w, h, sx, sy, sw, sh, rotate);
}

void gFboBase::drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subpos, glm::vec2 subsize, float rotate) {
	if(isdepthmap) return;
	texture->drawSub(pos, size, subpos, subsize, rotate);
}

void gFboBase::drawSub(const gRect& src, const gRect& dst, float rotate) {
	if(isdepthmap) return;
	texture->drawSub(src, dst, rotate);
}
