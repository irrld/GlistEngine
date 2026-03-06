//
// Created by sadettin on 23.08.2025.
//

#include "gGLRenderEngine.h"

//screenShot Related includes
#include "stb/stb_image_write.h"
#include "gBaseApp.h"
#include "gImage.h"
#include "gShader.h"
#include "gTracy.h"

void gCheckGLErrorAndPrint(const std::string& prefix, const std::string& func, int line) {
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		gLogi("gGLRenderEngine") << prefix << "OpenGL ERROR at " << func << ", line " << line << ", error: " << gToHex(error, 4);
	}
}

void gEnableCulling() {
	G_CHECK_GL(glEnable(GL_CULL_FACE));
}

void gDisableCulling() {
	G_CHECK_GL(glDisable(GL_CULL_FACE));
}

bool gIsCullingEnabled() {
	G_CHECK_GL2(GLboolean res, glIsEnabled(GL_CULL_FACE));
	return res == GL_TRUE;
}

void gCullFace(int cullingFace) {
	G_CHECK_GL(glCullFace(cullingFace));
}

int gGetCullFace() {
	GLint i;
	G_CHECK_GL(glGetIntegerv(GL_CULL_FACE_MODE, &i));
	return i;
}

void gSetCullingDirection(int cullingDirection) {
	G_CHECK_GL(glFrontFace(cullingDirection));
}

int gGetCullingDirection() {
	GLint i;
	G_CHECK_GL(glGetIntegerv(GL_FRONT_FACE, &i));
	return i;
}

gGLRenderEngine::~gGLRenderEngine() {
	delete originalgrid;
}

