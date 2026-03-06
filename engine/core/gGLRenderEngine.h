//
// Created by sadettin on 13.08.2025.
//

#pragma once

#ifndef CORE_GGLENGINERENDERER_H
#define CORE_GGLENGINERENDERER_H
#include "gRenderer.h"
#include "gUbo.h"

class gGLRenderEngine : public gRenderer {
public:
	gGLRenderEngine() = default;
	~gGLRenderEngine() override;

	void clear() override;
	void clearColor(int r, int g, int b, int a = 255) override;
	void clearColor(gColor color) override;

	void takeScreenshot(gImage& img, int x, int y, int width, int height) override;
	void takeScreenshot(gImage& img) override;

	void enableDepthTest() override;
	void enableDepthTest(int depthTestType) override;
	void setDepthTestFunc(int depthTestType) override;
	void disableDepthTest() override;
	bool isDepthTestEnabled() override;
	int getDepthTestType() override;

	void enableAlphaBlending() override;
	void disableAlphaBlending() override;
	bool isAlphaBlendingEnabled() override;
	void enableAlphaTest() override;
	void disableAlphaTest() override;
	bool isAlphaTestEnabled() override;

	/* -------------- Drawing ------------- */
	void drawArrays(int drawMode, int count) override;
	void drawElements(int drawMode, int count) override;
	void setViewport(int x, int y, int width, int height) override;

	/* ------------ Screen / Framebuffer ------------- */
	void clearScreen(bool color = true, bool depth = true) override;
	void bindDefaultFramebuffer() override;

	/* -------------- Depth / Skybox --------------- */
	void enableDepthTestEqual() override;
	void enableCubeMap() override;

	/* ---------------- gRenderObject ---------------- */
	void pushMatrix() override;
	void popMatrix() override;

protected:
	void init() override;
private:
	void updatePackUnpackAlignment(int i) override;
};

#endif