/*
 * gFboBase.h
 *
 *  Created on: 23 Feb 2021
 *      Author: Noyan
 *
 *  Refactored: Base/GL/Vk split
 */

#ifndef GRAPHICS_GFBOBASE_H_
#define GRAPHICS_GFBOBASE_H_

#include "gRenderObject.h"
#include "gTexture.h"


class gFboBase : public gRenderObject {
public:
	int getWidth() const;
	int getHeight() const;
	unsigned int getTextureId() const;
	unsigned int getDepthTextureId() const;
	gTexture& getTexture();
	gTexture& getDepthTexture();

	void draw(int x, int y);
	void draw(int x, int y, int w, int h);
	void draw(int x, int y, int w, int h, float rotate);
	void draw(glm::vec2 position, glm::vec2 size, float rotate = 0.0f);

	void drawSub(int x, int y, int sx, int sy, int sw, int sh);
	void drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh);
	void drawSub(int x, int y, int w, int h, int sx, int sy, int sw, int sh, float rotate);
	void drawSub(glm::vec2 pos, glm::vec2 size, glm::vec2 subpos, glm::vec2 subsize, float rotate = 0.0f);
	void drawSub(const gRect& src, const gRect& dst, float rotate = 0.f);

	static int defaultfbo;

protected:
	gFboBase();
	virtual ~gFboBase();

	gTexture* texture;
	gTexture* depthtexture;
	int width, height;
	bool isdepthmap;
	bool usedepthtexture;
	bool isallocated;
};

#endif /* GRAPHICS_GFBOBASE_H_ */
