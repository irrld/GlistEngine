/*
 * gSkyTextureGL.cpp
 *
 *  Created on: 2026-03-01
 *      Author: Metehan Gezer
 *
 *  Refactored: OpenGL implementation
 */

#include "gSkyTextureGL.h"


gSkyTextureGL::gSkyTextureGL() : textureid(0) {
}

gSkyTextureGL::~gSkyTextureGL() {
	destroy();
}

uint32_t gSkyTextureGL::create() {
	G_CHECK_GL(glGenTextures(1, &textureid));
	id = textureid;
	return textureid;
}

void gSkyTextureGL::bind() {
	G_CHECK_GL(glBindTexture(GL_TEXTURE_CUBE_MAP, textureid));
}

void gSkyTextureGL::bind(int textureSlot) {
	G_CHECK_GL(glActiveTexture(textureSlot));
	G_CHECK_GL(glBindTexture(GL_TEXTURE_CUBE_MAP, textureid));
}

void gSkyTextureGL::unbind() {
	G_CHECK_GL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

void gSkyTextureGL::unbind(int textureSlotNo) {
	G_CHECK_GL(glActiveTexture(GL_TEXTURE0 + textureSlotNo));
	G_CHECK_GL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

void gSkyTextureGL::texImage2D(int faceIndex, int internalFormat, int width, int height,
		int format, int type, void* data) {
	G_CHECK_GL(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0,
			internalFormat, width, height, 0, format, type, data));
}

void gSkyTextureGL::setWrappingAndFiltering(int wrapS, int wrapT, int wrapR,
		int minFilter, int magFilter) {
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrapS));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrapT));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, wrapR));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter));
	G_CHECK_GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magFilter));
}

void gSkyTextureGL::generateMipMap() {
	G_CHECK_GL(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
}

void gSkyTextureGL::destroy() {
	if (textureid != 0) {
		G_CHECK_GL(glDeleteTextures(1, &textureid));
		textureid = 0;
		id = 0;
	}
}
