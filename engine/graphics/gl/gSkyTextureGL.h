/*
 * gSkyTextureGL.h
 *
 *  Created on: 2026-03-01
 *      Author: Metehan Gezer
 *
 *  Refactored: OpenGL implementation
 */

#ifndef GRAPHICS_GL_GSKYTEXTUREGL_H_
#define GRAPHICS_GL_GSKYTEXTUREGL_H_

#include "gSkyTextureBase.h"


class gSkyTextureGL : public gSkyTextureBase {
public:
	gSkyTextureGL();
	~gSkyTextureGL();

	uint32_t create();

	void bind();
	void bind(int textureSlot);
	void unbind();
	void unbind(int textureSlotNo);

	void texImage2D(int faceIndex, int internalFormat, int width, int height,
			int format, int type, void* data);

	void setWrappingAndFiltering(int wrapS, int wrapT, int wrapR,
			int minFilter, int magFilter);

	void generateMipMap();

	void destroy();

private:
	GLuint textureid;
};

#endif /* GRAPHICS_GL_GSKYTEXTUREGL_H_ */
