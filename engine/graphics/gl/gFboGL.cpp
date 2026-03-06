/*
 * gFboGL.cpp
 *
 *  Created on: 23 Feb 2021
 *      Author: Noyan Culum
 *
 *  Refactored: OpenGL implementation
 */

#include "gFboGL.h"
#include "gTracy.h"


gFboGL::gFboGL() {
	fboid = 0;
	rbo = 0;
}

gFboGL::~gFboGL() {
	G_PROFILE_ZONE_SCOPED_N("gFbo::~gFbo()");
	delete texture;
	texture = nullptr;
	delete depthtexture;
	depthtexture = nullptr;
	if (!usedepthtexture && rbo != 0) {
		GLuint glrbo = rbo;
		G_CHECK_GL(glDeleteRenderbuffers(1, &glrbo));
		rbo = 0;
	}
	if (fboid != 0) {
		G_CHECK_GL(glDeleteFramebuffers(1, &fboid));
		fboid = 0;
	}
}

void gFboGL::allocate(int width, int height, bool isDepthMap, bool useDepthTexture) {
	G_PROFILE_ZONE_SCOPED_N("gFbo::allocate()");
	G_PROFILE_ZONE_VALUE(width);
	G_PROFILE_ZONE_VALUE(height);
	G_PROFILE_ZONE_VALUE(isDepthMap);
	if(isallocated) {
		delete texture;
		delete depthtexture;
		texture = nullptr;
		depthtexture = nullptr;
		if (!usedepthtexture && rbo != 0) {
			GLuint glrbo = rbo;
			G_CHECK_GL(glDeleteRenderbuffers(1, &glrbo));
			rbo = 0;
		}
		if (fboid != 0) {
			G_CHECK_GL(glDeleteFramebuffers(1, &fboid));
			fboid = 0;
		}
		isallocated = false;
	}

	this->width = width;
	this->height = height;
	isdepthmap = isDepthMap;
	usedepthtexture = useDepthTexture;

	G_CHECK_GL(glGenFramebuffers(1, &fboid));
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, fboid));

	if(!isDepthMap) {
		// create a color attachment texture
		texture = new gTexture(width, height, GL_RGBA, true);
		texture->bind();
		G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getId(), 0));

		if(useDepthTexture) {
			// create a sampleable depth texture attachment
			depthtexture = new gTexture(width, height, GL_DEPTH_COMPONENT, true);
			depthtexture->bind();
			G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthtexture->getId(), 0));
		} else {
			// create a renderbuffer for depth and stencil attachment
			G_CHECK_GL(glGenRenderbuffers(1, (GLuint*)&rbo));
			G_CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
			G_CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));
			G_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo));
		}
	} else {
		// create a depth attachment texture
		texture = new gTexture(width, height, GL_DEPTH_COMPONENT, true);
		texture->bind();
		G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getId(), 0));
#if defined(GLIST_OPENGLES)
		G_CHECK_GL(glDrawBuffers(0, GL_NONE));
#else
		G_CHECK_GL(glDrawBuffer(GL_NONE));
#endif
		G_CHECK_GL(glReadBuffer(GL_NONE));
	}
#if defined(DEBUG) || defined(ENGINE_OPENGL_CHECKS)
	G_CHECK_GL2(GLuint status, glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		gLogi("gFbo") << "Framebuffer is not complete! status:" << gToHex(status, 4);
	}
#endif
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, defaultfbo));
	isallocated = true;
}

unsigned int gFboGL::getId() const {
	return fboid;
}

void gFboGL::bind() {
	G_PROFILE_ZONE_SCOPED_N("gFbo::bind()");
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, fboid));
	renderer->setViewport(0, 0, width, height);
}

void gFboGL::unbind() {
	G_PROFILE_ZONE_SCOPED_N("gFbo::unbind()");
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, defaultfbo));
	renderer->setViewport(0, 0, renderer->getScreenWidth(), renderer->getScreenHeight());
}

void gFboGL::attachTexture(int attachment, int texTarget, uint32_t texId, int level) {
	G_CHECK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texTarget, texId, level));
}

uint32_t gFboGL::createRenderbuffer() {
	GLuint rbo;
	G_CHECK_GL(glGenRenderbuffers(1, &rbo));
	return rbo;
}

void gFboGL::bindRenderbuffer(uint32_t rbo) {
	G_CHECK_GL(glBindRenderbuffer(GL_RENDERBUFFER, rbo));
}

void gFboGL::setRenderbufferStorage(int format, int width, int height) {
	G_CHECK_GL(glRenderbufferStorage(GL_RENDERBUFFER, format, width, height));
}

void gFboGL::attachRenderbuffer(int attachment, uint32_t rbo) {
	G_CHECK_GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo));
}

void gFboGL::deleteRenderbuffer(uint32_t& rbo) {
	if (rbo != 0) {
		GLuint glrbo = rbo;
		G_CHECK_GL(glDeleteRenderbuffers(1, &glrbo));
		rbo = 0;
	}
}

void gFboGL::setDrawBufferNone() {
#if defined(GLIST_OPENGLES)
	G_CHECK_GL(glDrawBuffers(0, GL_NONE));
#else
	G_CHECK_GL(glDrawBuffer(GL_NONE));
#endif
}

void gFboGL::setReadBufferNone() {
	G_CHECK_GL(glReadBuffer(GL_NONE));
}

void gFboGL::checkStatus() {
	G_CHECK_GL2(GLuint status, glCheckFramebufferStatus(GL_FRAMEBUFFER));
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		gLogi("gFbo") << "Framebuffer is not complete! status:" << gToHex(status, 4);
	}
}
