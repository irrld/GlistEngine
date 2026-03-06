/*
 * gSkyTextureBase.cpp
 *
 *  Created on: 2026-03-01
 *      Author: Metehan Gezer
 *
 *  Refactored: Base/GL/Vk split
 */

#include "gSkyTextureBase.h"


gSkyTextureBase::gSkyTextureBase() {
	id = 0;
}

gSkyTextureBase::~gSkyTextureBase() {
}

unsigned int gSkyTextureBase::getId() const {
	return id;
}
