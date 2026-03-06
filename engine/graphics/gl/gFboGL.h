/*
 * gFboGL.h
 *
 *  Created on: 23 Feb 2021
 *      Author: Noyan
 *
 *  Refactored: OpenGL implementation
 */

#ifndef GRAPHICS_GL_GFBOGL_H_
#define GRAPHICS_GL_GFBOGL_H_

#include "gFboBase.h"


class gFboGL : public gFboBase {
public:
	gFboGL();
	~gFboGL();

	void allocate(int width, int height, bool isDepthMap = false, bool useDepthTexture = false);
	unsigned int getId() const;

	void bind();
	void unbind();

	// Lower-level methods (used by gSkyboxGL for PBR map generation)
	void attachTexture(int attachment, int texTarget, uint32_t texId, int level = 0);
	uint32_t createRenderbuffer();
	void bindRenderbuffer(uint32_t rbo);
	void setRenderbufferStorage(int format, int width, int height);
	void attachRenderbuffer(int attachment, uint32_t rbo);
	void deleteRenderbuffer(uint32_t& rbo);
	void setDrawBufferNone();
	void setReadBufferNone();
	void checkStatus();

private:
	GLuint fboid;
	unsigned int rbo;
};

#endif /* GRAPHICS_GL_GFBOGL_H_ */