void gGLRenderEngine::clear() {
	G_CHECK_GL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	G_CHECK_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void gGLRenderEngine::clearColor(int r, int g, int b, int a) {
	//    glBindFramebuffer(GL_FRAMEBUFFER, gFbo::defaultfbo);
	G_CHECK_GL(glClearColor((float)r / 255, (float)g / 255, (float)b / 255, (float)a / 255));
	G_CHECK_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void gGLRenderEngine::clearColor(gColor color) {
	G_CHECK_GL(glClearColor(color.r, color.g, color.b, color.a));
	G_CHECK_GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void gGLRenderEngine::enableDepthTest() {
	G_CHECK_GL(enableDepthTest(DEPTHTESTTYPE_LESS));
}

void gGLRenderEngine::enableDepthTest(int depthTestType) {
	G_CHECK_GL(glEnable(GL_DEPTH_TEST));
	G_CHECK_GL(glDepthFunc(depthtesttypeid[depthTestType]));
	isdepthtestenabled = true;
	depthtesttype = depthTestType;
}

void gGLRenderEngine::setDepthTestFunc(int depthTestType) {
	G_CHECK_GL(glDepthFunc(depthtesttypeid[depthTestType]));
	depthtesttype = depthTestType;
}

void gGLRenderEngine::disableDepthTest() {
	G_CHECK_GL(glDisable(GL_DEPTH_TEST));
	isdepthtestenabled = false;
}

bool gGLRenderEngine::isDepthTestEnabled() {
	return isdepthtestenabled;
}

int gGLRenderEngine::getDepthTestType() {
	return depthtesttype;
}

void gGLRenderEngine::enableAlphaBlending() {
	G_CHECK_GL(glEnable(GL_BLEND));
	G_CHECK_GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	isalphablendingenabled = true;
}

void gGLRenderEngine::disableAlphaBlending() {
	G_CHECK_GL(glDisable(GL_BLEND));
	isalphablendingenabled = false;
}

bool gGLRenderEngine::isAlphaBlendingEnabled() {
	return isalphablendingenabled;
}

void gGLRenderEngine::enableAlphaTest() {
#if defined(WIN32) || defined(LINUX)
	G_CHECK_GL(glEnable(GL_ALPHA_TEST));
	G_CHECK_GL(glAlphaFunc(GL_GREATER, 0.1));
	isalphatestenabled = true;
#endif
}

void gGLRenderEngine::disableAlphaTest() {
#if defined(WIN32) || defined(LINUX)
	G_CHECK_GL(glDisable(GL_ALPHA_TEST));
	isalphatestenabled = false;
#endif
}

bool gGLRenderEngine::isAlphaTestEnabled() {
	return isalphatestenabled;
}

/*
 * Rotates The Pixel Data upside down. Hence rotates flips the image upside down
 */
void flipVertically(unsigned char* pixelData, int width, int height, int numChannels) {
	G_PROFILE_ZONE_SCOPED_N("flipVertically()");
	int rowsize = width * numChannels;
	unsigned char* temprow = new unsigned char[rowsize];

	for(int row = 0; row < height / 2; ++row) {
		// Calculate the corresponding row from the bottom
		int bottomrow = height - row - 1;

		// Swap the rows
		memcpy(temprow, pixelData + row * rowsize, rowsize);
		memcpy(pixelData + row * rowsize, pixelData + bottomrow * rowsize, rowsize);
		memcpy(pixelData + bottomrow * rowsize, temprow, rowsize);
	}

	delete[] temprow;
}

void gGLRenderEngine::takeScreenshot(gImage& img, int x, int y, int width, int height) {
	G_PROFILE_ZONE_SCOPED_N("gGLRenderEngine::takeScreenshot()");
	unsigned char* pixeldata = new unsigned char[width * height * 4];
	G_CHECK_GL(glReadPixels(x, getHeight() - y - height, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata));
	flipVertically(pixeldata, width, height, 4);
	img.setImageData(pixeldata, width, height, 4);
	//std::string imagePath = "output.png";   USE IT TO SAVE THE IMAGE
	// screenShot->saveImage(imagePath);  USE IT TO SAVE THE IMAGE
}

void gGLRenderEngine::takeScreenshot(gImage& img) {
	G_PROFILE_ZONE_SCOPED_N("gGLRenderEngine::takeScreenshot()");
	int height = gBaseApp::getAppManager()->getWindow()->getHeight();
	int width = gBaseApp::getAppManager()->getWindow()->getWidth();
	unsigned char* pixeldata = new unsigned char[width * height * 4];
	G_CHECK_GL(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata));
	flipVertically(pixeldata, width, height, 4);
	img.setImageData(pixeldata, width, height, 4);
	//std::string imagePath = "output.png";   USE IT TO SAVE THE IMAGE
	// screenShot->saveImage(imagePath);  USE IT TO SAVE THE IMAGE
}


// ----- Draw -----
void gGLRenderEngine::drawArrays(int drawMode, int count) {
	G_CHECK_GL(glDrawArrays(drawMode, 0, count));
}

void gGLRenderEngine::drawElements(int drawMode, int count) {
	G_CHECK_GL(glDrawElements(drawMode, count, G_INDEX_SIZE, 0));
}

void gGLRenderEngine::setViewport(int x, int y, int width, int height) {
	G_CHECK_GL(glViewport(x, y, width, height));
}

void gGLRenderEngine::clearScreen(bool color, bool depth) {
	GLbitfield mask = 0;
	if(color) mask |= GL_COLOR_BUFFER_BIT;
	if(depth) mask |= GL_DEPTH_BUFFER_BIT;
	glClear(mask);
}

void gGLRenderEngine::bindDefaultFramebuffer() {
	G_CHECK_GL(glBindFramebuffer(GL_FRAMEBUFFER, gFbo::defaultfbo));
}

void gGLRenderEngine::enableDepthTestEqual() {
	G_CHECK_GL(glDepthFunc(GL_LEQUAL));
}

void gGLRenderEngine::enableCubeMap() {
#if defined(GLIST_OPENGLES)
	G_CHECK_GL(glEnable(GL_TEXTURE_CUBE_MAP)); // OpenGL ES does not support GL_TEXTURE_CUBE_MAP_SEAMLESS
#else
	G_CHECK_GL(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
#endif
}

void gGLRenderEngine::pushMatrix() {
#ifndef GLIST_OPENGLES
	G_CHECK_GL(glPushMatrix());
#endif
}

void gGLRenderEngine::popMatrix() {
#ifndef GLIST_OPENGLES
	G_CHECK_GL(glPopMatrix());
#endif
}

#if !defined(GLIST_OPENGLES) && (defined(DEBUG) || defined(ENGINE_OPENGL_CHECKS))
void GLAPIENTRY openglErrorCallback(GLenum source, GLenum type, GLuint id,
								   GLenum severity, GLsizei length,
								   const GLchar* message, const void* userParam) {
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
		return;
	}

	gLoge("gGLRenderEngine") << "Received OpenGL Debug Message: "<<
			"src: " << source << "\n"
			"type: " << type << "\n"
			"id: " << id << "\n"
			"severity: " << severity << "\n"
			"message: " << message;
	// We flush here because it might not show on the console immediately, having this function being spammed will slow down the game but for purpose of debugging, it is required.
	std::cerr << std::flush;
}
#endif

void gGLRenderEngine::init() {
#if !defined(GLIST_OPENGLES) && (defined(DEBUG) || defined(ENGINE_OPENGL_CHECKS))
	// On newer versions of OpenGL, debug callbacks are available; we enable them only for debug builds because it might have a performance impact.
	// You can place a debug point and go back to the original source of the message from the stack trace, because it is sync.
	if (GLEW_VERSION_4_3 || GLEW_ARB_debug_output) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openglErrorCallback, nullptr);
	}
#endif
	gRenderer::init();
}

void gGLRenderEngine::updatePackUnpackAlignment(int i) {
	G_CHECK_GL(glPixelStorei(GL_UNPACK_ALIGNMENT, i));
	G_CHECK_GL(glPixelStorei(GL_PACK_ALIGNMENT, i));
}

