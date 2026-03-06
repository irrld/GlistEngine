/*
 * gVboBase.cpp
 *
 *  Created on: 25 Kas 2020
 *      Author: Acer
 *
 *  Refactored: Base/GL/Vk split
 */

#include "gVboBase.h"

gVboBase::gVboBase() {
	isdynamic = false;
	isenabled = true;
	isvertexdataallocated = false;
	vertexdatacoordnum = 0;
	totalvertexnum = 0;
	vertexusage = 0;
	vertexstride = 0;
	isindexdataallocated = false;
	totalindexnum = 0;
}

gVboBase::~gVboBase() {
}

void gVboBase::enable() {
	isenabled = true;
}

void gVboBase::disable() {
	isenabled = false;
}

bool gVboBase::isEnabled() const {
	return isenabled;
}

bool gVboBase::isVertexDataAllocated() const {
	return isvertexdataallocated;
}

bool gVboBase::isIndexDataAllocated() const {
	return isindexdataallocated;
}

int gVboBase::getVerticesNum() const {
	return totalvertexnum;
}

int gVboBase::getIndicesNum() const {
	return totalindexnum;
}

void gVboBase::setDynamic(bool dynamic) {
	isdynamic = dynamic;
}

bool gVboBase::isDynamic() const {
	return isdynamic;
}

void gVboBase::setColorData(const gColor* colors, int total, int usage) {
	setColorData(&colors[0].r, total, usage);
}

void gVboBase::setColorData(const float* color0r, int total, int usage, int stride) {
	gLogw("gVbo") << "setColorData(float*) is not implemented";
}

void gVboBase::setTexCoordData(const glm::vec2* texCoords, int total, int usage) {
	setTexCoordData(&texCoords[0].x, total, usage);
}

void gVboBase::setTexCoordData(const float* texCoord0x, int total, int usage, int stride) {
	gLogw("gVbo") << "setTexCoordData(float*) is not implemented";
}

void gVboBase::setNormalData(const glm::vec3* normals, int total, int usage) {
	setNormalData(&normals[0].x, total, usage);
}

void gVboBase::setNormalData(const float* normal0x, int total, int usage, int stride) {
	gLogw("gVbo") << "setNormalData(float*) is not implemented";
}

void gVboBase::setIndexData(const int* indices, int total, int usage) {
	gLogw("gVbo") << "setIndexData(int*) is not implemented";
}
